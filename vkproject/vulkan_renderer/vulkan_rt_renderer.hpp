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

#include "vulkan_renderer_acceleration_structure.hpp"
#include "shaders/common.glsl"


struct VulkanRTRenderer : public IRenderer
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
    SceneData scene_data{context, buffer_utils, texture_utils};
    
    DescriptorSet rt_descriptor_set{context, texture_utils, swapchain};
    DescriptorSet blit_descriptor_set{context, texture_utils, swapchain};
    RenderTarget rt_rendertarget{context, texture_utils, 8};
    AccelerationStructure acceleration_structure{context, buffer_utils, scene_data};

    struct BlitPipeline
    {
        Context& context;
        SwapChain& swapchain;
        DescriptorSet& descriptor_set;
        BlitPipeline(Context& context, SwapChain& swapchain, DescriptorSet& descriptor_set)
            : context(context), swapchain(swapchain), descriptor_set(descriptor_set)
        {
        }
        vk::PipelineLayout pipeline_layout;
        vk::Pipeline pipeline;
        void Create();
        void Destroy();
        void Submit(uint32_t image_index);
    } blit_pipeline{context, swapchain, blit_descriptor_set};

    struct RT_Pipeline
    {
        Context& context;
        DescriptorSet& descriptor_set; 
        RenderTarget& render_target;
        BufferUtils& buffer_utils; 
        SwapChain& swapchain;
        AccelerationStructure& acceleration_structure; 
        RT_Pipeline(Context& context, DescriptorSet& descriptor_set, RenderTarget& render_target,
                    BufferUtils& buffer_utils, SwapChain &swapchain, AccelerationStructure& acceleration_structure)
            : context(context),
              descriptor_set(descriptor_set),
              render_target(render_target),
              buffer_utils(buffer_utils),
              swapchain(swapchain),
              acceleration_structure(acceleration_structure)
        {
        }
        vk::PipelineLayout pipeline_layout;
        vk::Pipeline pipeline; 
        BufferUtils::Buffer SBT_buffer; 
        vk::StridedDeviceAddressRegionKHR sbt_rgen_region{};
        vk::StridedDeviceAddressRegionKHR sbt_miss_region{};
        vk::StridedDeviceAddressRegionKHR sbt_hit_region{};
        void Create();
        void Destroy();
        int num_indirect_samples = 0; 
        bool taa_blend = true;
        bool enable_reflections = true;
        int max_lights = 4;
        float jitter_factor = 0.1f;
        #define MAX_NUM_LIGHTS 1000
        uint32_t num_lights; 
        BufferUtils::Buffer lights_buffer[MAX_FRAMES_IN_FLIGHT]; 
        void SetLights(const std::vector<Light> & lights);
        glm::mat4 prev_view_proj; 
        void Submit(const glm::mat4& P, const glm::mat4& V, vk::Image & render_target); 
    } rt_pipeline{context, rt_descriptor_set, rt_rendertarget, buffer_utils, swapchain, acceleration_structure};    

    virtual void Destroy() override; 
    virtual void SetScene(Scene *scene) override;
    glm::mat4 P, V; 
    virtual void SetCamera(const glm::vec3& eye, const glm::vec3& target, const glm::vec3& up) override;
    virtual void NewFrame() override; 
    virtual void Render() override; 
};

