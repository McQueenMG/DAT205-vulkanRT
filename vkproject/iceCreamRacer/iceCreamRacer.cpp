#include "iceCreamRacer.hpp"

#include <filesystem>

#include <GLFW/glfw3.h>
#include <ecs/ECS.h>
#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#include <vkproject/scene.hpp>
#include <vkproject/asset_manager.hpp>
#include <vkproject/triangle_asset.hpp>
#include <vkproject/game.hpp>
#include <vkproject/log.hpp>
#include <vkproject/renderer.hpp>
#include <vkproject/input.hpp>
#include <vkproject/glfw_input.hpp>
#include <memory>

AssetManager g_asset_manager;
AssetManager* asset_manager = &g_asset_manager;

// Global input system
IInput* input = nullptr;

// Define globals declared as extern in headers
Game* game = nullptr;
IRenderer* renderer = nullptr;

namespace
{
bool g_components_registered = false;

class IceCreamCarScene : public Scene
{
public:
    void Init() override
    {
        if (!g_components_registered)
        {
            entity_manager.RegisterComponent<StaticRenderable>();
            entity_manager.RegisterComponent<DynamicRenderable>();
            entity_manager.RegisterComponent<LightComponent>();
            g_components_registered = true;
        }

        entity_manager.ClearAll();
        used_assets.clear();

        const std::filesystem::path source_path(__FILE__);
        const std::filesystem::path vkproject_root = source_path.parent_path().parent_path();
        const std::filesystem::path car_obj_path = vkproject_root / "triangleObjects/ice_cream_car/ice_cream_car.obj";



        auto car_mesh = triangle_asset::LoadObjMesh(car_obj_path.string());
        car_mesh = triangle_asset::NormalizeTriangleMesh(car_mesh);
        car_mesh = triangle_asset::OrientTriangleMeshOutwards(car_mesh);

        glm::mat4 rot = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        for (auto &v : car_mesh.vertices) {
            glm::vec4 p = rot * glm::vec4(v, 1.0f);
            v = glm::vec3(p);
        }

        auto registered_car = asset_manager->RegisterMeshAsset("ice_cream_car_demo", car_mesh);
        car_asset_id = registered_car.asset_id;

        Material floor_material{};
        floor_material.color = glm::vec4(0.13f, 0.15f, 0.17f, 1.0f);
        floor_material.emittance = 0.0f;
        floor_material.metalness = 0.0f;
        floor_material.shininess = 0.0f;
        auto registered_floor = asset_manager->RegisterMeshAsset(
            "ice_cream_car_demo_floor",
            triangle_asset::CreateQuadMesh(glm::vec3(-12.0f, -12.0f, 3.0f), glm::vec3(12.0f, 12.0f, 3.0f), floor_material));
        floor_asset_id = registered_floor.asset_id;

        auto& car_entity = entity_manager.Create();
        car_entity.AddComponent<StaticRenderable>(glm::vec2(0.0f, 0.0f), car_asset_id);
        car_entity.GetComponent<StaticRenderable>()->variation = 0;

        auto& floor_entity = entity_manager.Create();
        floor_entity.AddComponent<StaticRenderable>(glm::vec2(0.0f, 0.0f), floor_asset_id);

        // Light positioned above the scene — with -Z up, "above" means negative Z.
        auto& light_entity = entity_manager.Create();
        light_entity.AddComponent<LightComponent>(glm::vec3(-6.0f, -10.0f, -10.0f), glm::vec3(340.0f, 320.0f, -300.0f));

        auto& light_entity2 = entity_manager.Create();
        light_entity2.AddComponent<LightComponent>(glm::vec3(-6.0f, 5.0f, -10.0f), glm::vec3(540.0f, 520.0f, -500.0f));

        used_assets = {car_asset_id, floor_asset_id};
    }

    std::vector<uint32_t> GetUsedAssets() override
    {
        return used_assets;
    }

