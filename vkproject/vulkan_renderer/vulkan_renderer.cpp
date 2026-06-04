#include "vulkan_renderer.hpp"
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

void GLFWErrorCallback(int error, const char* description)
{
    LOG(WARNING) << "GLFW Error[" << error << "]: " << description;
}

static void FramebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    auto renderer = reinterpret_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window));
    renderer->framebuffer_resized = true;
}

void VulkanRenderer::Init(int width, int height)
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
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);  // No OpenGL
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    GLFWwindow* glfw_window = glfwCreateWindow(window_size.x, window_size.y, "Vulkan renderer", NULL, NULL);
    glfwSetWindowUserPointer(glfw_window, this);
    glfwSetFramebufferSizeCallback(glfw_window, FramebufferResizeCallback);
    if (!glfw_window) LOG(FATAL) << "GLFW Window failed to open.";

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
    standard_descriptor_set.AddDescriptors(3, vk::DescriptorType::eStorageBuffer,
                                           vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
    standard_descriptor_set.Create();
    standard_pipeline.Create();
    imgui_context.Create(); 
}

void VulkanRenderer::Destroy() 
{ 
    context.device.waitIdle();
    standard_descriptor_set.Destroy();
    standard_pipeline.Destroy();
    imgui_context.Destroy();
    scene_data.Destroy();
    texture_utils.Destroy();
    swapchain.Destroy(); 
    context.Destroy(); 
}

void VulkanRenderer::SetScene(Scene* scene) {
    context.device.waitIdle();
    current_scene = scene;
    scene_data.Destroy(); 
    scene_data.Create(scene);
}

void VulkanRenderer::SetCamera(const glm::vec3& eye, const glm::vec3& target, const glm::vec3& up)
{
    float aspect_ratio = context.GetWindowSize().width / float(context.GetWindowSize().height);
    P = glm::perspective(glm::radians(45.0f), aspect_ratio, 100.0f, 1000.0f);
    V = glm::lookAt(eye, target, up);
}

void VulkanRenderer::NewFrame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void VulkanRenderer::Render()
{
    ///////////////////////////////////////////////////////////////////////
    // Block until frame is finished on GPU
    ///////////////////////////////////////////////////////////////////////
    uint32_t image_index = swapchain.Swap();
    if (image_index == UINT32_MAX)
        return;
    auto& command_buffer = context.command_buffers[swapchain.current];
    command_buffer.reset();

    ///////////////////////////////////////////////////////////////////////////
    // Begin Command Buffer
    ///////////////////////////////////////////////////////////////////////////
    vk::CommandBufferBeginInfo begin_info = {};
    auto result = command_buffer.begin(&begin_info);

    

    standard_descriptor_set.Update({&standard_pipeline.model_uniforms[swapchain.current],
                                    &scene_data.material_index_buffer, &scene_data.material_buffer}, 0);
    swapchain.BeginRenderPass(command_buffer, image_index, glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
    standard_pipeline.Submit(image_index, P, V);
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
        framebuffer_resized = false;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Present to window
    ///////////////////////////////////////////////////////////////////////////
    swapchain.Present(framebuffer_resized, image_index);
}


