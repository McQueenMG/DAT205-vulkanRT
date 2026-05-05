#include "game.hpp"
#include "input.hpp"

void Game::Start()
{ 
    while (should_restart)
    {
        should_restart = false; 
        Init();
        assert(scenes.size() > 0);
        current_scene = 0;
        next_scene = -1;
        while (!should_quit && !should_restart)
        {
            auto& scene = scenes[current_scene];
            scene->Init();

            renderer->SetScene(scene);
            while (next_scene == -1 && !should_quit && !should_restart)
            {
                renderer->NewFrame(); 
                input->Update();
                scene->Update();
                renderer->Render();
                if (input->IsPressed(ESCAPE)) Quit();
            }
            if (next_scene != -1)
            {
                current_scene = next_scene;
                next_scene = -1;
            }
        }
    }
}

void Game::Quit() {
	Destroy();
	should_quit = true; 
}

void Game::Restart() { 
    Destroy();  
    should_restart = true; 
}