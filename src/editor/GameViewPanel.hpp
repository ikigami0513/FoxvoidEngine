#pragma once

#include <raylib.h>
#include <imgui.h>
#include <rlImGui.h>

// Panel to display the renderer game exactly as the player will see it
class GameViewPanel {
    public:
        void Draw(RenderTexture2D& gameTexture);
};
