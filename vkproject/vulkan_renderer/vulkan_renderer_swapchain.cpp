#include "vulkan_renderer_swapchain.hpp"
#include <vkproject/log.hpp>


void SwapChain::Create() 
{
    ///////////////////////////////////////////////////////////////////////////
    // Create swapchain (and handles to swap chain images)
    ///////////////////////////////////////////////////////////////////////////
    {
        glfwGetFramebufferSize(context.glfw_window, (int*)&extent.width, (int*)&extent.height);
        extent = context.GetWindowSize(); 
        vk::SwapchainCreateInfoKHR create_info;
        create_info.surface = context.surface;
        create_info.minImageCount = context.physical_device.getSurfaceCapabilitiesKHR(context.surface).minImageCount;
        create_info.imageFormat = context.required_surface_format.format;
        create_info.imageColorSpace = context.required_surface_format.colorSpace;
        create_info.imageExtent = extent; 
        create_info.imageArrayLayers = 1;
        create_info.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
        create_info.imageSharingMode = vk::SharingMode::eExclusive;
        create_info.queueFamilyIndexCount = 0;      // Optional
        create_info.pQueueFamilyIndices = nullptr;  // Optional
        create_info.preTransform = context.physical_device.getSurfaceCapabilitiesKHR(context.surface).currentTransform;
        create_info.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        create_info.presentMode = vk::PresentModeKHR::eFifo;
        create_info.clipped = VK_TRUE;
        swapchain = context.device.createSwapchainKHR(create_info);
        images[0] = context.device.getSwapchainImagesKHR(swapchain)[0];
        images[1] = context.device.getSwapchainImagesKHR(swapchain)[1];
        current_image_index = 0;
    }
    {  // Create image views for swap chain images
        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vk::ImageViewCreateInfo create_info;
            create_info.image = images[i];
            create_info.viewType = vk::ImageViewType::e2D;
            create_info.format = context.required_surface_format.format;
            create_info.components.r = vk::ComponentSwizzle::eIdentity;
            create_info.components.g = vk::ComponentSwizzle::eIdentity;
            create_info.components.b = vk::ComponentSwizzle::eIdentity;
            create_info.components.a = vk::ComponentSwizzle::eIdentity;
            create_info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            create_info.subresourceRange.baseMipLevel = 0;
            create_info.subresourceRange.levelCount = 1;
            create_info.subresourceRange.baseArrayLayer = 0;
            create_info.subresourceRange.layerCount = 1;
            image_views[i] = context.device.createImageView(create_info);
        }
    }
    
    // Create depth image and view
    {
        vk::ImageCreateInfo create_info;
        create_info.imageType = vk::ImageType::e2D;
        create_info.extent.width = context.GetWindowSize().width;
        create_info.extent.height = context.GetWindowSize().height;
        create_info.extent.depth = 1;
        create_info.mipLevels = 1;
        create_info.arrayLayers = 1;
        create_info.format = vk::Format::eD32Sfloat;
        create_info.tiling = vk::ImageTiling::eOptimal;
        create_info.initialLayout = vk::ImageLayout::eUndefined;
        create_info.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
        create_info.samples = vk::SampleCountFlagBits::e1;
        create_info.sharingMode = vk::SharingMode::eExclusive;
        vma::AllocationCreateInfo alloc_create_info;
        alloc_create_info.usage = vma::MemoryUsage::eAuto;
        alloc_create_info.flags = vma::AllocationCreateFlagBits::eDedicatedMemory;
        alloc_create_info.priority = 1.0f;
        std::tie(depth_image, depth_alloc) = context.allocator.createImage(create_info, alloc_create_info);
    }
    {
        vk::ImageViewCreateInfo create_info;
        create_info.image = depth_image;
        create_info.viewType = vk::ImageViewType::e2D;
        create_info.format = vk::Format::eD32Sfloat;
        create_info.components.r = vk::ComponentSwizzle::eIdentity;
        create_info.components.g = vk::ComponentSwizzle::eIdentity;
        create_info.components.b = vk::ComponentSwizzle::eIdentity;
        create_info.components.a = vk::ComponentSwizzle::eIdentity;
        create_info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;
        depth_image_view = context.device.createImageView(create_info);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Create the renderpass for blitting an offscreen image to the swapchain
    ///////////////////////////////////////////////////////////////////////////
    vk::AttachmentDescription colorAttachment{};
    colorAttachment.format = context.required_surface_format.format;
    colorAttachment.samples = vk::SampleCountFlagBits::e1;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
    colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::AttachmentDescription depthAttachment{};
    depthAttachment.format = vk::Format::eD32Sfloat;
    depthAttachment.samples = vk::SampleCountFlagBits::e1;
    depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
    depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::AttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::SubpassDescription subpass{};
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    vk::SubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask =
        vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
    dependency.srcAccessMask = vk::AccessFlagBits::eNoneKHR;
    dependency.dstStageMask =
        vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
    dependency.dstAccessMask =
        vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

    std::array<vk::AttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
    vk::RenderPassCreateInfo renderPassInfo{};
    renderPassInfo.attachmentCount = (uint32_t)attachments.size();
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;
    render_pass = context.device.createRenderPass(renderPassInfo);

    ///////////////////////////////////////////////////////////////////////////
    // Create the framebuffers
    ///////////////////////////////////////////////////////////////////////////
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        std::array<vk::ImageView, 2> attachments = {image_views[i], depth_image_view};
        vk::FramebufferCreateInfo framebufferInfo{};
        framebufferInfo.renderPass = render_pass;
        framebufferInfo.attachmentCount = (uint32_t)attachments.size();
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = extent.width;
        framebufferInfo.height = extent.height;
        framebufferInfo.layers = 1;
        framebuffers[i] = context.device.createFramebuffer(framebufferInfo);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Create sync objects
    ///////////////////////////////////////////////////////////////////////////
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        image_available_semaphores[i] = context.device.createSemaphore({});
        render_finished_semaphores[i] = context.device.createSemaphore({});
        vk::FenceCreateInfo fence_create_info{};
        fence_create_info.flags = vk::FenceCreateFlagBits::eSignaled;
        in_flight_fences[i] = context.device.createFence(fence_create_info);
        images_in_flight[i] = vk::Fence();
    }

}
void SwapChain::Destroy()
{
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        context.device.destroySemaphore(image_available_semaphores[i]);
        context.device.destroySemaphore(render_finished_semaphores[i]);
        context.device.destroyFence(in_flight_fences[i]);
    }
    for (auto& framebuffer : framebuffers)
        context.device.destroyFramebuffer(framebuffer); 
    context.device.destroyRenderPass(render_pass);
    for (auto& v : image_views)
        context.device.destroyImageView(v);
    context.allocator.destroyImage(depth_image, depth_alloc);
    context.device.destroyImageView(depth_image_view);
    context.device.destroySwapchainKHR(swapchain);
}

