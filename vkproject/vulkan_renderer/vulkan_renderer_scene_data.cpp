#include "vulkan_renderer_scene_data.hpp"
#include <vkproject/asset_manager.hpp>
#include <vkproject/magica.hpp>
#include <vkproject/log.hpp>
#include <glm/glm.hpp>
// import set
#include <set>

void SceneData::Create(Scene *_current_scene)
{
    ///////////////////////////////////////////////////////////////////////////
    // Create a "template" drawcall for each asset. These are then copied to
    // create the current list of drawcalls. Templates and vertex/index/etc
    // buffers are only changed between scenes, but drawcall list is updated
    // every frame.
    ///////////////////////////////////////////////////////////////////////////
    current_scene = _current_scene;

    std::vector<uint32_t> assets = current_scene->GetUsedAssets();
    std::vector<uint32_t> all_material_indices;
    std::vector<Material> all_materials;
    std::vector<glm::vec4> all_vertices;
    std::vector<glm::ivec3> all_indices;
    std::vector<glm::vec2> all_uvs;

    material_textures_cpu.clear();
    diffuse_textures_gpu.clear();
    roughness_textures_gpu.clear();
    metalness_textures_gpu.clear();
    normal_textures_gpu.clear();

    for (auto &c : assets)
    {
        const auto asset_type = asset_manager->GetAssetType(c);
        const uint32_t num_variations = asset_manager->GetAssetVariationCount(c);
        for (uint32_t v = 0; v < num_variations; v++)
        {
            asset_drawcall_idx[std::make_pair(c, v)] = (uint32_t)drawcalls.size();
            drawcalls.push_back({});

            std::vector<glm::vec3> vertices;
            std::vector<uint32_t> material_indices;
            std::vector<glm::ivec3> indices;
            std::vector<Material> materials;
            std::vector<glm::vec2> uv_list;
            std::vector<glm::ivec3> uv_indices;

            if (asset_type == AssetManager::AssetType::Vox)
            {
                auto [coords, loaded_materials, voxel_material_indices] =
                    magica::LoadMagicaModel(asset_manager->GetVoxAsset(c).data, v);
                auto [loaded_vertices, loaded_material_indices, loaded_indices] =
                    magica::CreateMeshFromMagicaModel(coords, voxel_material_indices);
                vertices = std::move(loaded_vertices);
                material_indices = std::move(loaded_material_indices);
                indices = std::move(loaded_indices);
                materials = std::move(loaded_materials);
            }
            else
            {
                auto mesh_asset = asset_manager->GetMeshAsset(c);
                if (v >= mesh_asset.variations.size())
                {
                    LOG(ERROR) << "Variation " << v << " out of bounds for mesh asset " << c;
                    continue;
                }

                const auto &mesh = mesh_asset.variations[v];
                vertices = mesh.vertices;
                indices = mesh.indices;
                material_indices = mesh.material_indices;
                materials = mesh.materials;
                uv_indices = mesh.uv_indices;
                uv_list = mesh.uv_list;

                if (materials.empty())
                {
                    Material default_material{};
                    default_material.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
                    default_material.emittance = 0.0f;
                    default_material.shininess = 0.0f;
                    default_material.metalness = 0.0f;
                    materials.push_back(default_material);
                }

                if (material_indices.empty())
                {
                    material_indices.resize(indices.size(), 0);
                }
            }

            assert(indices.size() == material_indices.size());

            auto &drawcall = drawcalls.back();
            drawcall.vertex_offset = (uint32_t)all_vertices.size();
            drawcall.first_index = (uint32_t)all_indices.size() * 3;
            drawcall.instance_count = 1; // Instance count
            drawcall.first_instance = 0;
            drawcall.index_count = (uint32_t)indices.size() * 3;

            // Add vertices, indices, etc. into big global buffers.
            for (uint32_t i = 0; i < indices.size(); i++)
                all_indices.push_back(indices[i]);
            for (uint32_t i = 0; i < material_indices.size(); i++)
                all_material_indices.push_back(material_indices[i] + (uint32_t)all_materials.size());
            for (uint32_t i = 0; i < vertices.size(); i++)
                all_vertices.push_back(glm::vec4(vertices[i], 1.0f));

            const uint32_t material_texture_base = static_cast<uint32_t>(material_textures_cpu.size());
            for (uint32_t i = 0; i < materials.size(); i++)
            {
                materials[i].diffuse_texture_index = static_cast<int>(material_texture_base + i);
                materials[i].roughness_texture_index = static_cast<int>(material_texture_base + i);
                materials[i].metalness_texture_index = static_cast<int>(material_texture_base + i);
                materials[i].normal_texture_index = static_cast<int>(material_texture_base + i);
                if (materials[i].diffuse_texture_index < 0 || materials[i].diffuse_texture_index >= 256)
                {
                    LOG(ERROR) << "Clamping texture indices " << materials[i].diffuse_texture_index << " -> 255 for material " << (all_materials.size() + i);
                    materials[i].diffuse_texture_index = 255;
                    materials[i].roughness_texture_index = 255;
                    materials[i].metalness_texture_index = 255;
                    materials[i].normal_texture_index = 255;
                }
                all_materials.push_back(materials[i]);
            }
            if (asset_type != AssetManager::AssetType::Vox)
            {
                auto mesh_asset = asset_manager->GetMeshAsset(c);
                for (size_t tex_idx = 0; tex_idx < mesh_asset.variations[v].material_textures.size(); ++tex_idx)
                {
                    const auto &mat_tex = mesh_asset.variations[v].material_textures[tex_idx];
                    material_textures_cpu.push_back(mat_tex);
                }
            }
            else
            {
                // Vox meshes have no textures, push defaults
                for (int i = 0; i < static_cast<int>(materials.size()); ++i)
                {
                    material_textures_cpu.push_back(triangle_asset::MaterialTextures());
                }
            }

            for (uint32_t i = 0; i < materials.size(); ++i)
            {
                uint32_t global_mat_idx = static_cast<uint32_t>(all_materials.size() - materials.size() + i);
                uint32_t expected_tex_idx = material_texture_base + i;
                bool cpu_has_rough = (expected_tex_idx < material_textures_cpu.size()) ? material_textures_cpu[expected_tex_idx].has_roughness : false;
                LOG(INFO) << "Asset " << c << " var " << v << " local_mat=" << i
                          << " global_mat=" << global_mat_idx
                          << " tex_idx=" << expected_tex_idx
                          << " cpu_has_rough=" << cpu_has_rough;
            }

            std::vector<glm::vec2> resolved_uvs; // 3 UVs per triangle, aligned with all_indices
            resolved_uvs.reserve(indices.size() * 3);
            for (uint32_t i = 0; i < indices.size(); i++)
            {
                if (i < uv_indices.size())
                {
                    const glm::ivec3 &uv_tri = uv_indices[i];
                    int uv_idx_0 = uv_tri.x;
                    int uv_idx_1 = uv_tri.y;
                    int uv_idx_2 = uv_tri.z;
                    resolved_uvs.push_back(uv_idx_0 >= 0 ? uv_list[uv_idx_0] : glm::vec2(0.0f, 0.0f));
                    resolved_uvs.push_back(uv_idx_1 >= 0 ? uv_list[uv_idx_1] : glm::vec2(0.0f, 0.0f));
                    resolved_uvs.push_back(uv_idx_2 >= 0 ? uv_list[uv_idx_2] : glm::vec2(0.0f, 0.0f));
                }
                else
                {
                    resolved_uvs.push_back(glm::vec2(0.0f, 0.0f));
                    resolved_uvs.push_back(glm::vec2(0.0f, 0.0f));
                    resolved_uvs.push_back(glm::vec2(0.0f, 0.0f));
                }
            }
            // Add UVs into the global UV buffer.
            for (const auto &uv : resolved_uvs)
                all_uvs.push_back(uv);
        }
    }

    for (size_t i = 0; i < all_material_indices.size(); ++i)
    {
        uint32_t mid = all_material_indices[i];
        if (mid >= all_materials.size())
        {
            LOG(ERROR) << "Triangle " << i << " references material " << mid << " (out of range " << all_materials.size() << ")";
        }
    }

    const size_t texture_count = std::max<size_t>(1, std::min<size_t>(material_textures_cpu.size(), 256));
    int last_idx = static_cast<int>(texture_count - 1);
    for (size_t mi = 0; mi < all_materials.size(); ++mi)
    {
        auto &mat = all_materials[mi];
        if (mat.diffuse_texture_index < 0 || mat.diffuse_texture_index > last_idx)
        {
            LOG(WARNING) << "Remapping material " << mi << " tex index "
                         << mat.diffuse_texture_index << " -> " << last_idx;
            mat.diffuse_texture_index = last_idx;
            mat.roughness_texture_index = last_idx;
            mat.metalness_texture_index = last_idx;
            mat.normal_texture_index = last_idx;
        }
    }

    // Diagnostic: dump material -> texture mapping and used material ids
    std::set<uint32_t> used_material_ids;
    for (auto id : all_material_indices)
        used_material_ids.insert(id);

    // Store all colors and vertices and stuff as big buffers
    index_buffer = buffer_utils.CreateBuffer(
        all_indices, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst |
                         vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
                         vk::BufferUsageFlagBits::eIndexBuffer);
    vertex_buffer = buffer_utils.CreateBuffer(
        all_vertices, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst |
                          vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
                          vk::BufferUsageFlagBits::eVertexBuffer);
    material_index_buffer = buffer_utils.CreateBuffer(
        all_material_indices, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst);

    std::cout << "EXPECTED std430 offsets: color=0, emittance=16, diffuse_idx=32\n";
    std::cout << "sizeof(Material)=" << sizeof(Material) << "\n";
    std::cout << "offsetof(color)=" << offsetof(Material, color) << "\n";
    std::cout << "offsetof(emittance)=" << offsetof(Material, emittance) << "\n";
    std::cout << "offsetof(diffuse_texture_index)=" << offsetof(Material, diffuse_texture_index) << "\n";
    std::cout << "offsetof(roughness_texture_index)=" << offsetof(Material, roughness_texture_index) << "\n";
    std::cout << "offsetof(metalness_texture_index)=" << offsetof(Material, metalness_texture_index) << "\n";
    std::cout << "offsetof(normal_texture_index)=" << offsetof(Material, normal_texture_index) << "\n";
    std::cout << "offsetof(flip_uv_x)=" << offsetof(Material, flip_uv_x) << "\n";
    std::cout << "offsetof(flip_uv_y)=" << offsetof(Material, flip_uv_y) << "\n";

    for (size_t i = 0; i < std::min<size_t>(5, all_materials.size()); ++i)
    {
        const uint8_t *p = reinterpret_cast<const uint8_t *>(&all_materials[i]);
        printf("Material[%zu] raw:", i);
        for (size_t b = 0; b < sizeof(Material); ++b)
            printf(" %02X", p[b]);
        printf("\n");
        printf("  CPU ints: diff=%d rough=%d metal=%d norm=%d\n",
               all_materials[i].diffuse_texture_index,
               all_materials[i].roughness_texture_index,
               all_materials[i].metalness_texture_index,
               all_materials[i].normal_texture_index);
    }

    material_buffer = buffer_utils.CreateBuffer(
        all_materials, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc);

    size_t mat_buf_size = all_materials.size() * sizeof(Material);
    auto host_read_buf = buffer_utils.CreateBuffer(
        mat_buf_size,
        vk::BufferUsageFlagBits::eTransferDst,
        vma::MemoryUsage::eCpuToGpu,
        vma::AllocationCreateFlagBits::eMapped,
        true);

    // copy device material_buffer -> host_read_buf
    buffer_utils.CopyBuffer(material_buffer.buffer, host_read_buf.buffer, mat_buf_size);

    // Give the driver a moment: flush/ensure the mapped pointer is valid (host_read_buf.host_ptr is mapped)
    const uint8_t *gpu_bytes = reinterpret_cast<const uint8_t *>(host_read_buf.host_ptr);
    for (size_t i = 0; i < std::min<size_t>(4, all_materials.size()); ++i)
    {
        const uint8_t *p = gpu_bytes + i * sizeof(Material);
        printf("GPU Material[%zu] raw:", i);
        for (size_t b = 0; b < sizeof(Material); ++b)
            printf(" %02X", p[b]);
        printf("\n");
        const int *ints = reinterpret_cast<const int *>(p + offsetof(Material, diffuse_texture_index));
        printf("  GPU ints: diff=%d rough=%d metal=%d norm=%d\n", ints[0], ints[1], ints[2], ints[3]);
    }

    // cleanup
    buffer_utils.DestroyBuffer(host_read_buf);

    uv_buffer = buffer_utils.CreateBuffer(
        all_uvs, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst);

    assert(all_uvs.size() == all_indices.size() * 3);

    int max_tex_idx = -1;
    for (const auto &m : all_materials)
        max_tex_idx = std::max(max_tex_idx, m.diffuse_texture_index);

    LOG(INFO) << "=== Material Assignment Diagnostic ===";
    LOG(INFO) << "Total triangles: " << all_material_indices.size();
    LOG(INFO) << "Total unique materials: " << all_materials.size();

    // Count material usage
    std::map<uint32_t, size_t> mat_tri_count;
    std::map<uint32_t, glm::vec3> mat_avg_center;
    std::map<uint32_t, float> mat_min_z;
    std::map<uint32_t, float> mat_max_z;
    std::map<uint32_t, std::vector<glm::vec3>> mat_tri_centers;

    for (size_t tri = 0; tri < all_material_indices.size(); ++tri)
    {
        uint32_t mat_id = all_material_indices[tri];
        mat_tri_count[mat_id]++;

        // Compute triangle center for spatial analysis
        glm::vec3 tri_center = (glm::vec3(all_vertices[all_indices[tri].x]) +
                                glm::vec3(all_vertices[all_indices[tri].y]) +
                                glm::vec3(all_vertices[all_indices[tri].z])) /
                               3.0f;

        if (mat_avg_center.find(mat_id) == mat_avg_center.end())
        {
            mat_avg_center[mat_id] = glm::vec3(0.0f);
            mat_min_z[mat_id] = tri_center.z;
            mat_max_z[mat_id] = tri_center.z;
        }
        mat_avg_center[mat_id] += tri_center;
        mat_min_z[mat_id] = std::min(mat_min_z[mat_id], tri_center.z);
        mat_max_z[mat_id] = std::max(mat_max_z[mat_id], tri_center.z);
        mat_tri_centers[mat_id].push_back(tri_center);
    }

    int probeMat = 2; // change to the material id you saw missing a diffuse
    int found = 0;
    for (size_t tri = 0; tri < all_material_indices.size() && found < 8; ++tri)
    {
        if ((int)all_material_indices[tri] == probeMat)
        {
            glm::vec2 uv0 = all_uvs[tri * 3 + 0];
            glm::vec2 uv1 = all_uvs[tri * 3 + 1];
            glm::vec2 uv2 = all_uvs[tri * 3 + 2];
            ++found;
        }
    }

    // Count how many triangles have valid vs zero UVs
    int tri_valid_uv = 0, tri_zero_uv = 0;
    for (size_t i = 0; i < all_uvs.size(); i += 3)
    {
        bool is_zero = (all_uvs[i].x == 0 && all_uvs[i].y == 0 &&
                        all_uvs[i + 1].x == 0 && all_uvs[i + 1].y == 0 &&
                        all_uvs[i + 2].x == 0 && all_uvs[i + 2].y == 0);
        if (is_zero)
            ++tri_zero_uv;
        else
            ++tri_valid_uv;
    }
    LOG(INFO) << "UV coverage: " << tri_valid_uv << " triangles with valid UVs, "
              << tri_zero_uv << " with zero UVs";

    for (size_t d = 0; d < drawcalls.size(); ++d)
    {
        const auto &dc = drawcalls[d];
        size_t prim_start = dc.first_index / 3;
        size_t prim_count = dc.index_count / 3;
        LOG(INFO) << "Drawcall " << d << " prims=" << prim_count << " first_prim=" << prim_start;
        for (size_t p = 0; p < std::min<size_t>(prim_count, 8); ++p)
        {
            uint32_t mat_id = all_material_indices[prim_start + p];
            LOG(INFO) << " prim[" << (prim_start + p) << "] mat=" << mat_id;
        }
    }

    LOG(INFO) << "Before UploadMaterialTextures:";
    for (size_t i = 0; i < std::min(size_t(5), all_materials.size()); ++i)
    {
        LOG(INFO) << "  Material[" << i << "]: "
                  << "diffuse_idx=" << all_materials[i].diffuse_texture_index
                  << ", roughness_idx=" << all_materials[i].roughness_texture_index
                  << ", metalness_idx=" << all_materials[i].metalness_texture_index
                  << ", shininess=" << all_materials[i].shininess;

        if (i < material_textures_cpu.size())
        {
            LOG(INFO) << "    MaterialTextures[" << i << "]: "
                      << "has_diffuse=" << material_textures_cpu[i].has_diffuse
                      << ", has_roughness=" << material_textures_cpu[i].has_roughness
                      << ", has_metalness=" << material_textures_cpu[i].has_metalness;
        }
    }
    for (size_t i = 0; i < all_materials.size(); ++i)
    {
        bool cpu_has_rough = (i < material_textures_cpu.size()) ? material_textures_cpu[i].has_roughness : false;
        LOG(INFO) << "GlobalMaterial[" << i << "]: diff_idx=" << all_materials[i].diffuse_texture_index
                  << " rough_idx=" << all_materials[i].roughness_texture_index
                  << " cpu_has_rough=" << cpu_has_rough;
    }

    UploadMaterialTextures();
}

