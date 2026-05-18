#pragma once
#include <vkproject/vulkan_renderer/vulkan_rt_renderer.hpp>
#include <vkproject/game.hpp>
#include <vkproject/glfw_input.hpp>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

class IceCreamRacerGame : public Game
{
public:
    IceCreamRacerGame(int w, int h);
    virtual void Init() override;
    virtual void Destroy() override;

private:
    int width, height;
    VulkanRTRenderer renderer_impl;
    std::unique_ptr<GLFWInput> input_impl;
};
