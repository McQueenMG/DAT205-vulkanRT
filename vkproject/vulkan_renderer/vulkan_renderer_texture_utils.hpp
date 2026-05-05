#pragma once
#include "vulkan_renderer_context.hpp"

struct TextureUtils
{
    Context& context;
    TextureUtils(Context& context) : context(context) {}
    vk::Sampler default_sampler;
    void TransitionImageLayout(vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
    void CopyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height);
    void Create();
    void Destroy();
};