#include "vulkan_rt_renderer.hpp"
#include <ecs/ECS.h>
#include <vkproject/log.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vkproject/magica.hpp>
#include <vkproject/asset_manager.hpp>

///////////////////////////////////////////////////////////////////////////////
// GLFW callbacks
///////////////////////////////////////////////////////////////////////////////

void GLFWErrorCallback(int error, const char *description)
{
    LOG(WARNING) << "GLFW Error[" << error << "]: " << description;
}

static void FramebufferResizeCallback(GLFWwindow *window, int width, int height)
{
    auto renderer = reinterpret_cast<VulkanRTRenderer *>(glfwGetWindowUserPointer(window));
    renderer->framebuffer_resized = true;
}



void VulkanRTRenderer::Init(int width, int height)
{
    window_size = {width, height};
    ///////////////////////////////////////////////////////////////////////////
    // Initialize GLFW
    ///////////////////////////////////////////////////////////////////////////
    glfwSetErrorCallback(GLFWErrorCallback);
    if (!glfwInit())
        LOG(FATAL) << "GLFW Initialization failed";
    else
        LOG(INFO) << "GLFW Initialized";
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // No OpenGL
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    GLFWwindow *glfw_window = glfwCreateWindow(window_size.x, window_size.y, "Vulkan renderer", NULL, NULL);
    glfwSetWindowUserPointer(glfw_window, this);
    glfwSetFramebufferSizeCallback(glfw_window, FramebufferResizeCallback);
    if (!glfw_window)
        LOG(FATAL) << "GLFW Window failed to open.";

    ///////////////////////////////////////////////////////////////////////////
    // Set up address loading for extensions
    ///////////////////////////////////////////////////////////////////////////
    VULKAN_HPP_DEFAULT_DISPATCHER.init(::vkGetInstanceProcAddr);

    ///////////////////////////////////////////////////////////////////////////
    // Initialize Vulkan
    ///////////////////////////////////////////////////////////////////////////
    context.Create(glfw_window);
    swapchain.Create();
    texture_utils.Create();

    // rt_rendertarget.Create();
    // blit_descriptor_set.AddDescriptors(1, vk::DescriptorType::eCombinedImageSampler,
    //                                    vk::ShaderStageFlagBits::eFragment);
    // blit_descriptor_set.Create();
    // blit_pipeline.Create();

    // // Binding 0
    // rt_descriptor_set.AddDescriptors(1, vk::DescriptorType::eAccelerationStructureKHR,
    //                                  vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eAnyHitKHR);
    
    // // Bindings 1, 2, 3, 4, 5
    // // Reduced from 6 to 5 because Materials is skipped here to preserve explicit mapping
    // rt_descriptor_set.AddDescriptors(5, vk::DescriptorType::eStorageBuffer,
    //                                  vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eAnyHitKHR);
    
    // // Binding 6: Materials Buffer
    // rt_descriptor_set.AddDescriptors(1, vk::DescriptorType::eStorageBuffer,
    //                                  vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eAnyHitKHR);
    
    // // Binding 7: diffuse textures array (256 slots)
    // rt_descriptor_set.AddDescriptorArray(256, vk::DescriptorType::eCombinedImageSampler,
    //                                      vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eAnyHitKHR,
    //                                      vk::ImageLayout::eShaderReadOnlyOptimal);
    
    // // Binding 8: roughness textures array (256 slots)
    // rt_descriptor_set.AddDescriptorArray(256, vk::DescriptorType::eCombinedImageSampler,
    //                                      vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eAnyHitKHR,
    //                                      vk::ImageLayout::eShaderReadOnlyOptimal);
    
    // // Binding 9: metalness textures array (256 slots)
    // rt_descriptor_set.AddDescriptorArray(256, vk::DescriptorType::eCombinedImageSampler,
    //                                      vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eAnyHitKHR,
    //                                      vk::ImageLayout::eShaderReadOnlyOptimal);
    
    // // Binding 10: normal textures array (256 slots)
    // rt_descriptor_set.AddDescriptorArray(256, vk::DescriptorType::eCombinedImageSampler,
    //                                      vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eAnyHitKHR,
    //                                      vk::ImageLayout::eShaderReadOnlyOptimal);

    // // Binding 11: out_final_image
    // rt_descriptor_set.AddDescriptors(1, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eRaygenKHR);
    
    // // Binding 12: in_final_image
    // rt_descriptor_set.AddDescriptors(1, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eRaygenKHR);
    
    // // Binding 13: Lights Buffer
    // rt_descriptor_set.AddDescriptors(1, vk::DescriptorType::eStorageBuffer,
    //                                  vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR);
                            
    // // Binding 14: out_normal_image
    // rt_descriptor_set.AddDescriptors(1, vk::DescriptorType::eStorageImage,
    //                                  vk::ShaderStageFlagBits::eRaygenKHR);
    
    // // Binding 15: in_normal_image
    // rt_descriptor_set.AddDescriptors(1, vk::DescriptorType::eCombinedImageSampler,
    //                                  vk::ShaderStageFlagBits::eRaygenKHR);


    // OLD VERSION
    rt_rendertarget.Create();
    blit_descriptor_set.AddDescriptors(1, vk::DescriptorType::eCombinedImageSampler,
                                       vk::ShaderStageFlagBits::eFragment);
    blit_descriptor_set.Create();
    blit_pipeline.Create();

    rt_descriptor_set.AddDescriptors(1, vk::DescriptorType::eAccelerationStructureKHR,
                                     vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eAnyHitKHR);
    rt_descriptor_set.AddDescriptors(6, vk::DescriptorType::eStorageBuffer,
                                     vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eAnyHitKHR);
    // diffuse textures
    rt_descriptor_set.AddDescriptorArray(256, vk::DescriptorType::eCombinedImageSampler,
                                         vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eAnyHitKHR,
                                         vk::ImageLayout::eShaderReadOnlyOptimal);
    // roughness textures
    rt_descriptor_set.AddDescriptorArray(256, vk::DescriptorType::eCombinedImageSampler,
                                         vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eAnyHitKHR,
                                         vk::ImageLayout::eShaderReadOnlyOptimal);
    // metalness textures
    rt_descriptor_set.AddDescriptorArray(256, vk::DescriptorType::eCombinedImageSampler,
                                         vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eAnyHitKHR,
                                         vk::ImageLayout::eShaderReadOnlyOptimal);
    // normal textures
    rt_descriptor_set.AddDescriptorArray(256, vk::DescriptorType::eCombinedImageSampler,
                                         vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eAnyHitKHR,
                                         vk::ImageLayout::eShaderReadOnlyOptimal);
    rt_descriptor_set.AddDescriptors(1, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eRaygenKHR);
    rt_descriptor_set.AddDescriptors(1, vk::DescriptorType::eCombinedImageSampler,
                                     vk::ShaderStageFlagBits::eRaygenKHR);
    rt_descriptor_set.AddDescriptors(1, vk::DescriptorType::eStorageBuffer,
                                     vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR);
                            
    
    // normal output
    rt_descriptor_set.AddDescriptors(1, vk::DescriptorType::eStorageImage,
                                    vk::ShaderStageFlagBits::eRaygenKHR);
    
    // normal output in previous frame
    rt_descriptor_set.AddDescriptors(1, vk::DescriptorType::eCombinedImageSampler,
                                    vk::ShaderStageFlagBits::eRaygenKHR);

    rt_descriptor_set.Create();
    rt_pipeline.Create();
    imgui_context.Create();
}

