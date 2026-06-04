#include "glfw_input.hpp"
#include <map>


std::map<Keys, uint32_t> mapping = {{ESCAPE, GLFW_KEY_ESCAPE}, {SPACE, GLFW_KEY_SPACE}, {W, GLFW_KEY_W},
                                    {A, GLFW_KEY_A},           {S, GLFW_KEY_S},         {D, GLFW_KEY_D},
                                    {SHIFT, GLFW_KEY_LEFT_SHIFT}, {L, GLFW_KEY_L}, {R, GLFW_KEY_R}};

void GLFWInput::Update() 
{ 
    previous_state = current_state;
    glfwPollEvents(); 
    for (int i = 0; i < NUM_KEYS; ++i)
    {
        current_state[i] = glfwGetKey(glfw_window, mapping[(Keys)i]) == GLFW_PRESS;
    }

}

bool GLFWInput::IsPressed(Keys key) 
{ 
    if (glfwGetKey(glfw_window, mapping[key]) == GLFW_PRESS) return true; 
    return false; 
}

bool GLFWInput::IsJustPressed(Keys key)
{
    return current_state[key] && !previous_state[key];
}
