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
        void Draw(Scene& activeScene, GameObject*& selectedObject, bool& isPlaying, nlohmann::json& sceneBackup, EditorViewMode& currentViewMode);
};
