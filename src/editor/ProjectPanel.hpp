#pragma once

#include <imgui.h>
#include <filesystem>
#include "../world/Scene.hpp"
#include "../world/GameObject.hpp"

namespace fs = std::filesystem;

class ProjectPanel {
    public:
        void Draw(Scene& activeScene, GameObject*& selectedObject, const fs::path& assetsPath);

    private:
        // Recursive function to read and display the folder tree
        void DrawDirectoryNode(Scene& activeScene, GameObject*& selectedObject, const fs::path& path);
};
