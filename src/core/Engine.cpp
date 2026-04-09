#include "Engine.hpp"
#include <raylib.h>
#include <iostream>
#include <physics/Transform2d.hpp>
#include <graphics/ShapeRenderer.hpp>
#include <scripting/ScriptEngine.hpp>
#include <scripting/ScriptComponent.hpp>

Engine::Engine(int width, int height, const std::string& title)
    : m_windowWidth(width), 
      m_windowHeight(height), 
      m_windowTitle(title), 
      m_isRunning(false) 
{
    // Initialize the Raylib window
    InitWindow(m_windowWidth, m_windowHeight, m_windowTitle.c_str());
    
    // Set a target frame rate to avoid burning CPU cycles
    SetTargetFPS(60);

    // If the window was successfully created, mark the engine as running
    if (IsWindowReady()) {
        m_isRunning = true;
        std::cout << "[Engine] Initialized successfully." << std::endl;
        ScriptEngine::Initialize();
    } else {
        std::cerr << "[Engine] Failed to initialize window." << std::endl;
    }
}

Engine::~Engine() {
    m_activeScene.Clear();

    ScriptEngine::Shutdown();

    // Close the Raylib window and free graphics resources
    CloseWindow();
    std::cout << "[Engine] Shutdown complete." << std::endl;
}

void Engine::Run() {
    // Create a new GameObject in the scene
    GameObject* player = m_activeScene.CreateGameObject("Player");
    
    // Add a Transform component (centered on screen, assuming 800x600 window)
    player->AddComponent<Transform2d>(400.0f, 300.0f);

    // Add a ShapeRenderer component (a 50x50 RED square)
    player->AddComponent<ShapeRenderer>(50.0f, 50.0f, RED);

    player->AddComponent<ScriptComponent>("main", "PlayerController");

    // Main game loop: continues as long as the engine is running 
    // and the user hasn't pressed ESC or the close button
    while (m_isRunning && !WindowShouldClose()) {
        // Calculate the time taken by the last frame (Delta Time)
        // Useful for frame-rate independent movement
        float deltaTime = GetFrameTime();

        ProcessInput();
        Update(deltaTime);
        Render();
    }
}

void Engine::ProcessInput() {
    // Handle global engine inputs here (e.g., toggling debug mode)
    // Gameplay inputs will eventually be handled by the Scene/Entities
}

void Engine::Update(float deltaTime) {
    m_activeScene.Update(deltaTime);
}

void Engine::Render() {
    // Begin the drawing phase
    BeginDrawing();

    // Clear the screen with a specific background color
    ClearBackground(RAYWHITE);

    m_activeScene.Render();

    // End the drawing phase and swap buffers to display
    EndDrawing();
}
