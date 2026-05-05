project(VulkanRT)
cmake_minimum_required(VERSION 3.24)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (UNIX)
    set( CMAKE_EXPORT_COMPILE_COMMANDS ON )
    set( CMAKE_CXX_FLAGS_DEBUG
        "-g -std=c++17 -pipe -Wall -Wextra -Wpedantic -Wfatal-errors" )
    set( CMAKE_CXX_FLAGS_RELEASE
        "-std=c++17 -pipe -Ofast -Wfatal-errors -march=native -w" )
endif()
if (WIN32)
	# Required for structured bindings
	add_compile_options("/std:c++latest")
	# Sequrity is for wuzzes
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	# Enable inlining in Debug
	add_compile_options("/Ob1")
endif()

# Find required packages
find_package(Vulkan REQUIRED)
find_package( glm REQUIRED PATHS external/glm-0.9.8.5 NO_DEFAULT_PATH)
find_package( glfw3 REQUIRED PATHS external/glfw/ NO_DEFAULT_PATH )

add_compile_definitions(MAX_FRAMES_IN_FLIGHT=2)

# Add subdirectories
add_subdirectory( ecs )
add_subdirectory(vkproject)  
add_subdirectory(vkproject/vulkan_renderer)



add_library(imgui
    "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui_demo.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui_draw.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui_widgets.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui_tables.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends/imgui_impl_vulkan.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends/imgui_impl_glfw.cpp")
target_link_libraries( imgui
    PRIVATE
        Vulkan::Vulkan 
)

target_include_directories( imgui
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/"
        "${CMAKE_CURRENT_SOURCE_DIR}/external/glfw/include"
)

