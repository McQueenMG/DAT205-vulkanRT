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
#include <__msvc_chrono.hpp> 

AssetManager g_asset_manager;
AssetManager* asset_manager = &g_asset_manager;

// Global input system
IInput* input = nullptr;

// Define globals declared as extern in headers
Game* game = nullptr;
IRenderer* renderer = nullptr;
struct tunnel_part
{
    ecs::Entity* tunnel_entity = nullptr;
    std::vector<ecs::Entity*> box_entities;
    std::vector<ecs::Entity*> light_entities;
    int box_position1;
    int box_position2;
};

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
        is_left=true;
        box_position1=0;
        box_position2=0;
        tunnel_counter=0;
        tunnel_entities.clear();
        cam_locked_to_car = false;
        game_over_popup_open = false;


        const std::filesystem::path source_path(__FILE__);
        const std::filesystem::path vkproject_root = source_path.parent_path().parent_path();
        const std::filesystem::path car_obj_path = vkproject_root / "triangleObjects/ice_cream_car/ice_cream_car.obj";
        const std::filesystem::path tunnel_obj_path = vkproject_root / "triangleObjects/Tunnel/TUNNEL.obj";
        const std::filesystem::path box_obj_path = vkproject_root / "triangleObjects/box/Sci-fi_Container_Box.obj";

        auto tunnel_mesh = triangle_asset::LoadObjMesh(tunnel_obj_path.string());
        tunnel_mesh = triangle_asset::NormalizeTriangleMesh(tunnel_mesh);
        tunnel_mesh = triangle_asset::ScaleTriangleMesh(tunnel_mesh, 7.0f);

        auto car_mesh = triangle_asset::LoadObjMesh(car_obj_path.string());
        car_mesh = triangle_asset::NormalizeTriangleMesh(car_mesh);
        car_mesh = triangle_asset::ScaleTriangleMesh(car_mesh, 0.5f);  

        auto box_mesh = triangle_asset::LoadObjMesh(box_obj_path.string());
        box_mesh = triangle_asset::NormalizeTriangleMesh(box_mesh);
        box_mesh = triangle_asset::ScaleTriangleMesh(box_mesh, 0.4f);

        glm::mat4 rot = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        for (auto &v : car_mesh.vertices) {
            glm::vec4 p = rot * glm::vec4(v, 1.0f);
            v = glm::vec3(p);
        }

        for (auto &v : tunnel_mesh.vertices) {
            glm::vec4 p = rot * glm::vec4(v, 1.0f);
            v = glm::vec3(p);
        }

        rot = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        for (auto &v : tunnel_mesh.vertices) {
            glm::vec4 p = rot * glm::vec4(v, 1.0f);
            v = glm::vec3(p);
        }

        rot = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 1.0f));
        for (auto &v : box_mesh.vertices) {
            glm::vec4 p = rot * glm::vec4(v, 1.0f);
            v = glm::vec3(p);
        }

        car_pos = glm::vec3(0.0f, 0.5f, 0.0f);
        car_direction = glm::vec3(0.1f, 0.0f, 0.0f);
    

        auto registered_tunnel = asset_manager->RegisterMeshAsset("tunnel_demo", tunnel_mesh);
        tunnel_asset_id = registered_tunnel.asset_id;

        auto registered_car = asset_manager->RegisterMeshAsset("ice_cream_car_demo", car_mesh);
        car_asset_id = registered_car.asset_id;

        auto registered_box = asset_manager->RegisterMeshAsset("box_demo", box_mesh);
        box_asset_id = registered_box.asset_id;

        auto& car = entity_manager.Create();
        car_entity = &car;
        car_entity->AddComponent<DynamicRenderable>(car_pos, car_direction, car_asset_id);
        car_entity->GetComponent<DynamicRenderable>()->variation = 0;

        // auto& box_entity = entity_manager.Create();
        // box_entity.AddComponent<StaticRenderable>(glm::vec3(2.0f, 0.5f, 0.0f), box_asset_id);
        // box_entity.GetComponent<StaticRenderable>()->variation = 0;
        
        
        auto& light_entity = entity_manager.Create();
        light_entity.AddComponent<LightComponent>(glm::vec3(-3.0f, -5.0f, -5.0f), glm::vec3(8.0f, 8.0f, -8.0f));
        create_start_tunnel(); 
       

        used_assets = {car_asset_id, road_asset_id, tunnel_asset_id, box_asset_id};
    }

    void create_start_tunnel(){
         glm::vec3 tunnel_pos1(0.0f, 0.0f, -1.15f);
        tunnel_entities.push_back(createTunnelPart(entity_manager, tunnel_pos1, tunnel_asset_id));

        glm::vec3 tunnel_pos2(28.0f, 0.0f, -1.15f);
        tunnel_entities.push_back(createTunnelPart(entity_manager, tunnel_pos2, tunnel_asset_id));

        glm::vec3 tunnel_pos3(56.0f, 0.0f, -1.15f);
        tunnel_entities.push_back(createTunnelPart(entity_manager, tunnel_pos3, tunnel_asset_id));

        glm::vec3 tunnel_pos4(84.0f, 0.0f, -1.15f);
        tunnel_entities.push_back(createTunnelPart(entity_manager, tunnel_pos4, tunnel_asset_id));
    }

    tunnel_part createTunnelPart(ecs::EntityManager& entity_manager, const glm::vec3& tunnelPos, uint32_t tunnelId)
    {
        auto& tunnel_entity = entity_manager.Create();
        tunnel_entity.AddComponent<StaticRenderable>(tunnelPos, tunnelId);
        tunnel_entity.GetComponent<StaticRenderable>()->variation = 0;

        std::vector<ecs::Entity*> light_entities;
        std::vector<ecs::Entity*> box_entities;
        box_position1 = rand() % 2;
        box_position2 = rand() % 2;

        auto addTunnelLight = [&](const glm::vec3& offset)
        {
            auto& light_entity = entity_manager.Create();
            light_entity.AddComponent<LightComponent>(tunnelPos + offset, glm::vec3(20.0f, 20.0f, -20.0f));
            light_entities.push_back(&light_entity);
        };
        auto addBox = [&](const glm::vec3& offset)
        {
            auto& box_entity= entity_manager.Create();
            box_entity.AddComponent<StaticRenderable>(tunnelPos + offset, box_asset_id);
            box_entity.GetComponent<StaticRenderable>()->variation = 0;
            box_entities.push_back(&box_entity);
        };

        addTunnelLight(glm::vec3(0.0f, 0.0f, -1.5f));
        addTunnelLight(glm::vec3(10.0f, 0.0f, -1.5f));
        addTunnelLight(glm::vec3(-10.0f, 0.0f, -1.5f));
        if (box_position1== 0) {
            addBox(glm::vec3(10.0f, 0.5f, 0.7f));
        } else {
            addBox(glm::vec3(10.0f, -0.5f, 0.7f));
        }
        if (box_position2 == 0) {
            addBox(glm::vec3(20.0f, 0.5f, 0.7f));
        } else {
            addBox(glm::vec3(20.0f, -0.5f, 0.7f));
        }
        return tunnel_part{&tunnel_entity, std::move(box_entities), std::move(light_entities), box_position1, box_position2};
    }

    void removeTunnelPart(ecs::EntityManager& entity_manager, tunnel_part& part)
    {
        for (auto& light_entity : part.light_entities)
        {
            entity_manager.Remove(*light_entity);
        }
        for (auto& box_entity : part.box_entities)
        {
            entity_manager.Remove(*box_entity);
        }
        
        entity_manager.Remove(*part.tunnel_entity);
    }

    std::vector<uint32_t> GetUsedAssets() override
    {
        return used_assets;
    }

    void Update(float dt) override
    {
        if (!renderer) return;
        
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

        VulkanRTRenderer* vk_renderer = static_cast<VulkanRTRenderer*>(renderer);
        if (!vk_renderer) return;
        GLFWwindow* glfw_win = vk_renderer->context.glfw_window;
        if (!glfw_win) return;

        if (!timerhasstarted) {
            start = std::chrono::steady_clock::now();
            timerhasstarted = true;
        }
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
        if (!cam_locked_to_car) {
            if (input && input->IsPressed(W)) { camera_eye += forward * camera_speed; }
            if (input && input->IsPressed(S)) { camera_eye -= forward * camera_speed; }
            if (input && input->IsPressed(D)) { camera_eye += right   * camera_speed; }
            if (input && input->IsPressed(A)) { camera_eye -= right   * camera_speed; }
            
            if (input && input->IsPressed(SPACE)) { camera_eye -= world_up * camera_speed; }  
            if (input && input->IsPressed(SHIFT)) { camera_eye += world_up * camera_speed; }  
            camera_target = camera_eye + forward;
        } else {
            
            glm::vec3 car_offset = glm::vec3(-3.5f, 0.0f, -1.5f);  // back and above in local space
            float car_yaw = atan2(car_direction.y, car_direction.x);
            glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), car_yaw, world_up);
            glm::vec3 rotated_offset = glm::vec3(rotation * glm::vec4(car_offset, 1.0f));
            camera_eye = car_pos + rotated_offset;
            camera_target = car_pos;
            if (!game_over) 
            {
                car_speed+=car_speed_increment*dt;
            }  
            car_pos += car_direction * car_speed * dt;
            //if (input && input->IsPressed(W)) {car_pos += car_direction * car_speed;}
            //if (input && input->IsPressed(S)) {car_pos -= car_direction * car_speed;}
            if(input && input->IsPressed(R)) {
                Reset();
            }

            if (input && input->IsJustPressed(D) && is_left) {
                is_left = false;
                car_pos.y-=1.1f;
                // float angle = -glm::half_pi<float>() * car_turn_speed; // negative for right turn
                // glm::mat4 rot = glm::rotate(glm::mat4(1.0f), angle, world_up);
                // car_direction = glm::vec3(rot * glm::vec4(car_direction, 1.0f));

            }
            if (input && input->IsJustPressed(A) && !is_left) {
                is_left = true;
                car_pos.y+=1.1f;
                // float angle = glm::half_pi<float>() * car_turn_speed; // positive for left turn
                // glm::mat4 rot = glm::rotate(glm::mat4(1.0f), angle, world_up);
                // car_direction = glm::vec3(rot * glm::vec4(car_direction, 1.0f));
            }
            if (game_over) {
                if(stop_timer) {
                    std::chrono::steady_clock::time_point fin = std::chrono::steady_clock::now();
                    final_duration = std::chrono::duration_cast<std::chrono::duration<double>>(fin - start).count();
                    stop_timer=false;
                }
                if (!game_over_popup_open) {
                    ImGui::OpenPopup("Game Over");
                    game_over_popup_open = true;
                }
                if (ImGui::BeginPopupModal("Game Over", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
                    ImGui::Text("You hit a box! Time: %.3f s", final_duration);
                    ImGui::Separator();
                    ImGui::TextUnformatted("Press R to restart, ESC to exit");
                    if (input && input->IsJustPressed(R)) {
                        Reset();
                    }
                    if (input && input->IsJustPressed(ESCAPE)) {
                        if (vk_renderer && vk_renderer->context.glfw_window) {
                            glfwSetWindowShouldClose(vk_renderer->context.glfw_window, GLFW_TRUE);
                        }
                    }
                    ImGui::EndPopup();
                }
            } 
            //else {
            //     ImGui::Begin("Duration", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs);
            //     ImGui::Text("Time: %.3f s", run_duration);
            //     ImGui::End();
            // }
        }
        
        if (car_entity)
        {
            auto* dr = car_entity->GetComponent<DynamicRenderable>();
            if (dr)
            {
                dr->position = car_pos;
                dr->direction = car_direction;
            }
        }
        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        //if (car_pos.x>10.0f+28.0f*tunnel_counter) {
        // if (tunnel_x-5<car_pos<tunnel_x+5)
        if(!game_over) {
        if(tunnel_entities.front().tunnel_entity->GetComponent<StaticRenderable>()->position.x + 8.0f < car_pos.x && car_pos.x < tunnel_entities.front().tunnel_entity->GetComponent<StaticRenderable>()->position.x + 12.0f) { 
            if (is_left && tunnel_entities.front().box_position1==0){
                hit_box(start);
            }else if (!is_left && tunnel_entities.front().box_position1==1)
            {
                hit_box(start);
            }
        }
        if(tunnel_entities.front().tunnel_entity->GetComponent<StaticRenderable>()->position.x + 18.0f < car_pos.x && car_pos.x < tunnel_entities.front().tunnel_entity->GetComponent<StaticRenderable>()->position.x + 22.0f) { 
            if (is_left && tunnel_entities.front().box_position2==0){
                hit_box(start);
            }else if (!is_left && tunnel_entities.front().box_position2==1)
            {
                hit_box(start);
            }
        }
    }
        //elseif(car_pos.x==20.0f+28.0f*tunnel_counter)
        if (car_pos.x > tunnel_entities.front().tunnel_entity->GetComponent<StaticRenderable>()->position.x + 30.0f)
        {
            glm::vec3 new_tunnel_pos(tunnel_entities.back().tunnel_entity->GetComponent<StaticRenderable>()->position.x + 28.0f, 0.0f, -1.15f);
            tunnel_entities.push_back(createTunnelPart(entity_manager, new_tunnel_pos, tunnel_asset_id));
            removeTunnelPart(entity_manager, tunnel_entities.front());
            tunnel_entities.erase(tunnel_entities.begin());
            tunnel_counter++;
        }
        
        renderer->SetCamera(camera_eye, camera_target, world_up);
        
    


        if (input && input->IsJustPressed(L)) 
        { 
            if (cam_locked_to_car)
            {
                cam_locked_to_car = false;
                // pause the start timer
                // pause_time = std::chrono::steady_clock::now();

            }
            else{
                cam_locked_to_car = true;
                //start = pause_time;
            }
        }


    }
    void hit_box(std::chrono::steady_clock::time_point& start){
        car_speed=0.0f;
        game_over=true;
        stop_timer=true;
        game_over_popup_open = false;
        if(input && input->IsJustPressed(R)) {
            Reset();
        }
    }
    void Reset()
    {
        for (auto& tunnel_part : tunnel_entities)
        {
            removeTunnelPart(entity_manager, tunnel_part);
        }
        tunnel_entities.clear();
        tunnel_counter=0;
        is_left=true;
        timerhasstarted=false;
        game_over=false;
        car_speed=20.0f;
        create_start_tunnel();
        car_direction = glm::vec3(0.1f, 0.0f, 0.0f);
        car_pos = glm::vec3(0.0f, 0.5f, 0.0f);
   

    }

    void Destroy() override
    {
        entity_manager.ClearAll();
        used_assets.clear();
    }

