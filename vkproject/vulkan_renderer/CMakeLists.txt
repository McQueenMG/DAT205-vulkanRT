add_library(vulkan_renderer
    vma_impl.cpp
    vulkan_renderer.hpp
    vulkan_renderer.cpp
    vulkan_renderer_context.hpp
    vulkan_renderer_context.cpp
    vulkan_renderer_swapchain.hpp
    vulkan_renderer_swapchain.cpp
    vulkan_renderer_scene_data.hpp
    vulkan_renderer_scene_data.cpp
    vulkan_util_shaders.hpp
    vulkan_util_shaders.cpp
    vulkan_renderer_descriptor_set.hpp
    vulkan_renderer_descriptor_set.cpp
    vulkan_renderer_rendertarget.hpp
    vulkan_renderer_rendertarget.cpp
    vulkan_renderer_buffer_utils.hpp
    vulkan_renderer_buffer_utils.cpp
    vulkan_renderer_texture_utils.hpp
    vulkan_renderer_texture_utils.cpp
    vulkan_renderer_imgui.hpp
    vulkan_renderer_imgui.cpp
    vulkan_renderer_standard_pipeline.cpp   
    shaders/standard.vert
    shaders/standard.frag
)

target_link_libraries( vulkan_renderer
    PRIVATE
        glm
        ecs
        glfw
        Vulkan::Vulkan 
)

target_include_directories( vulkan_renderer
    PRIVATE
        ../../external/glfw/include
        ../../external/imgui
        ../../external/VulkanMemoryAllocator/include/
        ../../external/VulkanMemoryAllocator-Hpp/include/
)


add_library(vulkan_rt_renderer
    vulkan_renderer_rt_pipeline.cpp
    vulkan_renderer_blit_pipeline.cpp
    vulkan_renderer_acceleration_structure.hpp
    vulkan_renderer_acceleration_structure.cpp
    vulkan_rt_renderer.hpp
    vulkan_rt_renderer.cpp
    shaders/raytrace.rgen
    shaders/raytrace.rchit
    shaders/raytrace.rmiss
    shaders/blit.vert
    shaders/blit.frag
)

target_link_libraries( vulkan_rt_renderer
    PRIVATE
        vulkan_renderer
        glm
        ecs
        glfw
        Vulkan::Vulkan 
)

target_include_directories( vulkan_rt_renderer
    PRIVATE
        ../../external/glfw/include
        ../../external/imgui
        ../../external/VulkanMemoryAllocator/include/
        ../../external/VulkanMemoryAllocator-Hpp/include/
)

add_subdirectory(triangle_demo)
