find_program(GLSLC NAMES glslc)

set(TRI_SRC ${CMAKE_CURRENT_SOURCE_DIR}/iceCreamRacer.cpp)
set(TRI_HDR ${CMAKE_CURRENT_SOURCE_DIR}/iceCreamRacer.hpp)

set(RT_RGEN_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../vulkan_renderer/shaders/raytrace.rgen)
set(RT_RMISS_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../vulkan_renderer/shaders/raytrace.rmiss)
set(RT_RCHIT_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../vulkan_renderer/shaders/raytrace.rchit)
set(BLIT_VERT_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../vulkan_renderer/shaders/blit.vert)
set(BLIT_FRAG_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../vulkan_renderer/shaders/blit.frag)
set(STANDARD_VERT_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../vulkan_renderer/shaders/standard.vert)
set(STANDARD_FRAG_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../vulkan_renderer/shaders/standard.frag)

set(RT_RGEN_BIN ${CMAKE_CURRENT_BINARY_DIR}/raytrace.rgen.bin)
set(RT_RMISS_BIN ${CMAKE_CURRENT_BINARY_DIR}/raytrace.rmiss.bin)
set(RT_RCHIT_BIN ${CMAKE_CURRENT_BINARY_DIR}/raytrace.rchit.bin)
set(BLIT_VERT_BIN ${CMAKE_CURRENT_BINARY_DIR}/blit.vert.bin)
set(BLIT_FRAG_BIN ${CMAKE_CURRENT_BINARY_DIR}/blit.frag.bin)
set(STANDARD_VERT_BIN ${CMAKE_CURRENT_BINARY_DIR}/standard.vert.bin)
set(STANDARD_FRAG_BIN ${CMAKE_CURRENT_BINARY_DIR}/standard.frag.bin)

if(GLSLC)
    add_custom_command(OUTPUT ${RT_RGEN_BIN}
        COMMAND ${GLSLC} --target-spv=spv1.5 ${RT_RGEN_SRC} -o ${RT_RGEN_BIN}
        DEPENDS ${RT_RGEN_SRC}
        COMMENT "Compiling raytrace.rgen to SPIR-V"
    )
    add_custom_command(OUTPUT ${RT_RMISS_BIN}
        COMMAND ${GLSLC} --target-spv=spv1.5 ${RT_RMISS_SRC} -o ${RT_RMISS_BIN}
        DEPENDS ${RT_RMISS_SRC}
        COMMENT "Compiling raytrace.rmiss to SPIR-V"
    )
    add_custom_command(OUTPUT ${RT_RCHIT_BIN}
        COMMAND ${GLSLC} --target-spv=spv1.5 ${RT_RCHIT_SRC} -o ${RT_RCHIT_BIN}
        DEPENDS ${RT_RCHIT_SRC}
        COMMENT "Compiling raytrace.rchit to SPIR-V"
    )
    add_custom_command(OUTPUT ${BLIT_VERT_BIN}
        COMMAND ${GLSLC} --target-spv=spv1.5 ${BLIT_VERT_SRC} -o ${BLIT_VERT_BIN}
        DEPENDS ${BLIT_VERT_SRC}
        COMMENT "Compiling blit.vert to SPIR-V"
    )
    add_custom_command(OUTPUT ${BLIT_FRAG_BIN}
        COMMAND ${GLSLC} --target-spv=spv1.5 ${BLIT_FRAG_SRC} -o ${BLIT_FRAG_BIN}
        DEPENDS ${BLIT_FRAG_SRC}
        COMMENT "Compiling blit.frag to SPIR-V"
    )
    add_custom_command(OUTPUT ${STANDARD_VERT_BIN}
        COMMAND ${GLSLC} --target-spv=spv1.5 ${STANDARD_VERT_SRC} -o ${STANDARD_VERT_BIN}
        DEPENDS ${STANDARD_VERT_SRC}
        COMMENT "Compiling standard.vert to SPIR-V"
    )
    add_custom_command(OUTPUT ${STANDARD_FRAG_BIN}
        COMMAND ${GLSLC} --target-spv=spv1.5 ${STANDARD_FRAG_SRC} -o ${STANDARD_FRAG_BIN}
        DEPENDS ${STANDARD_FRAG_SRC}
        COMMENT "Compiling standard.frag to SPIR-V"
    )
    add_custom_target(iceCreamRacer_shaders
        DEPENDS ${RT_RGEN_BIN} ${RT_RMISS_BIN} ${RT_RCHIT_BIN} ${BLIT_VERT_BIN} ${BLIT_FRAG_BIN} ${STANDARD_VERT_BIN} ${STANDARD_FRAG_BIN}
    )
endif()

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

target_include_directories(iceCreamRacer
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..                                   # vkproject
    ${CMAKE_CURRENT_SOURCE_DIR}/../..                                # repo root
    ${CMAKE_CURRENT_SOURCE_DIR}/../../external/glfw/include         # glfw
    ${CMAKE_CURRENT_SOURCE_DIR}/../../external/imgui                # imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/../../external/VulkanMemoryAllocator/include/   # VMA C header
    ${CMAKE_CURRENT_SOURCE_DIR}/../../external/VulkanMemoryAllocator-Hpp/include/ # VMA-Hpp
)

if(GLSLC)
    add_dependencies(iceCreamRacer iceCreamRacer_shaders)
    add_custom_command(TARGET iceCreamRacer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:iceCreamRacer>/shaders
        COMMAND ${CMAKE_COMMAND} -E copy ${RT_RGEN_BIN} $<TARGET_FILE_DIR:iceCreamRacer>/shaders/raytrace.rgen.bin
        COMMAND ${CMAKE_COMMAND} -E copy ${RT_RMISS_BIN} $<TARGET_FILE_DIR:iceCreamRacer>/shaders/raytrace.rmiss.bin
        COMMAND ${CMAKE_COMMAND} -E copy ${RT_RCHIT_BIN} $<TARGET_FILE_DIR:iceCreamRacer>/shaders/raytrace.rchit.bin
        COMMAND ${CMAKE_COMMAND} -E copy ${BLIT_VERT_BIN} $<TARGET_FILE_DIR:iceCreamRacer>/shaders/blit.vert.bin
        COMMAND ${CMAKE_COMMAND} -E copy ${BLIT_FRAG_BIN} $<TARGET_FILE_DIR:iceCreamRacer>/shaders/blit.frag.bin
        COMMAND ${CMAKE_COMMAND} -E copy ${STANDARD_VERT_BIN} $<TARGET_FILE_DIR:iceCreamRacer>/shaders/standard.vert.bin
        COMMAND ${CMAKE_COMMAND} -E copy ${STANDARD_FRAG_BIN} $<TARGET_FILE_DIR:iceCreamRacer>/shaders/standard.frag.bin
    )
endif()

set_target_properties(iceCreamRacer PROPERTIES CXX_STANDARD 17)

