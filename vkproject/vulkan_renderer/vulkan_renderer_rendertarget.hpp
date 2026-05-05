#pragma once
#include "vulkan_renderer_context.hpp"
#include "vulkan_renderer_texture_utils.hpp"

struct RenderTarget
{
    Context& context;
    TextureUtils& texture_utils;
    const int num_images;
    std::vector<vk::Image> images;
    std::vector<vma::Allocation> image_allocs;
    std::vector<vk::ImageView> image_views;
    RenderTarget(Context& context, TextureUtils& texture_utils, uint32_t num_images)
        : context(context), texture_utils(texture_utils), num_images(num_images)
    {
        images.resize(num_images);
        image_allocs.resize(num_images);
        image_views.resize(num_images);
    }
    void Create();
    void Destroy();
    void ReCreate();
};