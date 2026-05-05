#include "vulkan_renderer_descriptor_set.hpp"
#include <map>
#include <variant>

void DescriptorSet::AddDescriptors(uint32_t count, vk::DescriptorType descriptor_type,
                                                   vk::ShaderStageFlags shader_stages)
{
    for (uint32_t i = 0; i < count; i++)
    {
        vk::DescriptorSetLayoutBinding binding{};
        binding.descriptorType = descriptor_type;
        binding.descriptorCount = 1;
        binding.stageFlags = shader_stages;
        binding.binding = (uint32_t)descriptors.size();
        descriptors.push_back(binding);
    }
}

void DescriptorSet::Create()
{
    ///////////////////////////////////////////////////////////////////////////
    // Create Pool
    ///////////////////////////////////////////////////////////////////////////
    std::map<vk::DescriptorType, uint32_t> counts;
    for (auto& d : descriptors)
        counts[d.descriptorType] += 1;
    std::vector<vk::DescriptorPoolSize> pool_sizes(counts.size());
    int ctr = 0;
    for (auto& c : counts)
    {
        pool_sizes[ctr].type = c.first;
        pool_sizes[ctr].descriptorCount = c.second * MAX_FRAMES_IN_FLIGHT; 
        ctr += 1;
    }
    vk::DescriptorPoolCreateInfo poolInfo{};
    poolInfo.poolSizeCount = (uint32_t)pool_sizes.size();
    poolInfo.pPoolSizes = pool_sizes.data();
    poolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;
    poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind;
    descriptor_pool = context.device.createDescriptorPool(poolInfo);

    ///////////////////////////////////////////////////////////////////////////
    // Descriptor Set Layout
    ///////////////////////////////////////////////////////////////////////////
    vk::DescriptorSetLayoutCreateInfo layout_info{};
    layout_info.bindingCount = (uint32_t)descriptors.size();
    layout_info.pBindings = descriptors.data();
    layout_info.flags = {};
    descriptor_set_layout = context.device.createDescriptorSetLayout(layout_info);
    vk::DescriptorSetAllocateInfo alloc_info{};
    alloc_info.descriptorPool = descriptor_pool;
    alloc_info.descriptorSetCount = 2;
    std::array<vk::DescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts;
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        layouts[i] = descriptor_set_layout; 
    alloc_info.pSetLayouts = layouts.data();
    auto allocated = context.device.allocateDescriptorSets(alloc_info);
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        descriptor_set[i] = allocated[i];

    ///////////////////////////////////////////////////////////////////////////
    // Allocate all the memory we will need for updating
    ///////////////////////////////////////////////////////////////////////////
    descriptor_writes.resize(descriptors.size());
}

void DescriptorSet::Update(const std::vector<DescriptorInput>& input_data)
{
    using DescriptorData =
        std::variant<vk::WriteDescriptorSetAccelerationStructureKHR, vk::DescriptorBufferInfo, vk::DescriptorImageInfo>;
    std::vector<DescriptorData> descriptor_data(descriptors.size());
    for (int i = 0; i < descriptors.size(); i++)
    {
        descriptor_writes[i].dstSet = descriptor_set[swapchain.current];
        descriptor_writes[i].dstBinding = i;
        descriptor_writes[i].dstArrayElement = 0;
        descriptor_writes[i].descriptorType = descriptors[i].descriptorType;
        descriptor_writes[i].descriptorCount = 1;
        switch (descriptors[i].descriptorType)
        {
            case vk::DescriptorType::eAccelerationStructureKHR:
            {
                descriptor_data[i] = vk::WriteDescriptorSetAccelerationStructureKHR();
                auto* wdsas = &std::get<vk::WriteDescriptorSetAccelerationStructureKHR>(descriptor_data[i]);
                wdsas->accelerationStructureCount = 1;
                wdsas->pAccelerationStructures = std::get<vk::AccelerationStructureKHR*>(input_data[i]);
                descriptor_writes[i].pNext = wdsas;
                break;
            };
            case vk::DescriptorType::eStorageBuffer:
            {
                descriptor_data[i] = vk::DescriptorBufferInfo();
                auto* buffer_info = &std::get<vk::DescriptorBufferInfo>(descriptor_data[i]);
                auto& buffer = std::get<BufferUtils::Buffer*>(input_data[i]);
                buffer_info->buffer = buffer->buffer;
                buffer_info->range = buffer->size;
                descriptor_writes[i].pBufferInfo = buffer_info;
                break;
            };
            case vk::DescriptorType::eCombinedImageSampler:
            case vk::DescriptorType::eStorageImage:
            {
                descriptor_data[i] = vk::DescriptorImageInfo();
                auto* image_info = &std::get<vk::DescriptorImageInfo>(descriptor_data[i]);
                auto& image_view = std::get<vk::ImageView*>(input_data[i]);
                image_info->imageLayout = vk::ImageLayout::eGeneral;
                image_info->imageView = *image_view;
                image_info->sampler = descriptors[i].descriptorType == vk::DescriptorType::eCombinedImageSampler
                                          ? texture_utils.default_sampler
                                          : VK_NULL_HANDLE;
                descriptor_writes[i].pImageInfo = image_info;
                break;
            };
        }
    }
    context.device.updateDescriptorSets((uint32_t)descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);
}

void DescriptorSet::Destroy()
{
    context.device.destroyDescriptorSetLayout(descriptor_set_layout);
    context.device.destroyDescriptorPool(descriptor_pool);
}
