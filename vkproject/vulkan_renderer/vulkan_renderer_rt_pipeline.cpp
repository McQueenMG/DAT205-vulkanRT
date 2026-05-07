#include "vulkan_rt_renderer.hpp"
#include "vulkan_util_shaders.hpp"
#include <ecs/ECS.h>
#include <vkproject/log.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vkproject/magica.hpp>
#include <vkproject/asset_manager.hpp>

void VulkanRTRenderer::RT_Pipeline::Create()
{
    ///////////////////////////////////////////////////////////////////////////
    // Create shader modules
    ///////////////////////////////////////////////////////////////////////////
    enum StageIndices
    {
        eRaygen,
        eMiss,
        eClosestHit,
        eAnyHit,
        eShaderGroupCount
    };
    std::array<vk::PipelineShaderStageCreateInfo, eShaderGroupCount> stages{};
    vk::PipelineShaderStageCreateInfo stage{};
    stage.pName = "main"; // All the same entry point
    // Raygen
    stage.module = CreateShaderModule(context.device, ReadBinaryFile("shaders/raytrace.rgen.bin"));
    stage.stage = vk::ShaderStageFlagBits::eRaygenKHR;
    stages[eRaygen] = stage;
    // Miss
    stage.module = CreateShaderModule(context.device, ReadBinaryFile("shaders/raytrace.rmiss.bin"));
    stage.stage = vk::ShaderStageFlagBits::eMissKHR;
    stages[eMiss] = stage;
    // Hit Group - Closest Hit
    stage.module = CreateShaderModule(context.device, ReadBinaryFile("shaders/raytrace.rchit.bin"));
    stage.stage = vk::ShaderStageFlagBits::eClosestHitKHR;
    stages[eClosestHit] = stage;
    // Any-Hit (new)
    stage.module = CreateShaderModule(context.device, ReadBinaryFile("shaders/raytrace.rahit.bin"));
    stage.stage = vk::ShaderStageFlagBits::eAnyHitKHR;
    stages[eAnyHit] = stage;

    // Shader groups
    vk::RayTracingShaderGroupCreateInfoKHR group{};
    group.anyHitShader = VK_SHADER_UNUSED_KHR;
    group.closestHitShader = VK_SHADER_UNUSED_KHR;
    group.generalShader = VK_SHADER_UNUSED_KHR;
    group.intersectionShader = VK_SHADER_UNUSED_KHR;
    std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shader_groups;
    group.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
    group.generalShader = eRaygen;
    shader_groups.push_back(group);
    group.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
    group.generalShader = eMiss;
    shader_groups.push_back(group);
    group.type = vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup;
    group.generalShader = VK_SHADER_UNUSED_KHR;
    group.closestHitShader = eClosestHit;
    group.anyHitShader = eAnyHit;
    shader_groups.push_back(group);

    // Push constant: we want to be able to update constants used by the shaders
    vk::PushConstantRange pushConstant{};
    pushConstant.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eAnyHitKHR;
    pushConstant.size = sizeof(PushConstants);

    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstant;

    // Descriptor sets
    std::vector<vk::DescriptorSetLayout> rtDescSetLayouts = {descriptor_set.descriptor_set_layout};
    pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(rtDescSetLayouts.size());
    pipelineLayoutCreateInfo.pSetLayouts = rtDescSetLayouts.data();

    pipeline_layout = context.device.createPipelineLayout(pipelineLayoutCreateInfo);

    // Assemble the shader stages and recursion depth info into the ray tracing pipeline
    vk::RayTracingPipelineCreateInfoKHR rayPipelineInfo{};
    rayPipelineInfo.stageCount = static_cast<uint32_t>(stages.size()); // Stages are shaders
    rayPipelineInfo.pStages = stages.data();

    // In this case, m_rtShaderGroups.size() == 4: we have one raygen group,
    // two miss shader groups, and one hit group.
    rayPipelineInfo.groupCount = static_cast<uint32_t>(shader_groups.size());
    rayPipelineInfo.pGroups = shader_groups.data();

    // The ray tracing process can shoot rays from the camera, and a shadow ray can be shot from the
    // hit points of the camera rays, hence a recursion level of 2. This number should be kept as low
    // as possible for performance reasons. Even recursive ray tracing should be flattened into a loop
    // in the ray generation to avoid deep recursion.
    rayPipelineInfo.maxPipelineRayRecursionDepth = 2; // Ray depth
    rayPipelineInfo.layout = pipeline_layout;

    pipeline = context.device.createRayTracingPipelineKHR({}, {}, rayPipelineInfo).value;

    for (auto &s : stages)
        context.device.destroyShaderModule(s.module);

    ///////////////////////////////////////////////////////////////////////////
    // Create Shader Binding Table
    ///////////////////////////////////////////////////////////////////////////
    uint32_t miss_count{1}, hit_count{1};
    auto handleCount = 1 + miss_count + hit_count;
    uint32_t handle_size = context.raytracing_pipeline_properties.shaderGroupHandleSize;
    // The SBT (buffer) need to have starting groups to be aligned and handles in the group to be aligned.
    auto AlignUp = [](uint32_t x, size_t a)
    { return uint32_t((x + (uint32_t(a) - 1)) & ~uint32_t(a - 1)); };
    auto AlignUp64 = [](uint64_t x, size_t a)
    { return uint64_t((x + (uint64_t(a) - 1)) & ~uint64_t(a - 1)); };
    uint32_t handle_size_aligned = AlignUp(handle_size, context.raytracing_pipeline_properties.shaderGroupHandleAlignment);
    sbt_rgen_region.stride =
        AlignUp(handle_size_aligned, context.raytracing_pipeline_properties.shaderGroupBaseAlignment);
    sbt_rgen_region.size = sbt_rgen_region.stride; // The size member of pRayGenShaderBindingTable must be equal to its stride member
    sbt_miss_region.stride = handle_size_aligned;
    sbt_miss_region.size =
        AlignUp(miss_count * handle_size_aligned, context.raytracing_pipeline_properties.shaderGroupBaseAlignment);
    sbt_hit_region.stride = handle_size_aligned;
    sbt_hit_region.size =
        AlignUp(hit_count * handle_size_aligned, context.raytracing_pipeline_properties.shaderGroupBaseAlignment);

    // Get the shader group handles (Doing this with C vulkan, cause I don't understand the C++ version)
    uint32_t dataSize = handleCount * handle_size;
    auto handles =
        context.device.getRayTracingShaderGroupHandlesKHR<uint8_t>(pipeline, (uint32_t)0, handleCount, dataSize);

    // Allocate a buffer for storing the SBT.
    vk::DeviceSize sbt_size = sbt_rgen_region.size + sbt_miss_region.size + sbt_hit_region.size;
    SBT_buffer =
        buffer_utils.CreateBuffer(sbt_size + context.raytracing_pipeline_properties.shaderGroupBaseAlignment,
                                  vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst |
                                      vk::BufferUsageFlagBits::eShaderBindingTableKHR,
                                  vma::MemoryUsage::eCpuToGpu, vma::AllocationCreateFlagBits::eMapped, true);
    // Find the SBT addresses of each group
    sbt_rgen_region.deviceAddress =
        AlignUp64(SBT_buffer.device_address, context.raytracing_pipeline_properties.shaderGroupBaseAlignment);
    sbt_miss_region.deviceAddress = sbt_rgen_region.deviceAddress + sbt_rgen_region.size;
    sbt_hit_region.deviceAddress = sbt_rgen_region.deviceAddress + sbt_rgen_region.size + sbt_miss_region.size;

    // Helper to retrieve the handle data
    memcpy((uint8_t *)SBT_buffer.host_ptr, handles.data(), handle_size);
    memcpy(((uint8_t *)SBT_buffer.host_ptr) + sbt_rgen_region.size, handles.data() + handle_size, handle_size);
    memcpy(((uint8_t *)SBT_buffer.host_ptr) + sbt_rgen_region.size + sbt_miss_region.size, handles.data() + 2 * handle_size, handle_size);

    ///////////////////////////////////////////////////////////////////////////
    // Allocate a buffer that will keep the lighting information
    ///////////////////////////////////////////////////////////////////////////
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        lights_buffer[i] =
            buffer_utils.CreateBuffer(MAX_NUM_LIGHTS * sizeof(Light),
                                      vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
                                      vma::MemoryUsage::eCpuToGpu, vma::AllocationCreateFlagBits::eMapped, true);
    }
}

