#include "vulkan_renderer_imgui.hpp"

void ImGuiContext::Create() 
{ 
	// 1: create descriptor pool for IMGUI
    //  the size of the pool is very oversize, but it's copied from imgui demo itself.
    vk::DescriptorPoolSize pool_sizes[] = {{vk::DescriptorType::eCombinedImageSampler, 1000},
                                           {vk::DescriptorType::eSampler, 1000},
                                           {vk::DescriptorType::eSampledImage, 1000},
                                           {vk::DescriptorType::eStorageImage, 1000},
                                           {vk::DescriptorType::eUniformTexelBuffer, 1000},
                                           {vk::DescriptorType::eStorageTexelBuffer, 1000},
                                           {vk::DescriptorType::eUniformBuffer, 1000},
                                           {vk::DescriptorType::eStorageBuffer, 1000},
                                           {vk::DescriptorType::eUniformBufferDynamic, 1000},
                                           {vk::DescriptorType::eStorageBufferDynamic, 1000},
                                           {vk::DescriptorType::eInputAttachment, 1000}};
    vk::DescriptorPoolCreateInfo pool_info = {};
    pool_info.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
    pool_info.maxSets = 1000;
    pool_info.poolSizeCount = (uint32_t)std::size(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;
    descriptor_pool = context.device.createDescriptorPool(pool_info);

    // 2: initialize imgui library

    // this initializes the core structures of imgui
    ImGui::CreateContext();

    // this initializes imgui for SDL
    ImGui_ImplGlfw_InitForVulkan(context.glfw_window, true);

    // this initializes imgui for Vulkan
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = context.instance;
    init_info.PhysicalDevice = context.physical_device;
    init_info.Device = context.device;
    init_info.Queue = context.graphics_queue;
    init_info.DescriptorPool = descriptor_pool;
    init_info.MinImageCount = MAX_FRAMES_IN_FLIGHT;
    init_info.ImageCount = MAX_FRAMES_IN_FLIGHT;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    ImGui_ImplVulkan_Init(&init_info, swapchain.render_pass);

    // execute a gpu command to upload imgui font textures
    auto command_buffer = context.BeginSingleTimeCommands();
    ImGui_ImplVulkan_CreateFontsTexture(command_buffer);
    context.EndSingleTimeCommands(command_buffer);

    // clear font textures from cpu data
    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void ImGuiContext::Destroy()
{
    context.device.destroyDescriptorPool(descriptor_pool);
    ImGui_ImplVulkan_Shutdown();
}

void ImGuiContext::Render(vk::CommandBuffer& command_buffer) 
{
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    ImGui_ImplVulkan_RenderDrawData(draw_data, command_buffer);
}