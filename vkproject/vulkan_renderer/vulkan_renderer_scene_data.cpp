#include "vulkan_renderer_scene_data.hpp"
#include <vkproject/asset_manager.hpp>
#include <vkproject/magica.hpp>
#include <vkproject/log.hpp>


void SceneData::Create(Scene* _current_scene)
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

    for (auto& c : assets)
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

                const auto& mesh = mesh_asset.variations[v];
                vertices = mesh.vertices;
                indices = mesh.indices;
                material_indices = mesh.material_indices;
                materials = mesh.materials;

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

            auto& drawcall = drawcalls.back();
            drawcall.vertex_offset = (uint32_t)all_vertices.size();
            drawcall.first_index = (uint32_t)all_indices.size() * 3;
            drawcall.instance_count = 1;  // Instance count
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
}

void SceneData::Destroy()
{
    drawcalls.clear();
    asset_drawcall_idx.clear();
    buffer_utils.DestroyBuffer(material_index_buffer);
    buffer_utils.DestroyBuffer(index_buffer);
    buffer_utils.DestroyBuffer(vertex_buffer);
    buffer_utils.DestroyBuffer(material_buffer);
}
