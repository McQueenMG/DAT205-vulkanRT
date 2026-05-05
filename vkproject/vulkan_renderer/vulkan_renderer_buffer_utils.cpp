#include "vulkan_renderer_buffer_utils.hpp"
#include <vkproject/log.hpp>

BufferUtils::Buffer BufferUtils::CreateBuffer(
    const uint64_t size, const vk::BufferUsageFlags& usage, vma::MemoryUsage vma_usage,
    vma::AllocationCreateFlags vma_flags, bool map_buffer)
{
    const uint64_t actual_size = size == 0 ? 4 : size;
    vk::BufferCreateInfo bufferInfo;
    bufferInfo.size = actual_size;
    bufferInfo.usage = usage;
    bufferInfo.usage |= vk::BufferUsageFlagBits::eShaderDeviceAddress;
    vma::AllocationCreateInfo allocCreateInfo = {};
    allocCreateInfo.usage = vma_usage;
    allocCreateInfo.flags = vma_flags;
    if (map_buffer) allocCreateInfo.flags |= vma::AllocationCreateFlagBits::eMapped;
    vma::AllocationInfo allocInfo;
    Buffer buffer; 
    std::tie(buffer.buffer, buffer.allocation) = context.allocator.createBuffer(bufferInfo, allocCreateInfo, allocInfo);
    if (map_buffer) buffer.host_ptr = allocInfo.pMappedData;
    else buffer.host_ptr = nullptr; 
    vk::BufferDeviceAddressInfo info{};
    info.buffer = buffer.buffer;
    buffer.device_address = context.device.getBufferAddress(info);
    buffer.size = (uint32_t)actual_size;
    return buffer;
}

BufferUtils::Buffer BufferUtils::CreateBuffer(const uint64_t size,
                                                                              void * data,
                                                                              const vk::BufferUsageFlags& usage,
                                                                              vma::MemoryUsage vma_usage,
                                                                              vma::AllocationCreateFlags vma_flags,
                                                                              bool map_buffer)
{
    auto buffer = CreateBuffer(size, usage, vma_usage, vma_flags, map_buffer);
    LoadBuffer(buffer.buffer, size, data, 0);
    return buffer; 
}

void BufferUtils::CopyBuffer(vk::Buffer src_buffer, vk::Buffer dst_buffer, uint64_t size,
                                             uint64_t offset)
{
    if (size == 0 || src_buffer == VK_NULL_HANDLE || dst_buffer == VK_NULL_HANDLE)
    {
        return;
    }
    vk::CommandBuffer command_buffer = context.BeginSingleTimeCommands();
    vk::BufferCopy copyRegion{};
    copyRegion.srcOffset = 0;       // Optional
    copyRegion.dstOffset = offset;  // Optional
    copyRegion.size = size;
    command_buffer.copyBuffer(src_buffer, dst_buffer, 1, &copyRegion);
    context.EndSingleTimeCommands(command_buffer);
}

void BufferUtils::LoadBuffer(vk::Buffer buffer, uint64_t size, const void* data, uint64_t offset)
{
    if (size == 0 || buffer == VK_NULL_HANDLE)
    {
        return;
    }
    // Put data in staging buffer
    vk::BufferCreateInfo stagingBufferInfo;
    stagingBufferInfo.size = size;
    stagingBufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
    vma::AllocationCreateInfo stagingAllocCreateInfo = {};
    stagingAllocCreateInfo.usage = vma::MemoryUsage::eAuto;
    stagingAllocCreateInfo.flags =
        vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eMapped;
    vma::AllocationInfo stagingAllocInfo;
    auto [staging_buffer, staging_allocation] =
        context.allocator.createBuffer(stagingBufferInfo, stagingAllocCreateInfo, stagingAllocInfo);
    memcpy(stagingAllocInfo.pMappedData, data, size);
    // Copy from staging buffer and delete staging buffer
    CopyBuffer(staging_buffer, buffer, size, offset);
    context.allocator.destroyBuffer(staging_buffer, staging_allocation);
}

void BufferUtils::DestroyBuffer(Buffer& buffer) 
{ 
    if (buffer.buffer == VK_NULL_HANDLE) return;
    context.allocator.destroyBuffer(buffer.buffer, buffer.allocation); 
    buffer.size = 0; 
}