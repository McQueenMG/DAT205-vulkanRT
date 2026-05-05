// In *one* source file:
#define VMA_IMPLEMENTATION
//#define VMA_STATIC_VULKAN_FUNCTIONS 0
//#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1

// If you don't like the `vma::` prefix:
//#define VMA_HPP_NAMESPACE <prefix>
#define VMA_VULKAN_VERSION 1000000  // Vulkan 1.0
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <vk_mem_alloc.hpp>