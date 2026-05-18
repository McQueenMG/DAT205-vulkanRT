#pragma once
#include "input.hpp"
#include <array>
#include <GLFW/glfw3.h>

struct GLFWInput : public IInput
{
    GLFWwindow* glfw_window; 
    GLFWInput(GLFWwindow* window) : glfw_window(window) {}
    virtual void Update() override;
    virtual bool IsPressed(Keys key) override;
    virtual bool IsJustPressed(Keys key) override;

private:
    std::array<bool, NUM_KEYS> current_state{};
    std::array<bool, NUM_KEYS> previous_state{};
};