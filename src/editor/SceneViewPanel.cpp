#include "SceneViewPanel.hpp"
#include <rlImGui.h>

void SceneViewPanel::Draw(RenderTexture2D& sceneTexture, EditorCamera& camera) {
    // Remove inner margins (padding) so the render texture touches the window borders
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Scene View");

    // Check if the user's mouse is currently hovering this specific ImGui window
    bool isHovered = ImGui::IsWindowHovered();

    // Pass the hover state to the camera so it only pans/zooms when appropriate
    camera.Update(isHovered);

    // Get the available size inside this specific ImGui window
    ImVec2 size = ImGui::GetContentRegionAvail();
    if (size.x > 0.0f && size.y > 0.0f) {
        // In OpenGL, textures are stored upside down. We use negative height to flip it.
        Rectangle sourceRec = { 0.0f, 0.0f, (float)sceneTexture.texture.width, -(float)sceneTexture.texture.height };
        rlImGuiImageRect(&sceneTexture.texture, (int)size.x, (int)size.y, sourceRec);
    }
    
    ImGui::End();
    ImGui::PopStyleVar();
}
