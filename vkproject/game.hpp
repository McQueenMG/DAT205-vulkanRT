#pragma once
#include "renderer.hpp"
#include "scene.hpp"
#include <vector>

struct Game
{
    std::vector<Scene *> scenes; 
    int current_scene; 
    bool should_quit = false;
    bool should_restart = true;
    int next_scene = -1; 
    void Start();
    virtual void Init() = 0; 
    void Quit();
    void Restart();
    virtual void Destroy() = 0; 
};

extern Game *game; 