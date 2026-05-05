#include "vulkan_renderer.hpp"
#if !RAYTRACE
#include "vulkan_util_shaders.hpp"
#include <fstream>
#include <imgui.h>

struct StandardPushConstants
{
    glm::mat4 view, proj;
};

struct ModelUniforms
{
    glm::mat4 model_matrix;
    uint32_t mat_idx_offset; 
    uint32_t dummy0, dummy1, dummy2; 
};

void VulkanRenderer::StandardPipeline::Create() 
{
    vk::ShaderModule vertex_shader_module = CreateShaderModule(context.device, ReadBinaryFile("shaders/standard.vert.bin"));
    vk::PipelineShaderStageCreateInfo vertex_stage_create_info;
    vertex_stage_create_info.stage = vk::ShaderStageFlagBits::eVertex;
    vertex_stage_create_info.module = vertex_shader_module;
    vertex_stage_create_info.pName = "main";

    vk::ShaderModule fragment_shader_module = CreateShaderModule(context.device, ReadBinaryFile("shaders/standard.frag.bin"));
    vk::PipelineShaderStageCreateInfo fragment_stage_create_info;
    fragment_stage_create_info.stage = vk::ShaderStageFlagBits::eFragment;
    fragment_stage_create_info.module = fragment_shader_module;
    fragment_stage_create_info.pName = "main";

    vk::PipelineShaderStageCreateInfo shader_stages[] = {vertex_stage_create_info, fragment_stage_create_info};

    ///////////////////////////////////////////////////////////////////////////
    // Vertex Input
    ///////////////////////////////////////////////////////////////////////////
    vk::PipelineVertexInputStateCreateInfo vertex_input_info;
    vk::VertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(glm::vec4);
    bindingDescription.inputRate = vk::VertexInputRate::eVertex;
    std::array<vk::VertexInputAttributeDescription, 1> attributeDescriptions{};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = vk::Format::eR32G32B32A32Sfloat;
    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.pVertexBindingDescriptions = &bindingDescription;
    vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertex_input_info.pVertexAttributeDescriptions = attributeDescriptions.data();

    ///////////////////////////////////////////////////////////////////////////
    // Input assembly
    ///////////////////////////////////////////////////////////////////////////
    vk::PipelineInputAssemblyStateCreateInfo input_assembly;
    input_assembly.topology = vk::PrimitiveTopology::eTriangleList;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    ///////////////////////////////////////////////////////////////////////////
    // Viewport and scissors
    ///////////////////////////////////////////////////////////////////////////
    vk::Viewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapchain.extent.width;
    viewport.height = (float)swapchain.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vk::Rect2D scissor{{0, 0}, swapchain.extent};
    vk::PipelineViewportStateCreateInfo viewportState{};
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    ///////////////////////////////////////////////////////////////////////////
    // Rasterizer
    ///////////////////////////////////////////////////////////////////////////
    vk::PipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = vk::PolygonMode::eFill;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = vk::CullModeFlagBits::eNone;
    rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;  // Optional
    rasterizer.depthBiasClamp = 0.0f;           // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f;     // Optional

    ///////////////////////////////////////////////////////////////////////////
    // Multisampling
    ///////////////////////////////////////////////////////////////////////////
    vk::PipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
    multisampling.minSampleShading = 1.0f;           // Optional
    multisampling.pSampleMask = nullptr;             // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE;  // Optional
    multisampling.alphaToOneEnable = VK_FALSE;       // Optional

    ///////////////////////////////////////////////////////////////////////////
    // Color Blending
    ///////////////////////////////////////////////////////////////////////////
    vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                          vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eOne;   // Optional
    colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eZero;  // Optional
    colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;              // Optional
    colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;   // Optional
    colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;  // Optional
    colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;              // Optional

    vk::PipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = vk::LogicOp::eCopy;  // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;  // Optional
    colorBlending.blendConstants[1] = 0.0f;  // Optional
    colorBlending.blendConstants[2] = 0.0f;  // Optional
    colorBlending.blendConstants[3] = 0.0f;  // Optional

    ///////////////////////////////////////////////////////////////////////////
    // Push constants
    ///////////////////////////////////////////////////////////////////////////
    vk::PushConstantRange pushConstant{};
    pushConstant.stageFlags = vk::ShaderStageFlagBits::eVertex;
    pushConstant.size = sizeof(StandardPushConstants);


    ///////////////////////////////////////////////////////////////////////////
    // Pipeline Layout
    ///////////////////////////////////////////////////////////////////////////
   
    vk::PipelineLayoutCreateInfo pipeline_layout_info{};
    std::array<vk::DescriptorSetLayout, 1> layouts = {descriptor_set.descriptor_set_layout};
    pipeline_layout_info.setLayoutCount = (uint32_t)layouts.size();
    pipeline_layout_info.pSetLayouts = layouts.data();  
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &pushConstant;
    pipeline_layout = context.device.createPipelineLayout(pipeline_layout_info);
    
    ///////////////////////////////////////////////////////////////////////////
    // Final pipeline
    ///////////////////////////////////////////////////////////////////////////
    std::vector<vk::DynamicState> dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    vk::PipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    vk::PipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = vk::CompareOp::eLess;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f;  // Optional
    depthStencil.maxDepthBounds = 1.0f;  // Optional
    depthStencil.stencilTestEnable = VK_FALSE;

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
    pipelineInfo.pDynamicState = &dynamicState;  // Optional
    pipelineInfo.layout = pipeline_layout;
    pipelineInfo.renderPass = swapchain.render_pass;
    pipelineInfo.subpass = 0;

    pipeline = context.device.createGraphicsPipeline(nullptr, pipelineInfo).value;

    ///////////////////////////////////////////////////////////////////////////
    // Create a large, mapped, buffer per frame for indirect commands
    ///////////////////////////////////////////////////////////////////////////
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        commands[i] = buffer_utils.CreateBuffer(MAX_NUM_COMMANDS * sizeof(DrawElementsIndirectCommand),
                                                vk::BufferUsageFlagBits::eIndirectBuffer, vma::MemoryUsage::eAuto,
                                                vma::AllocationCreateFlagBits::eHostAccessSequentialWrite, true);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Create a large, mapped, buffer per frame for per object data
    ///////////////////////////////////////////////////////////////////////////
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        model_uniforms[i] = buffer_utils.CreateBuffer(MAX_NUM_COMMANDS * sizeof(ModelUniforms),
                                                vk::BufferUsageFlagBits::eStorageBuffer, vma::MemoryUsage::eAuto,
                                                vma::AllocationCreateFlagBits::eHostAccessSequentialWrite, true);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Cleanup
    ///////////////////////////////////////////////////////////////////////////
    context.device.destroyShaderModule(vertex_shader_module);
    context.device.destroyShaderModule(fragment_shader_module);
}

