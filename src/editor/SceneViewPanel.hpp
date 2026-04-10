#pragma once

#include <raylib.h>
#include <imgui.h>

// Panel to display the rendered game texture
class SceneViewPanel {
    public:
        void Draw(RenderTexture2D& sceneTexture);
};
