#pragma once
#include "vulkan_renderer_context.hpp"
#include <glm/glm.hpp>

struct SwapChain
{
    Context& context;
    SwapChain(Context& context) : context(context) {}
    uint32_t current = 0;  // Index of current frame in flight
    uint32_t current_image_index = 0;
    uint32_t Swap();
    void Present(bool framebuffer_resized, uint32_t image_index);
    vk::SwapchainKHR swapchain;
    std::array<vk::Image, MAX_FRAMES_IN_FLIGHT> images;
    std::array<vk::ImageView, MAX_FRAMES_IN_FLIGHT> image_views;
    vk::Image depth_image;
    vma::Allocation depth_alloc;
    vk::ImageView depth_image_view;
    std::array<vk::Framebuffer, MAX_FRAMES_IN_FLIGHT> framebuffers;
    std::array<vk::Semaphore, MAX_FRAMES_IN_FLIGHT> image_available_semaphores;
    std::array<vk::Semaphore, MAX_FRAMES_IN_FLIGHT> render_finished_semaphores;
    std::array<vk::Fence, MAX_FRAMES_IN_FLIGHT> in_flight_fences;
    std::array<vk::Fence, MAX_FRAMES_IN_FLIGHT> images_in_flight;
    vk::RenderPass render_pass;
    vk::Extent2D extent;
    void Create();
    void Destroy();
    void ReCreate();
    void BeginRenderPass(vk::CommandBuffer& command_buffer, uint32_t image_index, glm::vec4 clear_color);
    void EndRenderPassAndSubmit(vk::CommandBuffer& command_buffer, uint32_t image_index);
};
