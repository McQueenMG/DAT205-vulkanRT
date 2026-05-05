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
        auto& asset = asset_manager->vox_asset_by_id[c].second;
        for (uint32_t v = 0; v < asset.num_variations; v++)
        {
            LOG(INFO) << "Reading asset " << c << ", variation " << v << "...";
            vox_asset_drawcall_idx[std::make_pair(c, v)] = (uint32_t)drawcalls.size();
            drawcalls.push_back({});

            // Read data
            auto [coords, materials, voxel_material_indices] =
                magica::LoadMagicaModel(asset_manager->GetVoxAsset(c).data, v);
            auto [vertices, material_indices, indices] =
                magica::CreateMeshFromMagicaModel(coords, voxel_material_indices);
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
    vox_asset_drawcall_idx.clear();
    buffer_utils.DestroyBuffer(material_index_buffer);
    buffer_utils.DestroyBuffer(index_buffer);
    buffer_utils.DestroyBuffer(vertex_buffer);
    buffer_utils.DestroyBuffer(material_buffer);
}