void SceneData::UploadMaterialTextures()
{
    constexpr int LOG_SAMPLE_COUNT = 8;

    const size_t texture_count = std::max<size_t>(1, std::min<size_t>(material_textures_cpu.size(), 256));
    diffuse_textures_gpu.resize(texture_count);
    roughness_textures_gpu.resize(texture_count);
    metalness_textures_gpu.resize(texture_count);
    normal_textures_gpu.resize(texture_count);

    triangle_asset::TextureMap white_texture;
    white_texture.data = {255, 255, 255, 255};
    white_texture.width = 1;
    white_texture.height = 1;
    white_texture.channels = 4;

    triangle_asset::TextureMap roughness_fallback_texture;
    roughness_fallback_texture.data = {255, 255, 255, 255}; // 1.0 = very rough/matte
    roughness_fallback_texture.width = 1;
    roughness_fallback_texture.height = 1;
    roughness_fallback_texture.channels = 4;

    triangle_asset::TextureMap black_texture;
    black_texture.data = {0, 0, 0, 255};
    black_texture.width = 1;
    black_texture.height = 1;
    black_texture.channels = 4;

    triangle_asset::TextureMap flat_normal_texture;
    flat_normal_texture.data = {128, 128, 255, 255};
    flat_normal_texture.width = 1;
    flat_normal_texture.height = 1;
    flat_normal_texture.channels = 4;

    const triangle_asset::MaterialTextures empty_material_textures{};

    auto upload_texture_array = [&](auto map_member, auto flag_member, std::vector<MaterialTextureGPU> &gpu_textures,
                                    const triangle_asset::TextureMap &fallback_texture, const char *label)
    {
        for (uint32_t mat_idx = 0; mat_idx < texture_count; ++mat_idx)
        {
            const auto &cpu_tex = (mat_idx < material_textures_cpu.size()) ? material_textures_cpu[mat_idx] : empty_material_textures;
            auto &gpu_tex = gpu_textures[mat_idx];

            const auto &tex_map = ((cpu_tex.*flag_member) && !(cpu_tex.*map_member).data.empty()) ? (cpu_tex.*map_member) : fallback_texture;
            const uint32_t tex_size = static_cast<uint32_t>(tex_map.data.size());

            vk::BufferCreateInfo staging_info;
            staging_info.size = tex_size;
            staging_info.usage = vk::BufferUsageFlagBits::eTransferSrc;
            vma::AllocationCreateInfo staging_alloc;
            staging_alloc.usage = vma::MemoryUsage::eAuto;
            staging_alloc.flags = vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eMapped;
            vma::AllocationInfo staging_alloc_info;
            auto [staging_buf, staging_alloc_handle] = context.allocator.createBuffer(staging_info, staging_alloc, staging_alloc_info);
            memcpy(staging_alloc_info.pMappedData, tex_map.data.data(), tex_size);

            vk::ImageCreateInfo img_info;
            img_info.imageType = vk::ImageType::e2D;
            img_info.extent = vk::Extent3D(tex_map.width, tex_map.height, 1);
            img_info.mipLevels = 1;
            img_info.arrayLayers = 1;
            img_info.format = vk::Format::eR8G8B8A8Unorm;
            img_info.tiling = vk::ImageTiling::eOptimal;
            img_info.initialLayout = vk::ImageLayout::eUndefined;
            img_info.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
            img_info.samples = vk::SampleCountFlagBits::e1;
            img_info.sharingMode = vk::SharingMode::eExclusive;

            vma::AllocationCreateInfo img_alloc;
            img_alloc.usage = vma::MemoryUsage::eAuto;
            img_alloc.flags = vma::AllocationCreateFlagBits::eDedicatedMemory;

            std::tie(gpu_tex.image, gpu_tex.allocation) = context.allocator.createImage(img_info, img_alloc);

            texture_utils.TransitionImageLayout(gpu_tex.image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
            texture_utils.CopyBufferToImage(staging_buf, gpu_tex.image, tex_map.width, tex_map.height);
            texture_utils.TransitionImageLayout(gpu_tex.image, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

            vk::ImageViewCreateInfo view_info;
            view_info.image = gpu_tex.image;
            view_info.viewType = vk::ImageViewType::e2D;
            view_info.format = vk::Format::eR8G8B8A8Unorm;
            view_info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            view_info.subresourceRange.baseArrayLayer = 0;
            view_info.subresourceRange.layerCount = 1;
            view_info.subresourceRange.baseMipLevel = 0;
            view_info.subresourceRange.levelCount = 1;
            gpu_tex.image_view = context.device.createImageView(view_info);

            std::string src = ((cpu_tex.*flag_member) && !(cpu_tex.*map_member).data.empty())
                                  ? (cpu_tex.*map_member).source_path
                                  : std::string("<fallback>");
            if (mat_idx < LOG_SAMPLE_COUNT)
            {
                LOG(INFO) << label << " view[" << mat_idx << "] = "
                          << (uint64_t)static_cast<VkImageView>(gpu_tex.image_view)
                          << " path=" << src;
            }
            else if (mat_idx == texture_count - 1)
            {
                LOG(INFO) << label << " view[last=" << mat_idx << "] = "
                          << (uint64_t)static_cast<VkImageView>(gpu_tex.image_view)
                          << " path=" << src;
            }

            context.allocator.destroyBuffer(staging_buf, staging_alloc_handle);
        }
    };

    upload_texture_array(&triangle_asset::MaterialTextures::diffuse_map, &triangle_asset::MaterialTextures::has_diffuse, diffuse_textures_gpu, white_texture, "diffuse");
    upload_texture_array(&triangle_asset::MaterialTextures::roughness_map, &triangle_asset::MaterialTextures::has_roughness, roughness_textures_gpu, roughness_fallback_texture, "roughness");
    upload_texture_array(&triangle_asset::MaterialTextures::metalness_map, &triangle_asset::MaterialTextures::has_metalness, metalness_textures_gpu, black_texture, "metalness");
    upload_texture_array(&triangle_asset::MaterialTextures::normal_map, &triangle_asset::MaterialTextures::has_normal, normal_textures_gpu, flat_normal_texture, "normal");
}

void SceneData::Destroy()
{
    drawcalls.clear();
    asset_drawcall_idx.clear();

    auto destroy_texture_vec = [&](std::vector<MaterialTextureGPU> &textures)
    {
        for (auto &gpu_tex : textures)
        {
            if (gpu_tex.image)
            {
                context.device.destroyImageView(gpu_tex.image_view);
                context.allocator.destroyImage(gpu_tex.image, gpu_tex.allocation);
            }
        }
        textures.clear();
    };

    destroy_texture_vec(diffuse_textures_gpu);
    destroy_texture_vec(roughness_textures_gpu);
    destroy_texture_vec(metalness_textures_gpu);
    destroy_texture_vec(normal_textures_gpu);

    buffer_utils.DestroyBuffer(material_index_buffer);
    buffer_utils.DestroyBuffer(index_buffer);
    buffer_utils.DestroyBuffer(vertex_buffer);
    buffer_utils.DestroyBuffer(material_buffer);
    buffer_utils.DestroyBuffer(uv_buffer);
}