void VulkanRenderer::StandardPipeline::Destroy()
{
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        buffer_utils.DestroyBuffer(commands[i]);
        buffer_utils.DestroyBuffer(model_uniforms[i]);
    }
    context.device.destroyPipeline(pipeline);
    context.device.destroyPipelineLayout(pipeline_layout);
}

void VulkanRenderer::StandardPipeline::Submit(uint32_t image_index, const glm::mat4& P, const glm::mat4& V)
{
    ///////////////////////////////////////////////////////////////////////
    // Create drawcalls
    ///////////////////////////////////////////////////////////////////////
    //static int ctr = 0; 
    //static int num_commands = 0;
    //if (ctr++ < 2)
    int num_commands = 0;
    {
    DrawElementsIndirectCommand* current_commands = (DrawElementsIndirectCommand*)commands[image_index].host_ptr;
    Scene* current_scene = scene_data.current_scene;
    for (auto& e : current_scene->entity_manager.EntitiesWithComponents<StaticRenderable>())
    {
        glm::mat4 model_matrix = e.GetComponent<StaticRenderable>()->GetModelMatrix();
        auto drawcall_idx = scene_data.vox_asset_drawcall_idx[std::make_pair(
            e.GetComponent<StaticRenderable>()->asset, e.GetComponent<StaticRenderable>()->variation)];
        current_commands[num_commands] = scene_data.drawcalls[drawcall_idx];
        ModelUniforms m{model_matrix};
        m.mat_idx_offset = scene_data.drawcalls[drawcall_idx].first_index / 3; 
        ((ModelUniforms*)(model_uniforms[image_index].host_ptr))[num_commands++] = m;
    }
    for (auto& e : current_scene->entity_manager.EntitiesWithComponents<DynamicRenderable>())
    {
        const auto& dr = e.GetComponent<DynamicRenderable>();
        glm::mat4 model_matrix = dr->GetCurrentAndSetPreviousModelMatrix();
        auto drawcall_idx = scene_data.vox_asset_drawcall_idx[std::make_pair(
            e.GetComponent<DynamicRenderable>()->asset, e.GetComponent<DynamicRenderable>()->variation)];
        current_commands[num_commands] = scene_data.drawcalls[drawcall_idx];
        ModelUniforms m{model_matrix};
        m.mat_idx_offset = scene_data.drawcalls[drawcall_idx].first_index / 3;
        ((ModelUniforms*)(model_uniforms[image_index].host_ptr))[num_commands++] = m;
    }
    }

    ///////////////////////////////////////////////////////////////////////
    // Record and submit command buffer for blit pass
    ///////////////////////////////////////////////////////////////////////
    auto& command_buffer = context.command_buffers[swapchain.current];

    vk::DebugMarkerMarkerInfoEXT marker_info; 
    std::string marker_name = "swapchain" + std::to_string(swapchain.current);
    marker_info.pMarkerName = marker_name.c_str();
    command_buffer.debugMarkerBeginEXT(marker_info);


    command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

    vk::Viewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapchain.extent.width);
    viewport.height = static_cast<float>(swapchain.extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    command_buffer.setViewport(0, {viewport});

    vk::Rect2D scissor = {{0, 0}, swapchain.extent};
    command_buffer.setScissor(0, {scissor});

    vk::DeviceSize offset = 0;


    ///////////////////////////////////////////////////////////////////////
    // Push constants
    ///////////////////////////////////////////////////////////////////////
    StandardPushConstants pc;
    pc.view = V;
    pc.proj = P;
    command_buffer.pushConstants<StandardPushConstants>(
        pipeline_layout, vk::ShaderStageFlagBits::eVertex, 0, pc);

    ///////////////////////////////////////////////////////////////////////
    // Bind descriptor set
    ///////////////////////////////////////////////////////////////////////
    auto desc_sets = {descriptor_set.descriptor_set[swapchain.current]};
    command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, desc_sets, {});
   
    ///////////////////////////////////////////////////////////////////////
    // Bind geometry and draw
    ///////////////////////////////////////////////////////////////////////
    command_buffer.bindVertexBuffers(0, 1, &scene_data.vertex_buffer.buffer, &offset);
    command_buffer.bindIndexBuffer(scene_data.index_buffer.buffer, 0, vk::IndexType::eUint32);
    command_buffer.drawIndexedIndirect(commands[image_index].buffer, 0, num_commands,
                                       sizeof(DrawElementsIndirectCommand));
    command_buffer.debugMarkerEndEXT();
}

#endif // !RAYTRACE