#include "triangle_demo.hpp"
#include <iostream>
#include "../vulkan_util_shaders.hpp"
#include <imgui.h>

// Push constant structure matching shader layout
struct TrianglePushConstants {
    float aspect_ratio;
    float scale;
};

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
// Provide a definition for the global asset_manager expected by engine code
#include <vkproject/asset_manager.hpp>
AssetManager* asset_manager = nullptr;

int TriangleDemo::Run(int width, int height)
{
    VulkanRenderer renderer;
    renderer.Init(width, height);

    // Create simple triangle vertex buffer
    std::vector<glm::vec4> vertices = {{0.0f, -0.5f, 0.0f, 1.0f}, {0.5f, 0.5f, 0.0f, 1.0f}, {-0.5f, 0.5f, 0.0f, 1.0f}};
    auto vertex_buffer = renderer.buffer_utils.CreateBuffer(vertices,
        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, vma::MemoryUsage::eAuto,
        vma::AllocationCreateFlagBits::eHostAccessSequentialWrite, true);

    // Create pipeline (simple, no descriptors)
    auto vert_bin = ReadBinaryFile("triangle.vert.spv");
    auto frag_bin = ReadBinaryFile("triangle.frag.spv");
    vk::ShaderModule vert_module = CreateShaderModule(renderer.context.device, vert_bin);
    vk::ShaderModule frag_module = CreateShaderModule(renderer.context.device, frag_bin);

    vk::PipelineShaderStageCreateInfo vert_stage{}, frag_stage{};
    vert_stage.stage = vk::ShaderStageFlagBits::eVertex; vert_stage.module = vert_module; vert_stage.pName = "main";
    frag_stage.stage = vk::ShaderStageFlagBits::eFragment; frag_stage.module = frag_module; frag_stage.pName = "main";
    vk::PipelineShaderStageCreateInfo shader_stages[] = {vert_stage, frag_stage};

    // Vertex input (vec4)
    vk::PipelineVertexInputStateCreateInfo vertex_input_info;
    vk::VertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(glm::vec4);
    bindingDescription.inputRate = vk::VertexInputRate::eVertex;
    vk::VertexInputAttributeDescription attributeDescription{};
    attributeDescription.binding = 0;
    attributeDescription.location = 0;
    attributeDescription.format = vk::Format::eR32G32B32A32Sfloat;
    attributeDescription.offset = 0;
    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.pVertexBindingDescriptions = &bindingDescription;
    vertex_input_info.vertexAttributeDescriptionCount = 1;
    vertex_input_info.pVertexAttributeDescriptions = &attributeDescription;

    vk::PipelineInputAssemblyStateCreateInfo input_assembly;
    input_assembly.topology = vk::PrimitiveTopology::eTriangleList;

    vk::Viewport viewport{};
    viewport.minDepth = 0.0f; viewport.maxDepth = 1.0f;
    vk::Rect2D scissor{};
    vk::PipelineViewportStateCreateInfo viewportState{};
    viewportState.viewportCount = 1; viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1; viewportState.pScissors = &scissor;

    vk::PipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.polygonMode = vk::PolygonMode::eFill;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = vk::CullModeFlagBits::eNone;
    rasterizer.frontFace = vk::FrontFace::eCounterClockwise;

    vk::PipelineMultisampleStateCreateInfo multisampling{};
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;

    vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                          vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    vk::PipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.attachmentCount = 1; colorBlending.pAttachments = &colorBlendAttachment;

    vk::PipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = vk::CompareOp::eLess;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;
    depthStencil.stencilTestEnable = VK_FALSE;

    vk::PipelineLayoutCreateInfo pipeline_layout_info{};
    vk::PushConstantRange push_constant_range{};
    push_constant_range.stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
    push_constant_range.offset = 0;
    push_constant_range.size = sizeof(TrianglePushConstants);
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &push_constant_range;
    vk::PipelineLayout pipeline_layout = renderer.context.device.createPipelineLayout(pipeline_layout_info);

    std::vector<vk::DynamicState> dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    vk::PipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    vk::GraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shader_stages;
    pipelineInfo.pVertexInputState = &vertex_input_info;
    pipelineInfo.pInputAssemblyState = &input_assembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipeline_layout;
    pipelineInfo.renderPass = renderer.swapchain.render_pass;
    pipelineInfo.subpass = 0;

    vk::Pipeline pipeline = renderer.context.device.createGraphicsPipeline(nullptr, pipelineInfo).value;

    // Main loop
    GLFWwindow* win = renderer.context.glfw_window;
    static float triangle_scale = 1.0f;
    static float bg_brightness = 0.1f;
    while (!glfwWindowShouldClose(win))
    {
        glfwPollEvents();
        renderer.NewFrame();

        // Simple ImGui demo window
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(300, 150), ImGuiCond_FirstUseEver);
        ImGui::Begin("Triangle Demo");
        ImGui::Text("Vulkan Triangle Renderer");
        ImGui::SliderFloat("Triangle Scale", &triangle_scale, 0.1f, 2.0f);
        ImGui::SliderFloat("Background Brightness", &bg_brightness, 0.0f, 1.0f);
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();

        uint32_t image_index = renderer.swapchain.Swap();
        if (image_index == UINT32_MAX)
            continue;
        auto& command_buffer = renderer.context.command_buffers[renderer.swapchain.current];
        command_buffer.reset();
        vk::CommandBufferBeginInfo begin_info = {};
        command_buffer.begin(&begin_info);

        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)renderer.swapchain.extent.width;
        viewport.height = (float)renderer.swapchain.extent.height;
        scissor.offset = vk::Offset2D{0, 0};
        scissor.extent = renderer.swapchain.extent;

        renderer.swapchain.BeginRenderPass(command_buffer, image_index, glm::vec4(bg_brightness, bg_brightness, bg_brightness, 1.0f));

        command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
        vk::DeviceSize offset = 0;
        command_buffer.bindVertexBuffers(0, 1, &vertex_buffer.buffer, &offset);
        command_buffer.setViewport(0, {viewport});
        command_buffer.setScissor(0, {scissor});
        
        // Push aspect ratio and scale to shader
        float aspect_ratio = (float)renderer.swapchain.extent.width / (float)renderer.swapchain.extent.height;
        TrianglePushConstants pc{aspect_ratio, triangle_scale};
        // std::cout << "Push constants: aspect_ratio=" << pc.aspect_ratio << ", scale=" << pc.scale << std::endl;
        command_buffer.pushConstants(pipeline_layout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof(TrianglePushConstants), &pc);
        
        command_buffer.draw(3, 1, 0, 0);

        // Render ImGui (balances NewFrame called earlier)
        renderer.imgui_context.Render(command_buffer);

        renderer.swapchain.EndRenderPassAndSubmit(command_buffer, image_index);
        renderer.swapchain.Present(false, image_index);
    }

    // Cleanup
    renderer.context.device.waitIdle();
    renderer.context.device.destroyPipeline(pipeline);
    renderer.context.device.destroyPipelineLayout(pipeline_layout);
    renderer.context.device.destroyShaderModule(vert_module);
    renderer.context.device.destroyShaderModule(frag_module);
    renderer.buffer_utils.DestroyBuffer(vertex_buffer);
    renderer.Destroy();
    return 0;
}

int main(int argc, char** argv)
{
    TriangleDemo demo;
    return demo.Run(800, 600);
}
