#include "vulkan_renderer_context.hpp"
#include <vkproject/log.hpp>
#include <vkproject/magica.hpp>
#include <vkproject/asset_manager.hpp>
#include <ecs/ECS.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                              VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                              const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                              void* pUserData)
{
    std::string severity; 
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) severity += "V";
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) severity += "I";
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) severity += "W";
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) severity += "E";
    std::string type;
    if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) type += "G";
    if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) type += "V";
    if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) type += "P";

    if (messageSeverity > VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
    {
        std::cout << severity << " : " << type << ">" << pCallbackData->pMessage << std::endl;
        __debugbreak();
    }
    return false;
}

void Context::Create(GLFWwindow* _glfw_window)
{
    glfw_window = _glfw_window; 
    ///////////////////////////////////////////////////////////////////////////
    // Create instance
    ///////////////////////////////////////////////////////////////////////////
    {
        // initialize the vk::ApplicationInfo structure
        vk::ApplicationInfo applicationInfo;
        applicationInfo.pApplicationName = "PROJECT_NAME";
        applicationInfo.pEngineName = "<engine_name>";
        applicationInfo.apiVersion = VK_API_VERSION_1_2;
        applicationInfo.applicationVersion = 1; 
        // initialize the vk::InstanceCreateInfo (GLFW knows what extensions are required)
        vk::InstanceCreateInfo instanceCreateInfo;
        instanceCreateInfo.pApplicationInfo = &applicationInfo;
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        std::vector<const char *> instance_extensions; 
        for (uint32_t i = 0; i < glfwExtensionCount; i++)
            instance_extensions.push_back(glfwExtensions[i]);
        instance_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        instance_extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        instanceCreateInfo.enabledExtensionCount = (uint32_t)instance_extensions.size();
        instanceCreateInfo.ppEnabledExtensionNames = instance_extensions.data();

        ///////////////////////////////////////////////////////////////////////////
        // Check for validation layers
        ///////////////////////////////////////////////////////////////////////////
        std::vector<const char*> required_layers;
        required_layers.push_back("VK_LAYER_KHRONOS_validation");
        for (auto& required_layer : required_layers)
        {
            bool found = false;
            for (auto& layer : vk::enumerateInstanceLayerProperties())
                if (strcmp(layer.layerName, required_layer) == 0) found = true;
            if (!found) LOG(FATAL) << "Required layer " << required_layer << " not found.\n";
        }
        instanceCreateInfo.enabledLayerCount = (uint32_t)required_layers.size();
        instanceCreateInfo.ppEnabledLayerNames = required_layers.data();

        // create the Instance
        instance = vk::createInstance(instanceCreateInfo);
    }
    VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);

    ///////////////////////////////////////////////////////////////////////////
    // Create a window surface
    // TODO: This cannot be the prettiest way to do these C/C++ casts
    ///////////////////////////////////////////////////////////////////////////
    VkSurfaceKHR c_surface = surface;
    if (glfwCreateWindowSurface(instance, glfw_window, nullptr, &c_surface) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create window surface!");
    }
    surface = c_surface;

    ///////////////////////////////////////////////////////////////////////////
    // Pick a physical device
    ///////////////////////////////////////////////////////////////////////////
    physical_device_graphics_family = -1;
    {
        // Just picking the first one
        auto a = instance.enumeratePhysicalDevices();

        physical_device = instance.enumeratePhysicalDevices()[0];

        LOG(INFO) << "Available devices:\n";
        for (auto& d : instance.enumeratePhysicalDevices())
        {
            LOG(INFO) << d.getProperties().deviceName << "\n";
        }
        LOG(INFO) << "Using first one (hardcoded). Adjust as necessary.\n";


        // List and choose a queue family. Assuming we will find one with gfx and presentation support.
        int ctr = 0;
        for (auto& p : physical_device.getQueueFamilyProperties())
        {
            std::string flagstring;
            if (p.queueFlags && VK_QUEUE_GRAPHICS_BIT) flagstring += "GFX ";
            if (p.queueFlags && VK_QUEUE_COMPUTE_BIT) flagstring += "COMP ";
            if (p.queueFlags && VK_QUEUE_TRANSFER_BIT) flagstring += "TFR ";
            if (p.queueFlags && VK_QUEUE_SPARSE_BINDING_BIT) flagstring += "SB ";
            if (p.queueFlags && VK_QUEUE_PROTECTED_BIT) flagstring += "PR ";
            if (physical_device.getSurfaceSupportKHR(ctr, surface)) flagstring += "[surface]";
            flagstring += " (" + std::to_string(p.queueCount) + ")";
            if ((p.queueFlags && VK_QUEUE_GRAPHICS_BIT) && physical_device.getSurfaceSupportKHR(ctr, surface))
            {
                physical_device_graphics_family = ctr;
                break;
            }
            ctr++;
        }
        if (physical_device_graphics_family == -1) LOG(FATAL) << "Device has no graphics family with present support.";

        // Ensure the glfw surface has what we need.
        auto surface_capabilities = physical_device.getSurfaceCapabilitiesKHR(surface);
        auto surface_formats = physical_device.getSurfaceFormatsKHR(surface);
        bool found_format = false;
        for (auto& f : surface_formats)
            if (f == required_surface_format) found_format = true;
        if (!found_format) LOG(FATAL) << "Could not find required surface format.";
        auto surface_present_modes = physical_device.getSurfacePresentModesKHR(surface);
        // TODO: If we want triple buffering (VK_PRESENT_MODE_MAILBOX_KHR), we should query for it here.
        //       I'm sticking with VK_PRESENT_MODE_FIFO_KHR, which is guaranteed to be available.
        // TODO: I will, on purpose, ignore the problem with "retina" displays here
        //       (https://vulkan-tutorial.com/en/Drawing_a_triangle/Presentation/Swap_chain)
    }

    ///////////////////////////////////////////////////////////////////////////
    // Create a logical device and get the queue
    ///////////////////////////////////////////////////////////////////////////
    {
        // Make sure our chosen device has the required extensions
        const std::vector<const char*> required_device_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                                                                     VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME,
                                                                     VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
                                                                     VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
                                                                     VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
                                                                     VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
                                                                     VK_EXT_DEBUG_MARKER_EXTENSION_NAME};
        float queuePriority = 1.0f;
        vk::DeviceQueueCreateInfo deviceQueueCreateInfo{};
        deviceQueueCreateInfo.queueFamilyIndex = static_cast<uint32_t>(physical_device_graphics_family);
        deviceQueueCreateInfo.queueCount = 1;
        deviceQueueCreateInfo.pQueuePriorities = &queuePriority;

        vk::DeviceCreateInfo device_create_info = {};  
        device_create_info.setQueueCreateInfos(deviceQueueCreateInfo);
        device_create_info.enabledExtensionCount = (uint32_t)required_device_extensions.size();
        device_create_info.ppEnabledExtensionNames = required_device_extensions.data();
        auto features = physical_device.getFeatures();

        // Enable ALL the available features
        vk::PhysicalDeviceRayTracingPipelineFeaturesKHR raytracing_features{};
        vk::PhysicalDeviceAccelerationStructureFeaturesKHR accel_features{};
        accel_features.pNext = &raytracing_features;
        vk::PhysicalDeviceVulkan12Features vulkan12features{};
        vulkan12features.pNext = &accel_features;
        vk::PhysicalDeviceFeatures2 physical_device_features_2{};
        physical_device_features_2.pNext = &vulkan12features;
        physical_device.getFeatures2(&physical_device_features_2);
        device_create_info.pEnabledFeatures = &features;
        device_create_info.pNext = &vulkan12features;

        device = physical_device.createDevice(device_create_info);
        graphics_queue = device.getQueue(physical_device_graphics_family, 0);
    }
    VULKAN_HPP_DEFAULT_DISPATCHER.init(device);

    ///////////////////////////////////////////////////////////////////////////
    // Set debug callback
    ///////////////////////////////////////////////////////////////////////////
    vk::DebugUtilsMessengerCreateInfoEXT create_info{}; 
    create_info.messageSeverity =
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning;
    create_info.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                              vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                              vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
    create_info.pfnUserCallback = reinterpret_cast<vk::PFN_DebugUtilsMessengerCallbackEXT>(debug_callback); 
    debug_utils_messenger = instance.createDebugUtilsMessengerEXT(create_info);

    ///////////////////////////////////////////////////////////////////////
    // Get raytracing properties from device.
    ///////////////////////////////////////////////////////////////////////
    auto properties =
        physical_device.getProperties2<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceRayTracingPipelinePropertiesKHR,
                                       vk::PhysicalDeviceAccelerationStructurePropertiesKHR>();
    raytracing_pipeline_properties = properties.get<vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();
    acceleration_structure_properties = properties.get<vk::PhysicalDeviceAccelerationStructurePropertiesKHR>();




    ///////////////////////////////////////////////////////////////////////////
    // Create command buffers
    ///////////////////////////////////////////////////////////////////////////
    vk::CommandPoolCreateInfo poolInfo{};
    poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    poolInfo.queueFamilyIndex = physical_device_graphics_family;
    command_pool = device.createCommandPool(poolInfo);
    vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.commandPool = command_pool;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;
    auto buffers = device.allocateCommandBuffers(allocInfo);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        command_buffers[i] = buffers[i];   

    ///////////////////////////////////////////////////////////////////////////
    // Initialize VMA
    ///////////////////////////////////////////////////////////////////////////
    {
        // TODO: There is probably some good reason to fix this so it uses > Vulkan 1.0
        // vma::VulkanFunctions vulkanFunctions = {};
        // vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
        // vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;
        vma::AllocatorCreateInfo info = {};
        info.vulkanApiVersion = VK_API_VERSION_1_0;
        info.device = device;
        info.instance = instance;
        info.physicalDevice = physical_device;
        // info.pVulkanFunctions = &vulkanFunctions;
        info.flags = vma::AllocatorCreateFlagBits::eBufferDeviceAddress;
        allocator = vma::createAllocator(info);
    }
}

vk::Extent2D Context::GetWindowSize() 
{
    vk::Extent2D extent; 
    glfwGetFramebufferSize(glfw_window, (int*)&extent.width, (int*)&extent.height);
    return extent; 
}

void Context::Destroy() 
{
    allocator.destroy();
    device.destroyCommandPool(command_pool);
    instance.destroySurfaceKHR(surface);
    device.destroy();
    instance.destroyDebugUtilsMessengerEXT(debug_utils_messenger);
    instance.destroy();
}

vk::CommandBuffer Context::BeginSingleTimeCommands()
{
    vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandPool = command_pool;
    allocInfo.commandBufferCount = 1;
    vk::CommandBuffer commandBuffer;
    commandBuffer = device.allocateCommandBuffers(allocInfo)[0];
    vk::CommandBufferBeginInfo begin_info;
    begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    commandBuffer.begin(begin_info);
    return commandBuffer;
}

void Context::EndSingleTimeCommands(vk::CommandBuffer command_buffer)
{
    command_buffer.end();
    vk::SubmitInfo submitInfo{};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &command_buffer;
    graphics_queue.submit(submitInfo);
    graphics_queue.waitIdle();
    device.freeCommandBuffers(command_pool, {command_buffer});
}
