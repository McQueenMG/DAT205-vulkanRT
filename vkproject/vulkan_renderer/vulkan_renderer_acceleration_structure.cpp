#include "vulkan_renderer_acceleration_structure.hpp"
#include "vulkan_renderer_scene_data.hpp"
#include "vulkan_util_shaders.hpp"
#include <ecs/ECS.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vkproject/renderer.hpp>
#include "shaders/common.glsl"

void AccelerationStructure::CreateBLASes(Scene * _current_scene)
{
    current_scene = _current_scene;
    std::vector<uint32_t> assets = current_scene->GetUsedAssets();
    ///////////////////////////////////////////////////////////////////////////
    // One BLAS per "template" drawcall in SceneData
    ///////////////////////////////////////////////////////////////////////////
    vk::AccelerationStructureGeometryTrianglesDataKHR triangles{};
    triangles.vertexFormat = vk::Format::eR32G32B32Sfloat;  // vec3 vertex position data.
    triangles.vertexData.deviceAddress =
        scene_data.vertex_buffer.device_address;
    triangles.vertexStride = sizeof(glm::vec4);
    triangles.indexType = vk::IndexType::eUint32;
    triangles.indexData.deviceAddress =
        scene_data.index_buffer.device_address;
    triangles.transformData = {};
    triangles.maxVertex =
        uint32_t(scene_data.vertex_buffer.size / sizeof(glm::vec4)) -
        1;  // the highest index of a vertex that will be addressed by a build command using this structure.
    for (auto& drawcall : scene_data.drawcalls)
    {
        // Create geometry
        vk::AccelerationStructureGeometryKHR asGeom{};
        asGeom.geometryType = vk::GeometryTypeKHR::eTriangles;
        asGeom.flags = vk::GeometryFlagBitsKHR::eOpaque;
        asGeom.geometry.triangles = triangles;
        vk::AccelerationStructureBuildRangeInfoKHR offset{};
        offset.primitiveCount = drawcall.index_count / 3;  // The number of triangles
        offset.firstVertex = drawcall.vertex_offset;
        offset.primitiveOffset = drawcall.first_index * sizeof(uint32_t);
        offset.transformOffset = 0;
        // Query required sizes
        vk::AccelerationStructureBuildGeometryInfoKHR buildInfo{};
        buildInfo.type = vk::AccelerationStructureTypeKHR::eBottomLevel;
        buildInfo.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
        buildInfo.mode = vk::BuildAccelerationStructureModeKHR::eBuild;
        buildInfo.geometryCount = 1;
        buildInfo.pGeometries = &asGeom;
        auto build_sizes = context.device.getAccelerationStructureBuildSizesKHR(
            vk::AccelerationStructureBuildTypeKHR::eDevice, buildInfo, (uint32_t)scene_data.index_buffer.size / sizeof(uint32_t));
        // Allocate scratch buffer
        // NOTE: On the 4090, the obtained address was suddenly not aligned properly, so have to do that manually. 
        uint32_t scratch_alignment =
            context.acceleration_structure_properties.minAccelerationStructureScratchOffsetAlignment;
        uint32_t aligned_size = (uint32_t )build_sizes.buildScratchSize + scratch_alignment;

        BufferUtils::Buffer scratch_buffer =
            buffer_utils.CreateBuffer(aligned_size, vk::BufferUsageFlagBits::eStorageBuffer);
        auto blas_buffer = buffer_utils.CreateBuffer(build_sizes.accelerationStructureSize,
                                                     vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR);
        vk::AccelerationStructureCreateInfoKHR createInfo{};
        createInfo.size = build_sizes.accelerationStructureSize;
        createInfo.buffer = blas_buffer.buffer;
        createInfo.type = vk::AccelerationStructureTypeKHR::eBottomLevel;
        auto blas = context.device.createAccelerationStructureKHR(createInfo);
        buildInfo.dstAccelerationStructure = blas;

        buildInfo.scratchData.deviceAddress =
            ((scratch_buffer.device_address / scratch_alignment) + 1) * scratch_alignment;
   
        auto command_buffer = context.BeginSingleTimeCommands();
        command_buffer.buildAccelerationStructuresKHR(buildInfo, &offset);
        vk::MemoryBarrier barrier;


        barrier.srcAccessMask = vk::AccessFlagBits::eAccelerationStructureWriteKHR;
        barrier.dstAccessMask = vk::AccessFlagBits::eAccelerationStructureReadKHR;
        command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
                                       vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR, {}, barrier, {}, {});
        context.EndSingleTimeCommands(command_buffer);
        BLAS_buffers.push_back(blas_buffer);
        BLASes.push_back(blas);
        buffer_utils.DestroyBuffer(scratch_buffer);
    }
}

