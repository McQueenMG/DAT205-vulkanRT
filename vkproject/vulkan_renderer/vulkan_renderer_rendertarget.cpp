#include "vulkan_renderer_rendertarget.hpp"

void RenderTarget::Create()
{
    ///////////////////////////////////////////////////////////////////////////
    // Create images
    ///////////////////////////////////////////////////////////////////////////
    vk::ImageCreateInfo create_info;
    create_info.imageType = vk::ImageType::e2D;
    create_info.extent.width = context.GetWindowSize().width;
    create_info.extent.height = context.GetWindowSize().height;
    create_info.extent.depth = 1;
    create_info.mipLevels = 1;
    create_info.arrayLayers = 1;
    create_info.format = vk::Format::eR32G32B32A32Sfloat;
    create_info.tiling = vk::ImageTiling::eOptimal;
    create_info.initialLayout = vk::ImageLayout::eUndefined;
    create_info.usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eColorAttachment |
                        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;
    create_info.samples = vk::SampleCountFlagBits::e1;
    create_info.sharingMode = vk::SharingMode::eExclusive;
    vma::AllocationCreateInfo alloc_create_info;
    alloc_create_info.usage = vma::MemoryUsage::eAuto;
    alloc_create_info.flags = vma::AllocationCreateFlagBits::eDedicatedMemory;
    alloc_create_info.priority = 1.0f;
    for (int i = 0; i < num_images; i++)
    {
        std::tie(images[i], image_allocs[i]) = context.allocator.createImage(create_info, alloc_create_info);
        create_info.format = vk::Format::eR16G16B16A16Sfloat;
        std::tie(normal_images[i], normal_image_allocs[i]) = context.allocator.createImage(create_info, alloc_create_info);
        create_info.format = vk::Format::eR32G32B32A32Sfloat;
#if 0  // Temp putting data in this image to find out what goes wrong.
        auto width = context.GetWindowSize().width;
        auto height = context.GetWindowSize().height;
        std::vector<glm::vec4> data(width * height);
        for (uint32_t i = 0; i < width * height; i++)
        {
            data[i] = {0.5f, 0.5f, 0.5f, 1.0f};
        }
        uint32_t data_size = uint32_t(data.size()) * sizeof(glm::vec4);
        vk::BufferCreateInfo stagingBufferInfo;
        stagingBufferInfo.size = data_size;
        stagingBufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
        vma::AllocationCreateInfo stagingAllocCreateInfo = {};
        stagingAllocCreateInfo.usage = vma::MemoryUsage::eAuto;
        stagingAllocCreateInfo.flags =
        vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eMapped;
        vma::AllocationInfo stagingAllocInfo;
        auto [staging_buffer, staging_allocation] =
            context.allocator.createBuffer(stagingBufferInfo, stagingAllocCreateInfo, stagingAllocInfo);
        memcpy(stagingAllocInfo.pMappedData, data.data(), data_size);
        texture_utils.TransitionImageLayout(images[i], vk::ImageLayout::eUndefined,
                                            vk::ImageLayout::eTransferDstOptimal);
        texture_utils.CopyBufferToImage(staging_buffer, images[i], width, height);
        texture_utils.TransitionImageLayout(images[i], vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eGeneral);
        context.allocator.destroyBuffer(staging_buffer, staging_allocation);
#else
        texture_utils.TransitionImageLayout(images[i], vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral);
        texture_utils.TransitionImageLayout(normal_images[i], vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral);
#endif
    }

    ///////////////////////////////////////////////////////////////////////////
    // Create image view
    ///////////////////////////////////////////////////////////////////////////
    for (int i = 0; i < num_images; i++)
    {
        vk::ImageViewCreateInfo image_view_create_info{};
        image_view_create_info.image = images[i];
        image_view_create_info.format = vk::Format::eR32G32B32A32Sfloat;
        image_view_create_info.viewType = vk::ImageViewType::e2D;
        image_view_create_info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        image_view_create_info.subresourceRange.baseArrayLayer = 0;
        image_view_create_info.subresourceRange.layerCount = 1;
        image_view_create_info.subresourceRange.baseMipLevel = 0;
        image_view_create_info.subresourceRange.levelCount = 1;
        image_views[i] = context.device.createImageView(image_view_create_info);

        vk::ImageViewCreateInfo normal_image_view_create_info{};
        normal_image_view_create_info.image = normal_images[i];
        normal_image_view_create_info.format = vk::Format::eR16G16B16A16Sfloat;
        normal_image_view_create_info.viewType = vk::ImageViewType::e2D;
        normal_image_view_create_info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        normal_image_view_create_info.subresourceRange.layerCount = 1;
        normal_image_view_create_info.subresourceRange.levelCount = 1;
        normal_image_views[i] = context.device.createImageView(normal_image_view_create_info);
    }
}

void RenderTarget::Destroy()
{
    for (int i = 0; i < num_images; i++)
    {
        context.allocator.destroyImage(images[i], image_allocs[i]);
        context.device.destroyImageView(image_views[i]);
        context.allocator.destroyImage(normal_images[i], normal_image_allocs[i]);
        context.device.destroyImageView(normal_image_views[i]);
    }
}
void RenderTarget::ReCreate()
{
    Destroy();
    Create();
}