private:
    uint32_t car_asset_id = 0;
    uint32_t road_asset_id = 0;
    uint32_t tunnel_asset_id = 0;
    uint32_t box_asset_id = 0;
    std::vector<uint32_t> used_assets;
    std::vector<tunnel_part> tunnel_entities;

    // Camera starts behind and above the car.
    // With -Z up, "above" means negative Z, so eye is at Z=-8.
    glm::vec3 camera_eye = glm::vec3(0.0f, -18.0f, -8.0f);  // back to original +Z
    glm::vec3 camera_target = glm::vec3(0.0f, 0.0f, 0.0f);

    ecs::Entity* car_entity = nullptr;
    glm::vec3 car_direction;
    glm::vec3 car_pos;
    bool cam_locked_to_car = false;

    float car_speed = 20.0f;
    float car_speed_increment = 2.5f;
    const float car_turn_speed = 0.05f;
    const float camera_speed      = 0.05f;
    const float mouse_sensitivity = 0.005f;
    double prev_mouse_x = 0.0, prev_mouse_y = 0.0;
    bool timerhasstarted = false;
    // std::chrono::steady_clock::time_point pause_time;

    float yaw   = glm::half_pi<float>();
    float pitch = 0.438f;
    bool is_left=true;
    int box_position1=0;
    int box_position2=0;
    int tunnel_counter=0;
    double run_duration = 0.0;
    double final_duration=0.0;
    bool game_over=false;
    bool stop_timer=false;
    bool game_over_popup_open=false;
    std::chrono::steady_clock::time_point start;

};
} 

IceCreamRacerGame::IceCreamRacerGame(int w, int h) : width(w), height(h) {}

void IceCreamRacerGame::Init()
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

void IceCreamRacerGame::Destroy()
{
    for (auto s : scenes) { s->Destroy(); delete s; }
    scenes.clear();

    if (renderer_impl.context.device) renderer_impl.context.device.waitIdle();
    input_impl.reset();
    renderer_impl.Destroy();
    renderer = nullptr;
    input = nullptr;
}

int main(int argc, char** argv)
{
    IceCreamRacerGame app(1920, 1080);
    game = &app;
    app.Start();
    return 0;
}