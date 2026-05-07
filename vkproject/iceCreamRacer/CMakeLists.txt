find_program(GLSLC NAMES glslc)

set(TRI_SRC ${CMAKE_CURRENT_SOURCE_DIR}/iceCreamRacer.cpp)
set(TRI_HDR ${CMAKE_CURRENT_SOURCE_DIR}/iceCreamRacer.hpp)


set(SHADER_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/../vulkan_renderer/shaders/raytrace.rgen
    ${CMAKE_CURRENT_SOURCE_DIR}/../vulkan_renderer/shaders/raytrace.rmiss
    ${CMAKE_CURRENT_SOURCE_DIR}/../vulkan_renderer/shaders/raytrace.rchit
    ${CMAKE_CURRENT_SOURCE_DIR}/../vulkan_renderer/shaders/raytrace.rahit
    ${CMAKE_CURRENT_SOURCE_DIR}/../vulkan_renderer/shaders/blit.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/../vulkan_renderer/shaders/blit.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/../vulkan_renderer/shaders/standard.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/../vulkan_renderer/shaders/standard.frag
)


set(SHADER_BINS)
foreach(SRC IN LISTS SHADER_SRCS)
    get_filename_component(NAME ${SRC} NAME)
    set(BIN ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.bin)
    add_custom_command(
        OUTPUT ${BIN}
        COMMAND ${GLSLC} --target-spv=spv1.5 ${SRC} -o ${BIN}
        DEPENDS ${SRC}
        COMMENT "Compiling shader ${NAME}"
        VERBATIM
    )
    list(APPEND SHADER_BINS ${BIN})
endforeach()

add_executable(iceCreamRacer ${TRI_SRC} ${TRI_HDR})

target_link_libraries(iceCreamRacer
    PRIVATE
        vulkan_renderer
        glm
        ecs
        glfw
    Vulkan::Vulkan
    imgui
    vkproject
)

add_custom_target(shaders ALL DEPENDS ${SHADER_BINS})

target_include_directories(iceCreamRacer
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..                                   # vkproject
    ${CMAKE_CURRENT_SOURCE_DIR}/../..                                # repo root
    ${CMAKE_CURRENT_SOURCE_DIR}/../../external/glfw/include         # glfw
    ${CMAKE_CURRENT_SOURCE_DIR}/../../external/imgui                # imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/../../external/VulkanMemoryAllocator/include/   # VMA C header
    ${CMAKE_CURRENT_SOURCE_DIR}/../../external/VulkanMemoryAllocator-Hpp/include/ # VMA-Hpp
)

add_custom_command(TARGET shaders POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:iceCreamRacer>/shaders
    COMMAND ${CMAKE_COMMAND} -E copy ${SHADER_BINS} $<TARGET_FILE_DIR:iceCreamRacer>/shaders
)
add_dependencies(iceCreamRacer shaders)

set_target_properties(iceCreamRacer PROPERTIES CXX_STANDARD 17)