void AccelerationStructure::DestroyBLASes() 
{ 
    // Should have a nicer way to find uninitialized
    if (BLASes.size() == 0) return; 
    buffer_utils.DestroyBuffer(object_info_buffer);
    for (auto& b : BLAS_buffers)
        buffer_utils.DestroyBuffer(b);
    BLAS_buffers.clear();
    for (auto& b : BLASes)
        context.device.destroyAccelerationStructureKHR(b);
    BLASes.clear();
}

void AccelerationStructure::CreateTLAS(uint32_t swap_idx) 
{
    ///////////////////////////////////////////////////////////////////////
    // Create top level acceleration structure
    ///////////////////////////////////////////////////////////////////////
    auto ToTransform = [](const glm::mat4& M)
    {
        const glm::mat4 T = glm::transpose(M);
        vk::TransformMatrixKHR tfm;
        memcpy(&tfm, glm::value_ptr(T), sizeof(vk::TransformMatrixKHR));
        return tfm;
    };

    const int num_objects = current_scene->entity_manager.CountEntitiesWithComponents<StaticRenderable>() +
                            current_scene->entity_manager.CountEntitiesWithComponents<DynamicRenderable>();

    std::vector<vk::AccelerationStructureInstanceKHR> as_instances(num_objects);
    int ctr = 0; 
    std::vector<ObjectInfo> object_infos(num_objects * 2);
    auto AddInstance = [&](uint32_t blas_idx, glm::mat4 & model_matrix)
    {       
        vk::AccelerationStructureInstanceKHR instance{};
        instance.transform = ToTransform(model_matrix);
        instance.instanceCustomIndex = ctr;
        const auto blas = BLASes[blas_idx];
        vk::AccelerationStructureDeviceAddressInfoKHR address_info; 
        address_info.accelerationStructure = BLASes[blas_idx];
        instance.accelerationStructureReference = context.device.getAccelerationStructureAddressKHR(address_info);

        // WHAT'S HAPPENING BELOW!?  Why suddenly a VK flag?
        //instance.flags =
        //    VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;  // vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable;

        instance.setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleCullDisable);
        instance.mask = 0xFF;
        instance.instanceShaderBindingTableRecordOffset = 0;
        as_instances[ctr++] = instance;
    };    
 
    for (auto& e : current_scene->entity_manager.EntitiesWithComponents<StaticRenderable>())
    {
        glm::mat4 model_matrix = e.GetComponent<StaticRenderable>()->GetModelMatrix();
        auto blas_idx = scene_data.vox_asset_drawcall_idx[std::make_pair(e.GetComponent<StaticRenderable>()->asset, e.GetComponent<StaticRenderable>()->variation)];
        object_infos[ctr].starting_primitive = scene_data.drawcalls[blas_idx].first_index / 3;
        object_infos[ctr].starting_vertex = scene_data.drawcalls[blas_idx].vertex_offset;
        object_infos[ctr].model_matrix = model_matrix; 
        object_infos[ctr].is_static = true; 
        AddInstance(blas_idx, model_matrix);
    }
    for (auto& e : current_scene->entity_manager.EntitiesWithComponents<DynamicRenderable>())
    {
        const auto& dr = e.GetComponent<DynamicRenderable>();
        glm::mat4 model_matrix = dr->GetCurrentAndSetPreviousModelMatrix();
        auto blas_idx = scene_data.vox_asset_drawcall_idx[std::make_pair(e.GetComponent<DynamicRenderable>()->asset, e.GetComponent<DynamicRenderable>()->variation)];
        object_infos[ctr].starting_primitive = scene_data.drawcalls[blas_idx].first_index / 3;
        object_infos[ctr].starting_vertex = scene_data.drawcalls[blas_idx].vertex_offset;
        object_infos[ctr].model_matrix = model_matrix;
        object_infos[ctr].prev_model_matrix = dr->prev_model_matrix;
        object_infos[ctr].is_static = false; 
        AddInstance(blas_idx, model_matrix);
    }

    ///////////////////////////////////////////////////////////////////////////
    // If it isn't already big enough, create an object info buffer twice
    // the size we need (to avoid lots of updates).
    ///////////////////////////////////////////////////////////////////////////

    if (num_objects > (object_info_buffer.size / sizeof(ObjectInfo)))
    {
        if (object_info_buffer.size > 0) buffer_utils.DestroyBuffer(object_info_buffer);
        object_info_buffer = buffer_utils.CreateBuffer(
            object_infos, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst);
    }
    else
    {
        buffer_utils.LoadBuffer(object_info_buffer.buffer, num_objects * sizeof(ObjectInfo), object_infos.data());
    }

    auto command_buffer = context.BeginSingleTimeCommands();
    auto instance_buffer = buffer_utils.CreateBuffer(
        as_instances.size() * sizeof(vk::AccelerationStructureInstanceKHR), as_instances.data(),
        vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eTransferDst |
            vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR);
    vk::BufferDeviceAddressInfo buffer_info{};
    buffer_info.buffer = instance_buffer.buffer;
    // Make sure the copy of the instance buffer are copied before triggering the acceleration structure build
    vk::MemoryBarrier barrier{};
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eAccelerationStructureWriteKHR;
    command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                   vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR, {}, barrier, {}, {});
    vk::AccelerationStructureGeometryInstancesDataKHR instance_data{};
    instance_data.data.deviceAddress = instance_buffer.device_address;
    vk::AccelerationStructureGeometryKHR topASGeometry{};
    topASGeometry.geometryType = vk::GeometryTypeKHR::eInstances;
    topASGeometry.geometry.instances = instance_data;

    // Find sizes
    vk::AccelerationStructureBuildGeometryInfoKHR buildInfo{};
    buildInfo.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
    buildInfo.geometryCount = 1;
    buildInfo.pGeometries = &topASGeometry;
    buildInfo.mode = vk::BuildAccelerationStructureModeKHR::eBuild;
    buildInfo.type = vk::AccelerationStructureTypeKHR::eTopLevel;
    buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;
    vk::AccelerationStructureBuildSizesInfoKHR size_info = context.device.getAccelerationStructureBuildSizesKHR(
        vk::AccelerationStructureBuildTypeKHR::eDevice, buildInfo, (uint32_t)as_instances.size());

    // Create
    vk::AccelerationStructureCreateInfoKHR createInfo{};
    createInfo.type = vk::AccelerationStructureTypeKHR::eTopLevel;
    createInfo.size = size_info.accelerationStructureSize;

    tlas_data[swap_idx].buffer = buffer_utils.CreateBuffer(
        size_info.accelerationStructureSize,
        vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress);
    createInfo.buffer = tlas_data[swap_idx].buffer.buffer;

    tlas_data[swap_idx].TLAS = context.device.createAccelerationStructureKHR(createInfo);

    // Allocate the scratch memory
    // Allocate scratch buffer
    // NOTE: On the 4090, the obtained address was suddenly not aligned properly, so have to do that manually.
    uint32_t scratch_alignment =
        context.acceleration_structure_properties.minAccelerationStructureScratchOffsetAlignment;
    uint32_t aligned_size = (uint32_t)size_info.buildScratchSize + scratch_alignment;

    auto tlas_scratch_buffer = buffer_utils.CreateBuffer(
        aligned_size, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress);

    // Update build information
    buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;  // update ? m_tlas.accel : VK_NULL_HANDLE;
    buildInfo.dstAccelerationStructure = tlas_data[swap_idx].TLAS;

    buildInfo.scratchData.deviceAddress =
        ((tlas_scratch_buffer.device_address / scratch_alignment) + 1) * scratch_alignment;

    // Build Offsets info: n instances
    vk::AccelerationStructureBuildRangeInfoKHR buildOffsetInfo{};
    buildOffsetInfo.primitiveCount = (uint32_t)as_instances.size();
    command_buffer.buildAccelerationStructuresKHR(buildInfo, &buildOffsetInfo);
    context.EndSingleTimeCommands(command_buffer);

    ///////////////////////////////////////////////////////////////////////////
    // Clean up
    ///////////////////////////////////////////////////////////////////////////
    buffer_utils.DestroyBuffer(instance_buffer);
    buffer_utils.DestroyBuffer(tlas_scratch_buffer);

    tlas_data[swap_idx].created = true; 
}

void AccelerationStructure::DestroyTLAS(uint32_t swap_idx) 
{
    if (!tlas_data[swap_idx].created) return; 
    buffer_utils.DestroyBuffer(tlas_data[swap_idx].buffer);

    context.device.destroyAccelerationStructureKHR(tlas_data[swap_idx].TLAS);
}


