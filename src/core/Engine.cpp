#include "Engine.hpp"
#include <iostream>
#include <physics/Transform2d.hpp>
#include <graphics/ShapeRenderer.hpp>
#include <scripting/ScriptEngine.hpp>
#include <scripting/ScriptComponent.hpp>
#include <imgui.h>
#include <rlImGui.h>
#include "ImGuizmo.h"
#include <graphics/SpriteSheetRenderer.hpp>
#include <graphics/Animation2d.hpp>
#include "AssetManager.hpp"
#include <graphics/Animator2d.hpp>
#include "world/ComponentRegistration.hpp"
#include <world/ComponentRegistry.hpp>
#include "extras/IconsFontAwesome6.h"
#include "InputManager.hpp"
#include "graphics/TileMap.hpp"

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

    // Load default input bindings if they exist
    InputManager::Load("assets/settings/inputs.json");

    // Create both render textures
    // We give it the base resolution of the game
    m_sceneTexture = LoadRenderTexture(m_windowWidth, m_windowHeight);
    m_gameTexture = LoadRenderTexture(m_windowWidth, m_windowHeight);

    // Initialize the Editor Camera, passing the base resolution so it centers correctly
    m_editorCamera = std::make_unique<EditorCamera>((float)m_windowWidth, (float)m_windowHeight);

    // Initialize ImGui and enable Docking
    rlImGuiSetup(true); // true = dark theme
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking Space!

    ImGuiIO& io = ImGui::GetIO();

    float baseFontSize = 28.0f;
    ImFont* font = io.Fonts->AddFontFromFileTTF("assets/fonts/Roboto-Regular.ttf", baseFontSize);
    io.FontDefault = font;

    float iconFontSize = baseFontSize * 0.8f;
    
    ImFontConfig config;
    config.MergeMode = true;
    config.PixelSnapH = true;
    config.GlyphMinAdvanceX = iconFontSize;

    static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };

    io.Fonts->AddFontFromFileTTF("assets/fonts/fa-solid-900.ttf", iconFontSize, &config, icon_ranges);

    ApplyModernTheme();

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
    // Safely unload the render textures from the GPU
    UnloadRenderTexture(m_sceneTexture);
    UnloadRenderTexture(m_gameTexture);

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
    // Check if a scene change was requested last frame
    if (!m_pendingScenePath.empty()) {
        std::cout << "[Engine] Loading new scene: " << m_pendingScenePath << std::endl;

        // Destroy all current GameObjects and free memory
        m_activeScene.Clear();

        // Load the new JSON (.scene) file
        m_activeScene.LoadFromFile(m_pendingScenePath);

        // Update the editor's path so it knows what we are editing
        m_currentScenePath = m_pendingScenePath;

        // Trigger the Start() method for all the newly loaded components
        m_activeScene.Start();

        // Clear the pending path so we don't load it again next frame
        m_pendingScenePath = "";
    }

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
    // Pass 1: Game Rendering (What the player sees)
    BeginTextureMode(m_gameTexture);
        ClearBackground(RAYWHITE);

        BeginMode2D(m_activeScene.GetMainCamera((float)m_windowWidth, (float)m_windowHeight));
            // Draw all entities normally
            m_activeScene.Render();
        EndMode2D();

        m_activeScene.RenderHUD();
    EndTextureMode();

    // Pass 2: Editor Rendering
    BeginTextureMode(m_sceneTexture);
        // Use a dark gray background to make the grid pop out nicely
        ClearBackground(Color{ 40, 40, 40, 255 });
        
        // Activate the 2D Camera
        m_editorCamera->Begin();
            
            // Draw the background grid only if the toggle is true (100 lines, spaced by 50 pixels)
            if (m_showGlobalGrid) {
                m_editorCamera->DrawGrid(100, 50.0f);
            }

            // Draw all entities. They will now be affected by zoom and pan!
            m_activeScene.Render();    

            // Draw the collision outlines over the graphics
            PhysicsEngine::RenderDebug(m_activeScene);

            if (m_selectedObject) {
                if (auto tilemap = m_selectedObject->GetComponent<TileMap>()) {
                    tilemap->RenderGrid();
                }
            }
            
        // Deactivate the camera
        m_editorCamera->End();

        m_activeScene.RenderHUD();
    EndTextureMode();

    // Pass 3: Editor Rendering (ON-SCREEN)
    BeginDrawing();
    
        // Clear the actual application window with a dark color behind the editor panels
        ClearBackground(DARKGRAY); 

        rlImGuiBegin();
            ImGuizmo::BeginFrame();
        
            // Enable global docking over the entire application viewport
            ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport()); 

            m_mainMenuBar.Draw(m_activeScene, m_currentScenePath, m_isRunning, m_selectedObject, m_inputSettingsPanel, m_showGlobalGrid);

            // Draw the Input Settings (it will only draw if m_isOpen is true)
            m_inputSettingsPanel.Draw();

            Texture2D currentTileset = {0};
            Vector2 tileSize = { 32.0f, 32.0f };
            int tileSpacing = 0;

            if (m_selectedObject) {
                if (auto tileMap = m_selectedObject->GetComponent<TileMap>()) {
                    currentTileset = tileMap->GetTexture();
                    tileSize = tileMap->tileSize;
                    tileSpacing = tileMap->tileSpacing;
                }
            }

            TileMap* activeTileMap = nullptr;
            if (m_selectedObject) {
                activeTileMap = m_selectedObject->GetComponent<TileMap>();
            }
            m_tilePalettePanel.Draw(m_selectedTileID, m_selectedLayer, activeTileMap);

            // Draw all the isolated editor panels
            m_toolbarPanel.Draw(m_activeScene, m_selectedObject, m_isPlaying, m_sceneBackup, m_focusGameWindow);
            
            // Draw both panels, feeding them their respective textures
            m_sceneViewPanel.Draw(m_sceneTexture, *m_editorCamera, m_activeScene, m_selectedObject, m_selectedTileID, m_selectedLayer);
            m_gameViewPanel.Draw(m_gameTexture, m_focusGameWindow);

            m_hierarchyPanel.Draw(m_activeScene, m_selectedObject);
            m_console.Draw("Console");
            m_inspectorPanel.Draw(m_selectedObject);
            m_projectPanel.Draw(m_activeScene, m_selectedObject, m_assetsPath, m_currentScenePath);

        rlImGuiEnd();
    EndDrawing();
}