    void Update() override
    {
        if (!renderer) return;

        VulkanRTRenderer* vk_renderer = static_cast<VulkanRTRenderer*>(renderer);
        if (!vk_renderer) return;
        GLFWwindow* glfw_win = vk_renderer->context.glfw_window;
        if (!glfw_win) return;

        double mouse_x, mouse_y;
        glfwGetCursorPos(glfw_win, &mouse_x, &mouse_y);
        double mouse_dx = mouse_x - prev_mouse_x;
        double mouse_dy = mouse_y - prev_mouse_y;
        prev_mouse_x = mouse_x;
        prev_mouse_y = mouse_y;

        // Rebuild forward from yaw/pitch
        glm::vec3 forward;
        forward.x =  cos(pitch) * cos(yaw);
        forward.y =  cos(pitch) * sin(yaw);
        forward.z = -sin(pitch);   // negate for -Z up
        forward = glm::normalize(forward);

        const glm::vec3 world_up = glm::vec3(0.0f, 0.0f, 1.0f);  // back to +Z
        glm::vec3 right = glm::normalize(glm::cross(forward, world_up));

        // Mouse look
        if (mouse_dx != 0.0 || mouse_dy != 0.0)
        {
            yaw   -= (float)mouse_dx * mouse_sensitivity;
            pitch -= (float)mouse_dy * mouse_sensitivity;
            pitch  = glm::clamp(pitch, -glm::half_pi<float>() + 0.01f,
                                        glm::half_pi<float>() - 0.01f);

            // Recompute after update
            forward.x = cos(pitch) * cos(yaw);
            forward.y = cos(pitch) * sin(yaw);
            forward.z = -sin(pitch);
            forward = glm::normalize(forward);
            right = glm::normalize(glm::cross(forward, world_up));
        }

        // WASD
        if (input && input->IsPressed(W)) { camera_eye += forward * camera_speed; }
        if (input && input->IsPressed(S)) { camera_eye -= forward * camera_speed; }
        if (input && input->IsPressed(D)) { camera_eye += right   * camera_speed; }
        if (input && input->IsPressed(A)) { camera_eye -= right   * camera_speed; }

        if (input && input->IsPressed(SPACE)) { camera_eye -= world_up * camera_speed; }  // swapped
        if (input && input->IsPressed(SHIFT)) { camera_eye += world_up * camera_speed; }  // swapped

        camera_target = camera_eye + forward;
        renderer->SetCamera(camera_eye, camera_target, world_up);
    }

    void Destroy() override
    {
        entity_manager.ClearAll();
        used_assets.clear();
    }

private:
    uint32_t car_asset_id = 0;
    uint32_t floor_asset_id = 0;
    std::vector<uint32_t> used_assets;

    // Camera starts behind and above the car.
    // With -Z up, "above" means negative Z, so eye is at Z=-8.
    glm::vec3 camera_eye = glm::vec3(0.0f, -18.0f, -8.0f);  // back to original +Z
    glm::vec3 camera_target = glm::vec3(0.0f, 0.0f, 0.0f);

    const float camera_speed      = 0.05f;
    const float mouse_sensitivity = 0.005f;
    double prev_mouse_x = 0.0, prev_mouse_y = 0.0;

    // eye=(0,-18,-8), target=(0,0,0) => forward = normalize(0, 18, 8)
    // yaw   = atan2(18, 0) = pi/2
    // pitch = asin(8 / length(0,18,8)) = asin(8/19.73) ≈ 0.438 (positive now, looking slightly "down" toward origin)
    float yaw   = glm::half_pi<float>();
    float pitch = 0.438f;
};
}  // namespace

class IceCreamRacerGame : public Game
{
public:
    IceCreamRacerGame(int w, int h) : width(w), height(h) {}

    void Init() override
    {
        renderer_impl.Init(width, height);
        renderer = &renderer_impl;

        GLFWwindow* win = renderer_impl.context.glfw_window;
        input_impl = std::make_unique<GLFWInput>(win);
        input = input_impl.get();

        auto* scene = new IceCreamCarScene();
        scenes.push_back(scene);

        double mx, my;
        if (win) glfwGetCursorPos(win, &mx, &my);
    }

    void Destroy() override
    {
        for (auto s : scenes) { s->Destroy(); delete s; }
        scenes.clear();

        if (renderer_impl.context.device) renderer_impl.context.device.waitIdle();
        input_impl.reset();
        renderer_impl.Destroy();
        renderer = nullptr;
        input = nullptr;
    }

private:
    int width, height;
    VulkanRTRenderer renderer_impl;
    std::unique_ptr<GLFWInput> input_impl;
};

int main(int argc, char** argv)
{
    IceCreamRacerGame app(1920, 1080);
    game = &app;
    app.Start();
    return 0;
}