void VulkanRTRenderer::RT_Pipeline::Destroy()
{
    for (auto &b : lights_buffer)
        buffer_utils.DestroyBuffer(b);
    buffer_utils.DestroyBuffer(SBT_buffer);
    context.device.destroyPipelineLayout(pipeline_layout);
    context.device.destroyPipeline(pipeline);
}

void VulkanRTRenderer::RT_Pipeline::SetLights(const std::vector<Light> &lights)
{
    num_lights = (uint32_t)lights.size();
    memcpy(lights_buffer[swapchain.current].host_ptr, lights.data(), lights.size() * sizeof(Light));
}

void VulkanRTRenderer::RT_Pipeline::Submit(const glm::mat4 &P, const glm::mat4 &V, vk::Image &render_target)
{
    auto &command_buffer = context.command_buffers[swapchain.current];
    command_buffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, pipeline);
    std::array<vk::DescriptorSet, 1> desc_sets{descriptor_set.descriptor_set[swapchain.current]};
    PushConstants pc;
    static uint32_t frame = 0;
    pc.frame = frame++;
    pc.inv_view = glm::inverse(V);
    pc.inv_proj = inverse(P);
    pc.prev_view_proj = prev_view_proj;
    pc.num_lights = num_lights;
    pc.num_indirect_samples = num_indirect_samples;
    // Storing current PV for next frame
    prev_view_proj = P * V;

    command_buffer.pushConstants<PushConstants>(pipeline_layout, vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eAnyHitKHR, 0, pc);
    command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, pipeline_layout, 0, desc_sets, {});
    command_buffer.traceRaysKHR(sbt_rgen_region, sbt_miss_region, sbt_hit_region, {}, context.GetWindowSize().width,
                                context.GetWindowSize().height, 1);

    vk::ImageMemoryBarrier barrier;
    barrier.image = render_target;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;
    barrier.oldLayout = vk::ImageLayout::eGeneral;
    barrier.newLayout = vk::ImageLayout::eGeneral;
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.srcAccessMask = vk::AccessFlagBits::eMemoryWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eMemoryRead;
    command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eBottomOfPipe, vk::PipelineStageFlagBits::eFragmentShader,
                                   {}, {}, {}, barrier);
}
