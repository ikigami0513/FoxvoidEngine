#pragma once

#include <string>
#include "../world/Scene.hpp"
#include "InputSettingsPanel.hpp"

class MainMenuBar {
    public:
        MainMenuBar() = default;
        ~MainMenuBar() = default;

        // Draws the menu bar and handles the Save as popup
        void Draw(Scene& activeScene, std::string& currentScenePath, bool& isRunning, GameObject*& selectedObject, InputSettingsPanel& inputPanel, bool& showGlobalGrid);
    private:
        bool m_openSavePopup = false;
};