void VulkanRTRenderer::Destroy()
{
    context.device.waitIdle();
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        acceleration_structure.DestroyTLAS(i);
    acceleration_structure.DestroyBLASes();
    rt_pipeline.Destroy();
    rt_descriptor_set.Destroy();
    blit_pipeline.Destroy();
    blit_descriptor_set.Destroy();
    rt_rendertarget.Destroy();
    imgui_context.Destroy();
    scene_data.Destroy();
    texture_utils.Destroy();
    swapchain.Destroy();
    context.Destroy();
}

void VulkanRTRenderer::SetScene(Scene *scene)
{
    context.device.waitIdle();
    current_scene = scene;
    m_static_descriptors_dirty = true;
    scene_data.Destroy();
    scene_data.Create(scene);
    acceleration_structure.DestroyBLASes();
    acceleration_structure.CreateBLASes(scene);
}

void VulkanRTRenderer::SetCamera(const glm::vec3 &eye, const glm::vec3 &target, const glm::vec3 &up)
{
    float aspect_ratio = context.GetWindowSize().width / float(context.GetWindowSize().height);
    P = glm::perspective(glm::radians(45.0f), aspect_ratio, 100.0f, 1000.0f);
    V = glm::lookAt(eye, target, up);
}

void VulkanRTRenderer::NewFrame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void VulkanRTRenderer::Render()
{
    ///////////////////////////////////////////////////////////////////////
    // Block until frame is finished on GPU
    ///////////////////////////////////////////////////////////////////////
    acceleration_structure.DestroyTLAS(swapchain.current);

    uint32_t image_index = swapchain.Swap();
    if (image_index == UINT32_MAX)
        return;
    auto &command_buffer = context.command_buffers[swapchain.current];
    command_buffer.reset();

    ImGui::Begin("RT Renderer");
    ImGui::SliderInt("Indirect samples", &rt_pipeline.num_indirect_samples, 0, 3);
    ImGui::Checkbox("TAA enabled", &rt_pipeline.taa_blend);
    ImGui::Checkbox("Reflections", &rt_pipeline.enable_reflections);
    ImGui::SliderInt("Max lights sampled", &rt_pipeline.max_lights, 1, 64);
    ImGui::SliderFloat("Shadow Jitter Factor", &rt_pipeline.jitter_factor, 0.0f, 1.0f);
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::End();

    ///////////////////////////////////////////////////////////////////////////
    // Set lights
    ///////////////////////////////////////////////////////////////////////////
    std::vector<Light> lights;
    for (auto e : current_scene->entity_manager.EntitiesWithComponents<LightComponent>())
    {
        Light l;
        l.position = e.GetComponent<LightComponent>()->position;
        l.intensity = e.GetComponent<LightComponent>()->intensity;
        lights.push_back(l);
    }
    rt_pipeline.SetLights(lights);

    ///////////////////////////////////////////////////////////////////////////
    // Begin Command Buffer
    ///////////////////////////////////////////////////////////////////////////
    vk::CommandBufferBeginInfo begin_info = {};
    auto result = command_buffer.begin(&begin_info);

    acceleration_structure.CreateTLAS(swapchain.current);
    // Build descriptor inputs in order: existing descriptors + material textures
    std::vector<DescriptorSet::DescriptorInput> descriptor_inputs;

    // Existing 10 descriptors
    descriptor_inputs.push_back(&acceleration_structure.tlas_data[swapchain.current].TLAS);
    descriptor_inputs.push_back(&acceleration_structure.object_info_buffer);
    descriptor_inputs.push_back(&scene_data.material_index_buffer);
    descriptor_inputs.push_back(&scene_data.index_buffer);
    descriptor_inputs.push_back(&scene_data.vertex_buffer);
    descriptor_inputs.push_back(&scene_data.uv_buffer);
    descriptor_inputs.push_back(&scene_data.material_buffer);

     auto build_image_view_array = [](std::vector<SceneData::MaterialTextureGPU> &textures) {
        vk::ImageView *fallback_view = textures.empty() ? nullptr : &textures.front().image_view;
        std::vector<vk::ImageView *> views(256, fallback_view);
        for (size_t i = 0; i < textures.size() && i < views.size(); ++i)
        {
            views[i] = &textures[i].image_view;
        }
        return views;
    };

    descriptor_inputs.push_back(build_image_view_array(scene_data.diffuse_textures_gpu));
    descriptor_inputs.push_back(build_image_view_array(scene_data.roughness_textures_gpu));
    descriptor_inputs.push_back(build_image_view_array(scene_data.metalness_textures_gpu));
    descriptor_inputs.push_back(build_image_view_array(scene_data.normal_textures_gpu));

    // needs to be updated every frame
    descriptor_inputs.push_back(&rt_rendertarget.image_views[(rt_descriptor_set.current_image_base + 0) % 2]);
    descriptor_inputs.push_back(&rt_rendertarget.image_views[(rt_descriptor_set.current_image_base + 1) % 2]);
    descriptor_inputs.push_back(&rt_pipeline.lights_buffer[swapchain.current]);
    descriptor_inputs.push_back(&rt_rendertarget.normal_image_views[(rt_descriptor_set.current_image_base + 0) % 2]);
    descriptor_inputs.push_back(&rt_rendertarget.normal_image_views[(rt_descriptor_set.current_image_base + 1) % 2]);

    if (m_static_descriptors_dirty && current_scene != nullptr)
    {

        rt_descriptor_set.Update(descriptor_inputs, 0);

        m_static_descriptors_dirty = false;
    }

    rt_descriptor_set.Update(descriptor_inputs, 11);

    rt_pipeline.Submit(P, V, rt_rendertarget.images[rt_descriptor_set.current_image_base % 2]);

    blit_descriptor_set.Update({&rt_rendertarget.image_views[(rt_descriptor_set.current_image_base + 0) % 2]}, 0);
    swapchain.BeginRenderPass(command_buffer, image_index, glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
    blit_pipeline.Submit(image_index);
    //rt_descriptor_set.current_image_base = (rt_descriptor_set.current_image_base + 1) % 2;
    imgui_context.Render(command_buffer);

    ///////////////////////////////////////////////////////////////////////////
    // End and Submit Command Buffer
    ///////////////////////////////////////////////////////////////////////////
    swapchain.EndRenderPassAndSubmit(command_buffer, image_index);

    ///////////////////////////////////////////////////////////////////////////
    // Handle reshape
    ///////////////////////////////////////////////////////////////////////////
    if (framebuffer_resized)
    {
        context.device.waitIdle();
        rt_rendertarget.ReCreate();
        framebuffer_resized = false;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Present to window
    ///////////////////////////////////////////////////////////////////////////
    swapchain.Present(framebuffer_resized, image_index);
}
