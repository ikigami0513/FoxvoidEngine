#pragma once

#ifndef STANDALONE_MODE
#include <imgui.h>
#endif

#include <nlohmann/json.hpp>
#include "editor/EditorViewMode.hpp"
#include "../world/Scene.hpp"
#include "../world/GameObject.hpp"

// Panel for Play / Stop and Save / Load controls
class ToolbarPanel {
    public:
        void Draw(Scene& activeScene, GameObject*& selectedObject, nlohmann::json& sceneBackup, EditorViewMode& currentViewMode);

    private:
        // Stores the original scene path to restore it when STOP is pressed
        std::string m_savedScenePathBeforePlay = "";
};
