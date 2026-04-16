#pragma once

#include <string>
#include <filesystem>
#include <memory>
#include <raylib.h>
#include "world/Scene.hpp"
#include "editor/EditorConsole.hpp"
#include "editor/HierarchyPanel.hpp"
#include "editor/InspectorPanel.hpp"
#include "editor/ProjectPanel.hpp"
#include "editor/SceneViewPanel.hpp"
#include "editor/GameViewPanel.hpp"
#include "editor/ToolbarPanel.hpp"
#include <editor/MainMenuBar.hpp>
#include "editor/InputSettingsPanel.hpp"
#include "editor/TilePalettePanel.hpp"

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

        // Returns the active instance of the Engine
        static Engine* Get() { return s_instance; }
        
        // Returns a reference to the active scene to allow GameObject instantiation
        Scene& GetActiveScene() { return m_activeScene; }

        // Requests a scene change for the start of the next frame
        void LoadScene(const std::string& scenePath);

    private:
        // Core loop stages
        void ProcessInput();
        void Update(float deltaTime);
        void Render();

        // Applies a modern, profesionnal look to the ImGui interface
        void ApplyModernTheme();

        // The root path of the project assets
        std::filesystem::path m_assetsPath = "assets";

        // Engine state
        bool m_isRunning;

        // Play / Edit mode state
        // True when  the game is running, False when in Editor mode
        bool m_isPlaying;

        int m_windowWidth;
        int m_windowHeight;
        std::string m_windowTitle;

        Scene m_activeScene;
        nlohmann::json m_sceneBackup;

        // Render textures
        RenderTexture2D m_sceneTexture; // What the Editor sees
        RenderTexture2D m_gameTexture; // What the Player sees

        // Keeps track of the currently selected entity in the Hierarchy
        GameObject* m_selectedObject = nullptr;

        // Editor UI Panels
        EditorConsole m_console;
        ToolbarPanel m_toolbarPanel;

        SceneViewPanel m_sceneViewPanel;
        GameViewPanel m_gameViewPanel;

        HierarchyPanel m_hierarchyPanel;
        InspectorPanel m_inspectorPanel;
        ProjectPanel m_projectPanel;

        InputSettingsPanel m_inputSettingsPanel;

        MainMenuBar m_mainMenuBar;

        int m_selectedTileID = -1;
        int m_selectedLayer = 0;

        TilePalettePanel m_tilePalettePanel;

        // The camera used to navigate the scene in the editor
        std::unique_ptr<EditorCamera> m_editorCamera;

        // Pointers for the stream redirectors
        std::unique_ptr<ConsoleSink> m_coutRedirect;
        std::unique_ptr<ConsoleSink> m_cerrRedirect;

        // Flag to trigger tab switching
        bool m_focusGameWindow = false;

        // Flag to toggle the global editor background grid
        bool m_showGlobalGrid = true;

        std::string m_currentScenePath = "";

        // Stores the path of the scene waiting to be loaded
        std::string m_pendingScenePath = "";

        // Static pointer holding the unique Engine instance
        static Engine* s_instance;
};
