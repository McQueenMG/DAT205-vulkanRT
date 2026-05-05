#pragma once
#include "vulkan_renderer_context.hpp"
#include "vulkan_renderer_buffer_utils.hpp"
#include "vulkan_renderer_scene_data.hpp"

struct AccelerationStructure
{
    Context& context;
    BufferUtils& buffer_utils;
    SceneData& scene_data;
    AccelerationStructure(Context& context, BufferUtils& buffer_utils, SceneData& scene_data)
        : context(context), buffer_utils(buffer_utils), scene_data(scene_data)
    {
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
            tlas_data[i].created = false;
    }

    Scene* current_scene = nullptr;
    BufferUtils::Buffer object_info_buffer;  // Indexed by custom instance id

    struct TLAS_data
    {
        bool created;
        BufferUtils::Buffer buffer;
        vk::AccelerationStructureKHR TLAS;
    } tlas_data[MAX_FRAMES_IN_FLIGHT];
    std::vector<BufferUtils::Buffer> BLAS_buffers;
    std::vector<vk::AccelerationStructureKHR> BLASes;

    void CreateBLASes(Scene* current_scene);
    void DestroyBLASes();
    void CreateTLAS(uint32_t swap_idx);
    void DestroyTLAS(uint32_t swap_idx);
};
