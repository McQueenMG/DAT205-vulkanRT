#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>
#include <vk_mem_alloc.hpp>

struct Context
{
    GLFWwindow* glfw_window = nullptr;
    vk::SurfaceFormatKHR required_surface_format = {vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear};
    vk::Instance instance;
    vk::PhysicalDevice physical_device;
    int physical_device_graphics_family;
    vk::Device device;
    vk::Queue graphics_queue;
    vk::SurfaceKHR surface;
    vma::Allocator allocator;
    vk::DebugUtilsMessengerEXT debug_utils_messenger;
    vk::PhysicalDeviceRayTracingPipelinePropertiesKHR raytracing_pipeline_properties;
    vk::PhysicalDeviceAccelerationStructurePropertiesKHR acceleration_structure_properties;
    void Create(GLFWwindow* glfw_window);
    void Destroy();
    vk::Extent2D GetWindowSize();
    vk::CommandPool command_pool;
    std::array<vk::CommandBuffer, MAX_FRAMES_IN_FLIGHT> command_buffers;
    vk::CommandBuffer BeginSingleTimeCommands();
    void EndSingleTimeCommands(vk::CommandBuffer command_buffer);
};