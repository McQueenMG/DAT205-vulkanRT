#include "vulkan_rt_renderer.hpp"
#include "vulkan_util_shaders.hpp"
#include <fstream>

void VulkanRTRenderer::BlitPipeline::Create() 
{
    vk::ShaderModule vertex_shader_module = CreateShaderModule(context.device, ReadBinaryFile("shaders/blit.vert.bin"));
    vk::PipelineShaderStageCreateInfo vertex_stage_create_info;
    vertex_stage_create_info.stage = vk::ShaderStageFlagBits::eVertex;
    vertex_stage_create_info.module = vertex_shader_module;
    vertex_stage_create_info.pName = "main";

    vk::ShaderModule fragment_shader_module = CreateShaderModule(context.device, ReadBinaryFile("shaders/blit.frag.bin"));
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
    bindingDescription.stride = (3 /*pos*/ + 3 /*normal*/ + 3 /*tangent*/ + 2 /*uv*/) * sizeof(float);
    bindingDescription.inputRate = vk::VertexInputRate::eVertex;
    vertex_input_info.vertexBindingDescriptionCount = 0;
    vertex_input_info.vertexAttributeDescriptionCount = 0;

    ///////////////////////////////////////////////////////////////////////////
    // Input assembly
    ///////////////////////////////////////////////////////////////////////////
    vk::PipelineInputAssemblyStateCreateInfo input_assembly;
    input_assembly.topology = vk::PrimitiveTopology::eTriangleStrip;
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
    // Pipeline Layout
    ///////////////////////////////////////////////////////////////////////////
    vk::PipelineLayoutCreateInfo pipeline_layout_info{};
    std::array<vk::DescriptorSetLayout, 1> layouts = {descriptor_set.descriptor_set_layout};
    pipeline_layout_info.setLayoutCount = (uint32_t)layouts.size();
    pipeline_layout_info.pSetLayouts = layouts.data();  
    pipeline_layout_info.pushConstantRangeCount = 0;     // Optional
    pipeline_layout_info.pPushConstantRanges = nullptr;  // Optional
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
    // Cleanup
    ///////////////////////////////////////////////////////////////////////////
    context.device.destroyShaderModule(vertex_shader_module);
    context.device.destroyShaderModule(fragment_shader_module);
}

void VulkanRTRenderer::BlitPipeline::Destroy() 
{
    context.device.destroyPipeline(pipeline);
    context.device.destroyPipelineLayout(pipeline_layout);
}

void VulkanRTRenderer::BlitPipeline::Submit(uint32_t image_index) 
{
    ///////////////////////////////////////////////////////////////////////
    // Reecord and submit command buffer for blit pass
    ///////////////////////////////////////////////////////////////////////
    auto& command_buffer = context.command_buffers[swapchain.current];
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
    // Bind descriptor
    ///////////////////////////////////////////////////////////////////////
    auto desc_sets = {descriptor_set.descriptor_set[swapchain.current]};
    command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, desc_sets, {});
    
    command_buffer.draw(6, 1, 0, 0);
}
