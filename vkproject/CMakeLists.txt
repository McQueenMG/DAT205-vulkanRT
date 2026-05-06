
add_library(vkproject 
    game.hpp
    game.cpp
    renderer.hpp
    renderer.cpp
    scene.hpp
    scene.cpp
    input.hpp
    glfw_input.hpp
    glfw_input.cpp
    log.hpp
    magica.hpp
    magica.cpp
    asset_manager.hpp
    asset_manager.cpp
    ogt_vox.hpp
    ogt_vox.cpp
    triangle_asset.cpp
    triangle_asset.hpp
    stb_image_impl.cpp
)

# Common libraries
target_link_libraries( vkproject PRIVATE glm ecs glfw )
target_include_directories( vkproject PRIVATE ../external/glfw/include ../external/imgui )

# Renderer dependent libraries
target_link_libraries( vkproject PRIVATE vulkan_renderer)
target_link_libraries( vkproject PRIVATE vulkan_rt_renderer)

add_subdirectory(iceCreamRacer)


