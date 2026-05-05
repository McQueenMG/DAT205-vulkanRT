#include "glfw_input.hpp"
#include <map>


std::map<Keys, uint32_t> mapping = {{ESCAPE, GLFW_KEY_ESCAPE}, {SPACE, GLFW_KEY_SPACE}, {W, GLFW_KEY_W},
                                    {A, GLFW_KEY_A},           {S, GLFW_KEY_S},         {D, GLFW_KEY_D}};

void GLFWInput::Update() 
{ 
    glfwPollEvents(); 

}

bool GLFWInput::IsPressed(Keys key) 
{ 
    if (glfwGetKey(glfw_window, mapping[key]) == GLFW_PRESS) return true; 
    return false; 
}