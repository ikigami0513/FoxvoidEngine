#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <filesystem>
#include "../world/Scene.hpp"
#include "InputSettingsPanel.hpp"
#include "GameStatePanel.hpp"

class MainMenuBar {
    public:
        MainMenuBar() = default;
        ~MainMenuBar() = default;

        // Draws the menu bar and handles the Save as popup
        void Draw(Scene& activeScene, std::string& currentScenePath, bool& isRunning, GameObject*& selectedObject, InputSettingsPanel& inputPanel, GameStatePanel& gameStatePanel, bool& showGlobalGrid);
    private:
        bool m_openSavePopup = false;

        // Flag to control the build configuration modal
        bool m_openBuildPopup = false;

        // Multi-threading and progress tracking for the Build system
        bool m_openBuildProgressPopup = false;

        std::atomic<bool> m_isBuilding{false};
        std::atomic<int> m_buildProgress{0}; // Tracks completion percentage (0 to 100, or -1 for error)

        std::mutex m_buildMutex;
        std::string m_buildStatusMsg = "";

        // Console logs container
        std::vector<std::string> m_buildLogs;

        // The function that will run in the background thread
        void RunBuildThread(std::string startSceneStr, std::string outputDirStr, std::filesystem::path projectRoot, std::string engineRoot);

        // Helper to execute command, read output, and parse progress
        int ExecuteCommandWithOutput(const std::string& cmd, int baseProgress, int maxProgress);
};
