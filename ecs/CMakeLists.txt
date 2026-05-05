find_package( glm REQUIRED )

add_library(ecs 
    ECS.h
    entity.h
    entity.cpp
    entity_methods.h
    entity_manager.h
    entity_manager.cpp
)

target_link_libraries( ecs 
    PRIVATE
        glm
)

target_include_directories( ecs
    PUBLIC
        ${CMAKE_SOURCE_DIR}
)

