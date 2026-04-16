#pragma once

#include <string>
#include <filesystem>
#include <memory>
#include <raylib.h>
#include "world/Scene.hpp"
#include "world/GameObject.hpp"

// Editor Panels
#include "editor/EditorConsole.hpp"
#include "editor/HierarchyPanel.hpp"
#include "editor/InspectorPanel.hpp"
#include "editor/ProjectPanel.hpp"
#include "editor/SceneViewPanel.hpp"
#include "editor/GameViewPanel.hpp"
#include "editor/ToolbarPanel.hpp"
#include "editor/MainMenuBar.hpp"
#include "editor/InputSettingsPanel.hpp"
#include "editor/TilePalettePanel.hpp"
#include "editor/GameStatePanel.hpp"
#include "ProjectHubPanel.hpp"

class Editor {
    public:
        Editor(int windowWidth, int windowHeight);
        ~Editor();

        // The main function that orchestrates all editor rendering
        void Draw(Scene& activeScene, RenderTexture2D& gameTexture, bool& isRunning, bool& isPlaying, std::string& currentScenePath, nlohmann::json& sceneBackup);

    private:
        void ApplyModernTheme();

        bool m_isProjectLoaded = false;
        ProjectHubPanel m_projectHub;

        void OnProjectLoaded();

        // Editor State
        GameObject* m_selectedObject = nullptr;
        bool m_focusGameWindow = false;
        bool m_showGlobalGrid = true;
        int m_selectedTileID = -1;
        int m_selectedLayer = 0;
        std::filesystem::path m_assetsPath = "assets";

        // Editor Specific Rendering
        RenderTexture2D m_sceneTexture; // Only the editor needs to render the scene with gizmos
        std::unique_ptr<EditorCamera> m_editorCamera;

        // Console Redirection
        std::unique_ptr<ConsoleSink> m_coutRedirect;
        std::unique_ptr<ConsoleSink> m_cerrRedirect;

        // Panels
        EditorConsole m_console;
        ToolbarPanel m_toolbarPanel;
        SceneViewPanel m_sceneViewPanel;
        GameViewPanel m_gameViewPanel;
        HierarchyPanel m_hierarchyPanel;
        InspectorPanel m_inspectorPanel;
        ProjectPanel m_projectPanel;
        InputSettingsPanel m_inputSettingsPanel;
        GameStatePanel m_gameStatePanel;
        MainMenuBar m_mainMenuBar;
        TilePalettePanel m_tilePalettePanel;
};
