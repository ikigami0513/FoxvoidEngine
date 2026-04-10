#include "Engine.hpp"
#include <iostream>
#include <physics/Transform2d.hpp>
#include <graphics/ShapeRenderer.hpp>
#include <scripting/ScriptEngine.hpp>
#include <scripting/ScriptComponent.hpp>
#include <imgui.h>
#include <rlImGui.h>
#include <graphics/SpriteSheetRenderer.hpp>
#include <graphics/Animation2d.hpp>
#include "AssetManager.hpp"
#include <graphics/Animator2d.hpp>
#include "world/ComponentRegistration.hpp"
#include <world/ComponentRegistry.hpp>

namespace fs = std::filesystem;

Engine* Engine::s_instance = nullptr;

Engine::Engine(int width, int height, const std::string& title)
    : m_windowWidth(width), 
      m_windowHeight(height), 
      m_windowTitle(title), 
      m_isRunning(false),
      m_isPlaying(false)
{
    // Assign the current instance to our static pointer 
    // so Engine::Get() can return it properly.
    s_instance = this;

    // Make the Engine window resizable
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);

    // Initialize the Raylib window
    InitWindow(m_windowWidth, m_windowHeight, m_windowTitle.c_str());
    
    // Initialize the stream redirectors
    // Everything sent to std::cout will be captured as Info (white text)
    m_coutRedirect = std::make_unique<ConsoleSink>(std::cout, LogLevel::Info, m_console);

    // Everything sent to std::cerr will be captured as Error (red text)
    m_cerrRedirect = std::make_unique<ConsoleSink>(std::cerr, LogLevel::Error, m_console);

    // Set a target frame rate to avoid burning CPU cycles
    SetTargetFPS(60);

    // One single call to register everything needed for the Scene loading
    EngineSetup::RegisterNativeComponents();

    // Create the render texture
    // We give it the base resolution of the game
    m_sceneTexture = LoadRenderTexture(m_windowWidth, m_windowHeight);

    // Initialize the Editor Camera, passing the base resolution so it centers correctly
    m_editorCamera = std::make_unique<EditorCamera>((float)m_windowWidth, (float)m_windowHeight);

    // Initialize ImGui and enable Docking
    rlImGuiSetup(true); // true = dark theme
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking Space!

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

    AssetManager::Clear();

    // Shutdown ImGui
    rlImGuiShutdown();

    // Free the VRAM
    // Safely unload the render texture from the GPU
    UnloadRenderTexture(m_sceneTexture);

    // Close the Raylib window and free graphics resources
    CloseWindow();
    std::cout << "[Engine] Shutdown complete." << std::endl;
}

void Engine::Run() {
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
    // Only run game logic (Python scripts, animations) if PLAY is active
    if (m_isPlaying) {
        m_activeScene.Update(deltaTime);
    }

    // Always manage memory, even in Edit mode
    // This ensures objects created via the Editor or Engine::Run
    // are immediately added to the scene graph.
    m_activeScene.Flush();
}

void Engine::Render() {
    // Pass 1: Game Rendering (OFF-SCREEN)
    BeginTextureMode(m_sceneTexture);
        // Use a dark gray background to make the grid pop out nicely
        ClearBackground(Color{ 40, 40, 40, 255 });
        
        // Activate the 2D Camera
        m_editorCamera->Begin();
            
            // Draw the background grid (100 lines, spaced by 50 pixels)
            m_editorCamera->DrawGrid(100, 50.0f);

            // Draw all entities. They will now be affected by zoom and pan!
            m_activeScene.Render();    
            
        // Deactivate the camera
        m_editorCamera->End();
    EndTextureMode();

    // Pass 2: Editor Rendering (ON-SCREEN)
    BeginDrawing();
    
    // Clear the actual application window with a dark color behind the editor panels
    ClearBackground(DARKGRAY); 

    rlImGuiBegin();
    
    // Enable global docking over the entire application viewport
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport()); 

    // Draw all the isolated editor panels
    m_toolbarPanel.Draw(m_activeScene, m_selectedObject, m_isPlaying, m_sceneBackup);
    m_sceneViewPanel.Draw(m_sceneTexture, *m_editorCamera);
    m_hierarchyPanel.Draw(m_activeScene, m_selectedObject);
    m_console.Draw("Console");
    m_inspectorPanel.Draw(m_selectedObject);
    m_projectPanel.Draw(m_activeScene, m_selectedObject, m_assetsPath);

    rlImGuiEnd();
    EndDrawing();
}
