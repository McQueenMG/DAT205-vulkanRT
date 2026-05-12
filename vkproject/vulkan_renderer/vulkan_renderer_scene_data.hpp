#pragma once
#include "vulkan_renderer_context.hpp"
#include "vulkan_renderer_buffer_utils.hpp"
#include <vkproject/Scene.hpp>
#include "vulkan_renderer_texture_utils.hpp"
#include "../triangle_asset.hpp"
#include <map>

struct DrawElementsIndirectCommand
{
    uint32_t index_count;
    uint32_t instance_count;
    uint32_t first_index;
    uint32_t vertex_offset;
    uint32_t first_instance;
};


struct SceneData
{
    struct MaterialTextureGPU
    {
        vk::Image image = VK_NULL_HANDLE;
        vma::Allocation allocation;
        vk::ImageView image_view = VK_NULL_HANDLE;
    };
    void UploadMaterialTextures();
    std::vector<MaterialTextureGPU> diffuse_textures_gpu;
    std::vector<MaterialTextureGPU> roughness_textures_gpu;
    std::vector<MaterialTextureGPU> metalness_textures_gpu;
    std::vector<MaterialTextureGPU> normal_textures_gpu;
    std::vector<triangle_asset::MaterialTextures> material_textures_cpu;
    Context& context;
    BufferUtils& buffer_utils;
    TextureUtils& texture_utils;
    Scene* current_scene = nullptr;
    SceneData(Context& context, BufferUtils& buffer_utils, TextureUtils& texture_utils) : context(context), buffer_utils(buffer_utils), texture_utils(texture_utils){};
    std::vector<DrawElementsIndirectCommand> drawcalls;
    std::map<std::pair<uint32_t /* asset id */, uint32_t /* variation*/>, uint32_t /* blas_idx */>
        asset_drawcall_idx;
    BufferUtils::Buffer material_index_buffer;  // Offset from object info
    BufferUtils::Buffer vertex_buffer;
    BufferUtils::Buffer index_buffer;
    BufferUtils::Buffer material_buffer;
    BufferUtils::Buffer uv_buffer;
    void Create(Scene* current_scene);
    void Destroy();
};
