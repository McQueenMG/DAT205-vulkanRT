#pragma once
#include "../renderer.hpp" 
#include <map>
#include "vulkan_renderer_context.hpp"
#include "vulkan_renderer_buffer_utils.hpp"
#include "vulkan_renderer_texture_utils.hpp"
#include "vulkan_renderer_swapchain.hpp"
#include "vulkan_renderer_imgui.hpp"
#include "vulkan_renderer_rendertarget.hpp"
#include "vulkan_renderer_descriptor_set.hpp"
#include "vulkan_renderer_scene_data.hpp"

#include <tuple>
#include <variant>
#include <imgui.h>

struct VulkanRenderer : public IRenderer
{
    Scene * current_scene = nullptr; 
    glm::ivec2 window_size; 
    virtual void Init(int width, int height) override; 
    
    bool framebuffer_resized = false;

    Context context;
    BufferUtils buffer_utils{context};
    TextureUtils texture_utils{context};
    SwapChain swapchain{context};
    ImGuiContext imgui_context{context, swapchain};
    SceneData scene_data{context, buffer_utils};
    DescriptorSet standard_descriptor_set{context, texture_utils, swapchain};

    struct StandardPipeline
    {
        Context& context;
        SwapChain& swapchain;
        DescriptorSet& descriptor_set;
        SceneData& scene_data; 
        BufferUtils& buffer_utils; 
        StandardPipeline(Context& context, SwapChain& swapchain, DescriptorSet& descriptor_set, SceneData& scene_data, BufferUtils& buffer_utils)
            : context(context),
              swapchain(swapchain),
              descriptor_set(descriptor_set),
              scene_data(scene_data),
              buffer_utils(buffer_utils)
        {
        }
        vk::PipelineLayout pipeline_layout;
        vk::Pipeline pipeline;
        static const uint32_t MAX_NUM_COMMANDS = 1024 * 1024;
        BufferUtils::Buffer commands[MAX_FRAMES_IN_FLIGHT];
        BufferUtils::Buffer model_uniforms[MAX_FRAMES_IN_FLIGHT];
        void Create();
        void Destroy();
        void Submit(uint32_t image_index, const glm::mat4& P, const glm::mat4& V);
    } standard_pipeline{context, swapchain, standard_descriptor_set, scene_data, buffer_utils};

    virtual void Destroy() override; 
    virtual void SetScene(Scene *scene) override;
    glm::mat4 P, V; 
    virtual void SetCamera(const glm::vec3& eye, const glm::vec3& target, const glm::vec3& up) override;
    virtual void NewFrame() override; 
    virtual void Render() override; 
};

