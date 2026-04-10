#pragma once

#include <raylib.h>
#include <imgui.h>
#include "EditorCamera.hpp"

// Panel to display the rendered game texture
class SceneViewPanel {
    public:
        void Draw(RenderTexture2D& sceneTexture, EditorCamera& camera);
};
