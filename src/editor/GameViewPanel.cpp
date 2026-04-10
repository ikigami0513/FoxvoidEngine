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
    
    ImVec2 size = ImGui::GetContentRegionAvail();
    if (size.x > 0.0f && size.y > 0.0f) {
        // Remember to flip the texture vertically for OpenGL
        Rectangle sourceRec = { 0.0f, 0.0f, (float)gameTexture.texture.width, -(float)gameTexture.texture.height };
        rlImGuiImageRect(&gameTexture.texture, (int)size.x, (int)size.y, sourceRec);
    }
    
    ImGui::End();
    ImGui::PopStyleVar();
}
