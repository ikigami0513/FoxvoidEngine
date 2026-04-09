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

    // Initialize the Raylib window
    InitWindow(m_windowWidth, m_windowHeight, m_windowTitle.c_str());
    
    // Set a target frame rate to avoid burning CPU cycles
    SetTargetFPS(60);

    // Create the render texture
    // We give it the base resolution of the game
    m_sceneTexture = LoadRenderTexture(m_windowWidth, m_windowHeight);

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
    // Create a new GameObject in the scene
    GameObject* player = m_activeScene.CreateGameObject("Player");
    
    // Add a Transform component (centered on screen, assuming 800x600 window)
    player->AddComponent<Transform2d>(400.0f, 300.0f);
    player->AddComponent<SpriteSheetRenderer>("assets/textures/player_base.png", 9, 56);
    player->AddComponent<Animation2d>(std::vector<int>{0, 1, 2, 3, 4, 5}, 0.15f, true);
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
    // ========================================================
    // PASS 1: GAME RENDERING (OFF-SCREEN)
    // ========================================================
    BeginTextureMode(m_sceneTexture);
        // Clear the background with the game's default color
        ClearBackground(RAYWHITE); 
        
        // Draw all entities present in the active scene
        m_activeScene.Render();    
    EndTextureMode();

    // ========================================================
    // PASS 2: EDITOR RENDERING (ON-SCREEN)
    // ========================================================
    BeginDrawing();
    
    // Clear the actual application window with a dark color behind the editor panels
    ClearBackground(DARKGRAY); 

    rlImGuiBegin();
    
    // Enable global docking over the entire application viewport
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport()); 

    ImGui::Begin("Toolbar");

    // Change button color and text based on the current state
    if (!m_isPlaying) {
        // Green Play Button
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
        if (ImGui::Button("PLAY")) {
            m_isPlaying = true;
            std::cout << "[Editor] Entered PLAY mode." << std::endl;
            
            m_sceneBackup = m_activeScene.Serialize();
        }
        ImGui::PopStyleColor();
    } else {
        // Red Stop Button
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button("STOP")) {
            m_isPlaying = false;
            std::cout << "[Editor] Entered EDIT mode." << std::endl;
            
            m_activeScene.Deserialize(m_sceneBackup);
        }
        ImGui::PopStyleColor();
    }
    
    ImGui::End();

    // --- WINDOW 1: THE VIEWPORT (The Game Scene) ---
    // Remove inner margins (padding) so the render texture touches the window borders
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Scene View");
    
    // Get the available size inside this specific ImGui window
    ImVec2 size = ImGui::GetContentRegionAvail();
    if (size.x > 0.0f && size.y > 0.0f) {
        // Draw the game's texture. 
        // Important note: In OpenGL, textures are stored upside down (Y-axis inverted).
        // We use a negative height in the source rectangle to flip the image right-side up!
        Rectangle sourceRec = { 
            0.0f, 
            0.0f, 
            (float)m_sceneTexture.texture.width, 
            -(float)m_sceneTexture.texture.height 
        };
        
        // Draw the texture fitting the available ImGui window space
        rlImGuiImageRect(&m_sceneTexture.texture, (int)size.x, (int)size.y, sourceRec);
    }
    
    ImGui::End();
    ImGui::PopStyleVar(); // Restore normal padding for subsequent windows

    // The hierarchy (Scene Graph)
    ImGui::Begin("Hierarchy");

    // Iterate through all alive objects in the active scene
    for (const auto& go : m_activeScene.GetGameObjects()) {
        // Check if the current object in the loop is the one we selected
        bool isSelected = (m_selectedObject == go.get());

        // ImGui::Selectable creates a clickable text line. 
        // If clicked, it returns true.
        if (ImGui::Selectable(go->name.c_str(), isSelected)) {
            // Update the engine's selected object pointer
            m_selectedObject = go.get();
        }
    }

    // Click on empty space to deselect
    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        m_selectedObject = nullptr;
    }

    ImGui::End();

    ImGui::Begin("Inspector");

    if (m_selectedObject != nullptr) {
        // Display the name of the selected object
        ImGui::Text("Name: %s", m_selectedObject->name.c_str());

        ImGui::Separator();

        // Iterate through all components attached to the selected object
        for (const auto& comp : m_selectedObject->GetComponents()) {
            // Push an ID so ImGui doesn't get confused if two components have identical variables
            ImGui::PushID(comp.get());

            // Create a collapsible header for each component using its specific name
            // ImGuiTreeNodeFlags_DefaultOpen ensures it's expanded by default
            if (ImGui::CollapsingHeader(comp->GetName().c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                
                // Call the component's custom UI code
                comp->OnInspector();
            }
            
            ImGui::PopID();
        }
    }
    else {
        // If nothing is selected, display a helpful message
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Select an object in the Hierarchy.");
    }

    ImGui::End();

    rlImGuiEnd();
    EndDrawing();
}
