#pragma once 
#include "vulkan_renderer_context.hpp"
#include "vulkan_renderer_texture_utils.hpp"
#include "vulkan_renderer_buffer_utils.hpp"
#include "vulkan_renderer_swapchain.hpp"
#include <variant>

struct DescriptorSet
{
    Context& context;
    TextureUtils& texture_utils;
    SwapChain& swapchain;
    DescriptorSet(Context& context, TextureUtils& texture_utils, SwapChain& swapchain)
        : context(context), texture_utils(texture_utils), swapchain(swapchain){};

    std::vector<vk::DescriptorSetLayoutBinding> descriptors;
    std::vector<vk::WriteDescriptorSet> descriptor_writes{};

    void AddDescriptors(uint32_t count, vk::DescriptorType descriptor_type, vk::ShaderStageFlags shader_stages);
    using DescriptorInput = std::variant<vk::AccelerationStructureKHR*, BufferUtils::Buffer*, vk::ImageView*>;
    void Update(const std::vector<DescriptorInput>& input);

    vk::DescriptorPool descriptor_pool;
    vk::DescriptorSetLayout descriptor_set_layout;
    vk::DescriptorSet descriptor_set[MAX_FRAMES_IN_FLIGHT];
    uint32_t current_image_base = 0;
    void Create();
    void Destroy();
};