void SwapChain::ReCreate()
{
    std::cout << "ReCreate()\n";
    ///////////////////////////////////////////////////////////////////////
    // Block while minimized
    ///////////////////////////////////////////////////////////////////////
    while (context.GetWindowSize() == vk::Extent2D{0, 0})
        glfwWaitEvents();
    ///////////////////////////////////////////////////////////////////////
    // Recreate Swap Chain
    ///////////////////////////////////////////////////////////////////////
    context.device.waitIdle();
    Destroy();
    Create();
}

void SwapChain::BeginRenderPass(vk::CommandBuffer& command_buffer, uint32_t image_index, glm::vec4 clear_color) 
{
    vk::RenderPassBeginInfo render_pass_info;
    render_pass_info.renderPass = render_pass;
    render_pass_info.framebuffer = framebuffers[image_index];
    render_pass_info.renderArea.offset = vk::Offset2D({0, 0});
    render_pass_info.renderArea.extent = extent;
    vk::ClearValue clear_values[2]{};
    vk::ClearColorValue v{};
    v.setFloat32({clear_color.x, clear_color.y, clear_color.z, clear_color.w});
    clear_values[0].color = v;
    clear_values[1].depthStencil.depth = 1.0f;
    clear_values[1].depthStencil.stencil = 0;
    render_pass_info.clearValueCount = 2;
    render_pass_info.pClearValues = &clear_values[0];
    command_buffer.beginRenderPass(render_pass_info, vk::SubpassContents::eInline);
}

void SwapChain::EndRenderPassAndSubmit(vk::CommandBuffer& command_buffer, uint32_t image_index)
{
    command_buffer.endRenderPass();
    command_buffer.end();
    vk::SubmitInfo submit_info;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &image_available_semaphores[current];
    vk::PipelineStageFlags wait_stages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;
    submit_info.signalSemaphoreCount = 1;
    // Signal the semaphore for the acquired image to avoid reusing semaphores across images
    submit_info.pSignalSemaphores = &render_finished_semaphores[image_index];
    context.graphics_queue.submit({submit_info}, in_flight_fences[current]);
}

uint32_t SwapChain::Swap() 
{
    // Wait for the fence for this frame index to ensure its semaphores/fences are free to reuse
    context.device.waitForFences({in_flight_fences[current]}, true, UINT64_MAX);

    uint32_t next_image; 
    auto swap_result = context.device.acquireNextImageKHR(
        swapchain, UINT64_MAX, image_available_semaphores[current], nullptr, &next_image);
    if (swap_result == vk::Result::eErrorOutOfDateKHR)
    {
        ReCreate();
        return UINT32_MAX; 
    }

    // If another frame is already using this image, wait for its fence
    if (images_in_flight[next_image])
        context.device.waitForFences({images_in_flight[next_image]}, true, UINT64_MAX);

    // Mark the image as now being in use by this frame
    images_in_flight[next_image] = in_flight_fences[current];
    context.device.resetFences({in_flight_fences[current]});
    current_image_index = next_image;
    return next_image;
}

void SwapChain::Present(bool framebuffer_resized, uint32_t image_index) 
{
    vk::PresentInfoKHR presentInfo{};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &render_finished_semaphores[image_index];
    vk::SwapchainKHR swap_chains[] = {swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swap_chains;
    presentInfo.pImageIndices = (uint32_t*)&image_index;
    try
    {
        auto present_result = context.graphics_queue.presentKHR(presentInfo);
        if (present_result == vk::Result::eErrorOutOfDateKHR || present_result == vk::Result::eSuboptimalKHR ||
            framebuffer_resized)
        {
            framebuffer_resized = false;
            ReCreate();
        }
    }
    catch (vk::OutOfDateKHRError)
    {
        framebuffer_resized = false;
        ReCreate();
    }
    current = (current + 1) % MAX_FRAMES_IN_FLIGHT;
}