void Engine::LoadScene(const std::string& scenePath) {
    std::cout << "[Engine] Scene change requested: " << scenePath << std::endl;
    m_pendingScenePath = scenePath;
}

void Engine::ApplyModernTheme() {
    // Get the global ImGui style instance
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // =========================================================
    // 1. GEOMETRY (Rounding & Borders)
    // =========================================================
    // Soften all corners to remove the blocky 90s look
    style.WindowRounding    = 6.0f;
    style.ChildRounding     = 4.0f;
    style.FrameRounding     = 4.0f;
    style.PopupRounding     = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding      = 3.0f;
    style.TabRounding       = 4.0f;

    // Remove harsh borders to make the UI look flat and modern
    style.WindowBorderSize  = 0.0f;
    style.FrameBorderSize   = 0.0f;
    style.PopupBorderSize   = 1.0f; // Keep a small border for popups so they don't blend in

    // =========================================================
    // 2. COLORS (Sleek Dark Theme with Orange Accent)
    // =========================================================
    // Backgrounds (Dark gradients)
    colors[ImGuiCol_WindowBg]           = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
    colors[ImGuiCol_ChildBg]            = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    colors[ImGuiCol_PopupBg]            = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    
    // Inputs and Frames (Buttons, Text boxes)
    colors[ImGuiCol_FrameBg]            = ImVec4(0.20f, 0.20f, 0.20f, 1.0f);
    colors[ImGuiCol_FrameBgHovered]     = ImVec4(0.30f, 0.30f, 0.30f, 1.0f);
    colors[ImGuiCol_FrameBgActive]      = ImVec4(0.40f, 0.40f, 0.40f, 1.0f);
    
    // Window Headers (Title bars)
    colors[ImGuiCol_TitleBg]            = ImVec4(0.09f, 0.09f, 0.09f, 1.0f);
    colors[ImGuiCol_TitleBgActive]      = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    
    // Tabs (Docking space)
    colors[ImGuiCol_Tab]                = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    colors[ImGuiCol_TabHovered]         = ImVec4(0.30f, 0.30f, 0.30f, 1.0f);
    colors[ImGuiCol_TabActive]          = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
    colors[ImGuiCol_TabUnfocused]       = ImVec4(0.10f, 0.10f, 0.10f, 1.0f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    
    // Standard Buttons
    colors[ImGuiCol_Button]             = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
    colors[ImGuiCol_ButtonHovered]      = ImVec4(0.35f, 0.35f, 0.35f, 1.0f);
    colors[ImGuiCol_ButtonActive]       = ImVec4(0.45f, 0.45f, 0.45f, 1.0f);
    
    // --- ACCENT COLOR (Foxvoid Orange) ---
    // This color is used for Checkboxes, Sliders, and active elements
    ImVec4 accentColor          = ImVec4(0.90f, 0.45f, 0.10f, 1.0f); 
    ImVec4 accentHovered        = ImVec4(1.00f, 0.55f, 0.20f, 1.0f);
    
    colors[ImGuiCol_CheckMark]          = accentColor;
    colors[ImGuiCol_SliderGrab]         = accentColor;
    colors[ImGuiCol_SliderGrabActive]   = accentHovered;
    colors[ImGuiCol_Header]             = ImVec4(0.30f, 0.30f, 0.30f, 1.0f); // Used for CollapsingHeaders
    colors[ImGuiCol_HeaderHovered]      = ImVec4(0.40f, 0.40f, 0.40f, 1.0f);
    colors[ImGuiCol_HeaderActive]       = ImVec4(0.45f, 0.45f, 0.45f, 1.0f);
}
