#pragma once
#include "vulkan_renderer_context.hpp"
#include "vulkan_renderer_swapchain.hpp"
#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_glfw.h>

struct ImGuiContext
{
    Context& context;
    SwapChain& swapchain;
    ImGuiContext(Context& context, SwapChain& swapchain) : context(context), swapchain(swapchain) {}
    vk::DescriptorPool descriptor_pool;
    void Create();
    void Destroy();
    void Render(vk::CommandBuffer& command_buffer);
};