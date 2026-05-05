find_program(GLSLANGVALIDATOR NAMES glslangValidator)

set(TRI_SRC ${CMAKE_CURRENT_SOURCE_DIR}/triangle_demo.cpp)
set(TRI_HDR ${CMAKE_CURRENT_SOURCE_DIR}/triangle_demo.hpp)

set(VERT_SRC ${CMAKE_CURRENT_SOURCE_DIR}/shaders/triangle.vert)
set(FRAG_SRC ${CMAKE_CURRENT_SOURCE_DIR}/shaders/triangle.frag)

set(VERT_SPV ${CMAKE_CURRENT_BINARY_DIR}/triangle.vert.spv)
set(FRAG_SPV ${CMAKE_CURRENT_BINARY_DIR}/triangle.frag.spv)

if(GLSLANGVALIDATOR)
    add_custom_command(OUTPUT ${VERT_SPV}
        COMMAND ${GLSLANGVALIDATOR} -V -S vert ${VERT_SRC} -o ${VERT_SPV}
        DEPENDS ${VERT_SRC}
        COMMENT "Compiling triangle.vert to SPIR-V"
    )
    add_custom_command(OUTPUT ${FRAG_SPV}
        COMMAND ${GLSLANGVALIDATOR} -V -S frag ${FRAG_SRC} -o ${FRAG_SPV}
        DEPENDS ${FRAG_SRC}
        COMMENT "Compiling triangle.frag to SPIR-V"
    )
    add_custom_target(triangle_shaders DEPENDS ${VERT_SPV} ${FRAG_SPV})
    # Compile engine standard shaders (renderer expects these under "shaders/*.bin")
    set(ENGINE_VERT_SPV ${CMAKE_CURRENT_BINARY_DIR}/standard.vert.bin)
    set(ENGINE_FRAG_SPV ${CMAKE_CURRENT_BINARY_DIR}/standard.frag.bin)
    add_custom_command(OUTPUT ${ENGINE_VERT_SPV}
        COMMAND ${GLSLANGVALIDATOR} -V -S vert ${CMAKE_SOURCE_DIR}/vkproject/vulkan_renderer/shaders/standard.vert -o ${ENGINE_VERT_SPV}
        DEPENDS ${CMAKE_SOURCE_DIR}/vkproject/vulkan_renderer/shaders/standard.vert
        COMMENT "Compiling engine standard.vert to SPIR-V"
    )
    add_custom_command(OUTPUT ${ENGINE_FRAG_SPV}
        COMMAND ${GLSLANGVALIDATOR} -V -S frag ${CMAKE_SOURCE_DIR}/vkproject/vulkan_renderer/shaders/standard.frag -o ${ENGINE_FRAG_SPV}
        DEPENDS ${CMAKE_SOURCE_DIR}/vkproject/vulkan_renderer/shaders/standard.frag
        COMMENT "Compiling engine standard.frag to SPIR-V"
    )
    add_custom_target(engine_standard_shaders DEPENDS ${ENGINE_VERT_SPV} ${ENGINE_FRAG_SPV})
else()
    message(WARNING "glslangValidator not found; please provide triangle.vert.spv and triangle.frag.spv in ${CMAKE_CURRENT_BINARY_DIR} or build shaders manually.")
endif()

add_executable(triangle_demo ${TRI_SRC} ${TRI_HDR})

target_link_libraries(triangle_demo
    PRIVATE
        vulkan_renderer
        glm
        ecs
        glfw
    Vulkan::Vulkan
    imgui
    vkproject
)

target_include_directories(triangle_demo
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..                                   # vkproject/vulkan_renderer
        ${CMAKE_CURRENT_SOURCE_DIR}/../..                                # vkproject
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../external/glfw/include         # glfw
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../external/imgui                # imgui
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../external/VulkanMemoryAllocator/include/   # VMA C header
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../external/VulkanMemoryAllocator-Hpp/include/ # VMA-Hpp
)

if(GLSLANGVALIDATOR)
    add_dependencies(triangle_demo triangle_shaders engine_standard_shaders)
    add_custom_command(TARGET triangle_demo POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:triangle_demo>/shaders
        COMMAND ${CMAKE_COMMAND} -E copy ${VERT_SPV} $<TARGET_FILE_DIR:triangle_demo>/triangle.vert.spv
        COMMAND ${CMAKE_COMMAND} -E copy ${FRAG_SPV} $<TARGET_FILE_DIR:triangle_demo>/triangle.frag.spv
        COMMAND ${CMAKE_COMMAND} -E copy ${ENGINE_VERT_SPV} $<TARGET_FILE_DIR:triangle_demo>/shaders/standard.vert.bin
        COMMAND ${CMAKE_COMMAND} -E copy ${ENGINE_FRAG_SPV} $<TARGET_FILE_DIR:triangle_demo>/shaders/standard.frag.bin
    )
endif()

set_target_properties(triangle_demo PROPERTIES CXX_STANDARD 17)
