#pragma once
#include "vulkan_renderer_context.hpp"

struct BufferUtils
{
    struct Buffer
    {
        vk::Buffer buffer = {};
        vma::Allocation allocation;
        void* host_ptr;
        vk::DeviceAddress device_address;
        uint32_t size;
    };
    Context& context;
    BufferUtils(Context& context) : context(context) {}
    Buffer CreateBuffer(const uint64_t size, const vk::BufferUsageFlags& usage,
                        vma::MemoryUsage vma_usage = vma::MemoryUsage::eAuto, vma::AllocationCreateFlags vma_flags = {},
                        bool map_buffer = false);
    Buffer CreateBuffer(const uint64_t size, void* data, const vk::BufferUsageFlags& usage,
                        vma::MemoryUsage vma_usage = vma::MemoryUsage::eAuto, vma::AllocationCreateFlags vma_flags = {},
                        bool map_buffer = false);
    template <typename T>
    Buffer CreateBuffer(const std::vector<T> data, const vk::BufferUsageFlags& usage,
                                                     vma::MemoryUsage vma_usage = vma::MemoryUsage::eAuto,
                                                     vma::AllocationCreateFlags vma_flags = {}, bool map_buffer = false)
    {
        return CreateBuffer(data.size() * sizeof(T), (void*)data.data(), usage, vma_usage, vma_flags, map_buffer);
    }
    void CopyBuffer(vk::Buffer src_buffer, vk::Buffer dst_buffer, uint64_t size, uint64_t offset = 0);
    void LoadBuffer(vk::Buffer buffer, uint64_t size, const void* data, uint64_t offset = 0);
    void DestroyBuffer(Buffer& buffer);
};