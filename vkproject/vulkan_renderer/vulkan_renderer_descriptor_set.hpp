#pragma once 
#include "vulkan_renderer_context.hpp"
#include "vulkan_renderer_texture_utils.hpp"
#include "vulkan_renderer_buffer_utils.hpp"
#include "vulkan_renderer_swapchain.hpp"
#include "vulkan_renderer_descriptor_set.hpp"
#include <variant>
#include <vector>

struct DescriptorSet
{

    // struct DescriptorData
    // {
    //     std::variant<vk::WriteDescriptorSetAccelerationStructureKHR, vk::DescriptorBufferInfo,
    //                  vk::DescriptorImageInfo, std::vector<vk::DescriptorImageInfo>>
    //         data;
    // };


    Context& context;
    TextureUtils& texture_utils;
    SwapChain& swapchain;
    DescriptorSet(Context& context, TextureUtils& texture_utils, SwapChain& swapchain)
        : context(context), texture_utils(texture_utils), swapchain(swapchain){};

    std::vector<vk::DescriptorSetLayoutBinding> descriptors;
    std::vector<vk::ImageLayout> descriptor_image_layouts;
    std::vector<vk::WriteDescriptorSet> descriptor_writes{};
    
    std::vector<std::variant<vk::WriteDescriptorSetAccelerationStructureKHR, vk::DescriptorBufferInfo,
                             vk::DescriptorImageInfo, std::vector<vk::DescriptorImageInfo>>>
        descriptor_data;
    

    void AddDescriptors(uint32_t count, vk::DescriptorType descriptor_type, vk::ShaderStageFlags shader_stages,
                        vk::ImageLayout image_layout = vk::ImageLayout::eGeneral);
    void AddDescriptorArray(uint32_t count, vk::DescriptorType descriptor_type, vk::ShaderStageFlags shader_stages,
                            vk::ImageLayout image_layout = vk::ImageLayout::eGeneral);
    using DescriptorInput =
        std::variant<vk::AccelerationStructureKHR*, BufferUtils::Buffer*, vk::ImageView*, std::vector<vk::ImageView*>>;
    void Update(const std::vector<DescriptorSet::DescriptorInput> &input_data, uint32_t descriptor_start_index);

    vk::DescriptorPool descriptor_pool;
    vk::DescriptorSetLayout descriptor_set_layout;
    vk::DescriptorSet descriptor_set[MAX_FRAMES_IN_FLIGHT];
    uint32_t current_image_base = 0;
    void Create();
    void Destroy();
};