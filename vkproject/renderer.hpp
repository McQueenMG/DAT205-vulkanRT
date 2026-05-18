#pragma once
#include "Scene.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


struct StaticRenderable
{
    static uint32_t component_type_id;
    StaticRenderable() = default;
    StaticRenderable(const glm::vec2& position, uint32_t asset) : position(position), asset(asset) {};
    glm::vec2 position;
    glm::mat4 model_matrix; 
    glm::mat4 GetModelMatrix()
    {
        glm::vec3 current_pos = glm::vec3(position, 0.0f);
        model_matrix = glm::translate(glm::mat4(1.0f), current_pos);
        return model_matrix;
    }
    uint32_t asset; 
    uint32_t variation = 0; 
};

struct DynamicRenderable
{
    static uint32_t component_type_id;
    enum Type { TURN_BASED, FREE } type;
    DynamicRenderable() = default;
    DynamicRenderable(const glm::vec2& position, const glm::vec2& direction, uint32_t asset, DynamicRenderable::Type type = TURN_BASED)
        : position(position), direction(direction), asset(asset), anim_progress(0.0f), type(type){};
    glm::vec2 position;
    glm::vec2 direction; 
    glm::vec2 anim_target;
    glm::mat4 current_model_matrix; 
    glm::mat4 prev_model_matrix; 
    glm::mat4 GetCurrentAndSetPreviousModelMatrix() { 
        prev_model_matrix = current_model_matrix; 
        auto current_pos = glm::vec3(position, 0.0f);
        auto current_dir = glm::vec3(direction, 0.0f);
        if (type == TURN_BASED)
        {
            current_pos =
                (1.0f - anim_progress) * glm::vec3(position, 0.0f) + anim_progress * glm::vec3(anim_target, 0.0f);
        }
        float angle = std::atan2(current_dir.y, current_dir.x);
        current_model_matrix = glm::translate(glm::mat4(1.0f), current_pos) * glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 0.0f, 1.0f));
        return current_model_matrix; 
    }
    float anim_progress; 
    uint32_t asset; 
    uint32_t variation = 0; 
};

struct LightComponent
{
    static uint32_t component_type_id;
    LightComponent() = default;
    LightComponent(const glm::vec3& position, const glm::vec3& intensity) : position(position), intensity(intensity){};
    glm::vec3 position;
    glm::vec3 intensity;
};


struct IRenderer
{
    virtual void Init(int width, int height) = 0; // Should probably take a json or something with settings
    virtual void Destroy() = 0; 
    virtual void SetScene(Scene * scene) = 0; 
    virtual void SetCamera(const glm::vec3 &eye, const glm::vec3 &target, const glm::vec3 &up) = 0;
    virtual void NewFrame(){}; 
    virtual void Render() = 0; 
};

extern IRenderer * renderer;