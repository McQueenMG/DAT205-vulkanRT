#include "vulkan_renderer_scene_data.hpp"
#include <vkproject/asset_manager.hpp>
#include <vkproject/magica.hpp>
#include <vkproject/log.hpp>

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
    std::vector<triangle_asset::MaterialTextures> material_textures_cpu;
    material_textures_gpu.clear();
    material_textures_cpu.clear();


    for (auto &c : assets)
    {
        const auto asset_type = asset_manager->GetAssetType(c);
        const uint32_t num_variations = asset_manager->GetAssetVariationCount(c);
        for (uint32_t v = 0; v < num_variations; v++)
        {
            LOG(INFO) << "Reading asset " << c << ", variation " << v << "...";
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
                    default_material.color = glm::vec3(1.0f, 1.0f, 1.0f);
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
            for (uint32_t i = 0; i < materials.size(); i++)
                all_materials.push_back(materials[i]);
            if (asset_type != AssetManager::AssetType::Vox)
            {
                auto mesh_asset = asset_manager->GetMeshAsset(c);
                for (const auto &mat_tex : mesh_asset.variations[v].material_textures)
                    material_textures_cpu.push_back(mat_tex);
            }
            else
            {
                // Vox meshes have no textures, push defaults
                for (int i = 0; i < materials.size(); ++i)
                    material_textures_cpu.push_back(triangle_asset::MaterialTextures());
            }

            std::vector<glm::vec2> resolved_uvs; // per-vertex UVs
            for (uint32_t i = 0; i < uv_indices.size(); i++)
            {
                const glm::ivec3 &uv_tri = uv_indices[i];
                // vertex 0 of triangle j
                int uv_idx_0 = uv_tri.x;
                resolved_uvs.push_back(uv_idx_0 >= 0 ? uv_list[uv_idx_0] : glm::vec2(0.0f, 0.0f));

                // vertex 1 of triangle j
                int uv_idx_1 = uv_tri.y;
                resolved_uvs.push_back(uv_idx_1 >= 0 ? uv_list[uv_idx_1] : glm::vec2(0.0f, 0.0f));

                // vertex 2 of triangle j
                int uv_idx_2 = uv_tri.z;
                resolved_uvs.push_back(uv_idx_2 >= 0 ? uv_list[uv_idx_2] : glm::vec2(0.0f, 0.0f));
            }
            // Add UVs into the global UV buffer.
            for (const auto &uv : resolved_uvs)
                all_uvs.push_back(uv);
        }
    }

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
    material_buffer = buffer_utils.CreateBuffer(
        all_materials, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst);
    uv_buffer = buffer_utils.CreateBuffer(
        all_uvs, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst);

    UploadMaterialTextures(material_textures_cpu);
    
}

void SceneData::UploadMaterialTextures(const std::vector<triangle_asset::MaterialTextures> &cpu_textures)
{
    material_textures_gpu.resize(cpu_textures.size());

    for (uint32_t mat_idx = 0; mat_idx < cpu_textures.size(); ++mat_idx)
    {
        const auto &cpu_tex = cpu_textures[mat_idx];
        auto &gpu_tex = material_textures_gpu[mat_idx];

        // Only upload diffuse for now
        if (cpu_tex.has_diffuse && !cpu_tex.diffuse_map.data.empty())
        {
            const auto &tex_map = cpu_tex.diffuse_map;
            uint32_t tex_size = tex_map.data.size();

            // Create staging buffer
            vk::BufferCreateInfo staging_info;
            staging_info.size = tex_size;
            staging_info.usage = vk::BufferUsageFlagBits::eTransferSrc;
            vma::AllocationCreateInfo staging_alloc;
            staging_alloc.usage = vma::MemoryUsage::eAuto;
            staging_alloc.flags = vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eMapped;
            vma::AllocationInfo staging_alloc_info;
            auto [staging_buf, staging_alloc_handle] = context.allocator.createBuffer(staging_info, staging_alloc, staging_alloc_info);
            memcpy(staging_alloc_info.pMappedData, tex_map.data.data(), tex_size);

            // Create VkImage
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

            // Upload: transition → copy → transition
            texture_utils.TransitionImageLayout(gpu_tex.image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
            texture_utils.CopyBufferToImage(staging_buf, gpu_tex.image, tex_map.width, tex_map.height);
            texture_utils.TransitionImageLayout(gpu_tex.image, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

            // Create ImageView
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

            LOG(INFO) << "Uploaded texture for material " << mat_idx << " (" << tex_map.width << "x" << tex_map.height << ")\n";

            // Clean up staging
            context.allocator.destroyBuffer(staging_buf, staging_alloc_handle);
        }
    }
}

void SceneData::Destroy()
{
    drawcalls.clear();
    asset_drawcall_idx.clear();

    // Destroy textures
    for (auto &gpu_tex : material_textures_gpu)
    {
        if (gpu_tex.image)
        {
            context.device.destroyImageView(gpu_tex.image_view);
            context.allocator.destroyImage(gpu_tex.image, gpu_tex.allocation);
        }
    }
    material_textures_gpu.clear();

    buffer_utils.DestroyBuffer(material_index_buffer);
    buffer_utils.DestroyBuffer(index_buffer);
    buffer_utils.DestroyBuffer(vertex_buffer);
    buffer_utils.DestroyBuffer(material_buffer);
    buffer_utils.DestroyBuffer(uv_buffer);
}
