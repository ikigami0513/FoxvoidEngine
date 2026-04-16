#pragma once

#include <imgui.h>
#include <filesystem>
#include <iostream>
#include "../world/Scene.hpp"
#include "../world/GameObject.hpp"

namespace fs = std::filesystem;

class ProjectPanel {
    public:
        ProjectPanel() = default;
        ~ProjectPanel() = default;
    
        void Draw(Scene& activeScene, GameObject*& selectedObject, const fs::path& assetsPath, std::string& currentScenePath);

    private:
        // Recursive function to read and display the folder tree
        void DrawDirectoryNode(Scene& activeScene, GameObject*& selectedObject, const fs::path& path, std::string& currentScenePath);

        void DrawExplorerView(Scene& activeScene, GameObject*& selectedObject, std::string& currentScenePath);

        bool m_isTreeView = true; // Start in Tree view by default
        fs::path m_currentDirectory = "";

        // Flag to trigger the modal safely
        bool m_requestScriptModal = false;
};
