#pragma once

#include <raylib.h>
#include "EditorCamera.hpp"
#include <world/Scene.hpp>

#ifndef STANDALONE_MODE
#include <imgui.h>
#endif
#include "EditorViewMode.hpp"

// Panel to display the rendered game texture
class SceneViewPanel {
    public:
        void Draw(RenderTexture2D& sceneTexture, EditorCamera& camera, Scene& activeScene, GameObject*& selectedObject, int selectedTileID, int selectedLayer, EditorViewMode& currentViewMode);
};
