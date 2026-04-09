#pragma once

#include <string>
#include "world/Scene.hpp"

// The Engine class encapsulates the core loop and window management.
class Engine {
    public:
        // Constructor initializes the Raylib window and core systems
        Engine(int width, int height, const std::string& title);
        
        // Destructor ensures proper cleanup of Raylib resources
        ~Engine();

        // Deleted copy constructor and assignment operator to prevent 
        // duplicating the engine instance (Singleton-like behavior)
        Engine(const Engine&) = delete;
        Engine& operator=(const Engine&) = delete;

        // Starts the main game loop
        void Run();

    private:
        // Core loop stages
        void ProcessInput();
        void Update(float deltaTime);
        void Render();

        // Engine state
        bool m_isRunning;
        int m_windowWidth;
        int m_windowHeight;
        std::string m_windowTitle;
        Scene m_activeScene;
};
