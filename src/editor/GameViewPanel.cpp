#include "GameViewPanel.hpp"

void GameViewPanel::Draw(RenderTexture2D& gameTexture, bool& focusGameWindow) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    
    // The name of the window matters for ImGui docking
    ImGui::Begin("Game View");

    // If the flag was set by the Toolbar, we force focus on this window
    if (focusGameWindow) {
        ImGui::SetWindowFocus();
        focusGameWindow = false; // Reset the flag immediately
    }
    
    ImVec2 windowSize = ImGui::GetContentRegionAvail();
    if (windowSize.x > 0.0f && windowSize.y > 0.0f) {
        // Aspect Ratio Calculation
        float texWidth = (float)gameTexture.texture.width;
        float texHeight = (float)gameTexture.texture.height;
        float targetAspect = texWidth / texHeight;
        float windowAspect = windowSize.x / windowSize.y;

        ImVec2 drawSize;
        if (windowAspect > targetAspect) {
            drawSize.y = windowSize.y;
            drawSize.x = windowSize.y * targetAspect;
        } else {
            drawSize.x = windowSize.x;
            drawSize.y = windowSize.x / targetAspect;
        }

        // Calculate centered position
        ImVec2 cursorPos = ImGui::GetCursorPos();
        cursorPos.x += (windowSize.x - drawSize.x) * 0.5f;
        cursorPos.y += (windowSize.y - drawSize.y) * 0.5f;
        ImGui::SetCursorPos(cursorPos);

        Rectangle sourceRec = { 0.0f, 0.0f, texWidth, -texHeight };
        rlImGuiImageRect(&gameTexture.texture, (int)drawSize.x, (int)drawSize.y, sourceRec);
    }
    
    ImGui::End();
    ImGui::PopStyleVar();
}
