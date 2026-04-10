#include "SceneViewPanel.hpp"
#include <rlImGui.h>

void SceneViewPanel::Draw(RenderTexture2D& sceneTexture, EditorCamera& camera, Scene& activeScene, GameObject*& selectedObject) {
    // Remove inner margins (padding) so the render texture touches the window borders
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Scene View");

    // Check if the user's mouse is currently hovering this specific ImGui window
    bool isHovered = ImGui::IsWindowHovered();

    // Pass the hover state to the camera so it only pans/zooms when appropriate
    camera.Update(isHovered);

    // Get the available size inside this specific ImGui window
    ImVec2 windowSize = ImGui::GetContentRegionAvail();
    if (windowSize.x > 0.0f && windowSize.y > 0.0f) {
        // Aspect Ratio Calculation
        float texWidth = (float)sceneTexture.texture.width;
        float texHeight = (float)sceneTexture.texture.height;
        float targetAspect = texWidth / texHeight;
        float windowAspect = windowSize.x / windowSize.y;

        ImVec2 drawSize;
        if (windowAspect > targetAspect) {
            // Window is wider than the texture -> Fit to height
            drawSize.y = windowSize.y;
            drawSize.x = windowSize.y * targetAspect;
        } else {
            // Window is taller than the texture -> Fit to width
            drawSize.x = windowSize.x;
            drawSize.y = windowSize.x / targetAspect;
        }

        // Calculate centered position
        ImVec2 cursorPos = ImGui::GetCursorPos(); // Top-left of the available region
        cursorPos.x += (windowSize.x - drawSize.x) * 0.5f;
        cursorPos.y += (windowSize.y - drawSize.y) * 0.5f;
        ImGui::SetCursorPos(cursorPos);

        // Draw the image with the correctly scaled size
        Rectangle sourceRec = { 0.0f, 0.0f, texWidth, -texHeight };
        rlImGuiImageRect(&sceneTexture.texture, (int)drawSize.x, (int)drawSize.y, sourceRec);

        // Mouse picking logic
        // Only pick if we hover the image and click the Left Mouse Button
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            // Get Mouse position exactly relative to the drawn image's top-left corner
            ImVec2 mousePosAbsolute = ImGui::GetMousePos();
            ImVec2 imagePosAbsolute = ImGui::GetItemRectMin(); 
            
            Vector2 mousePosRel = {
                mousePosAbsolute.x - imagePosAbsolute.x,
                mousePosAbsolute.y - imagePosAbsolute.y
            };

            // Convert from UI size to RenderTexture size
            Vector2 renderTexturePos = {
                (mousePosRel.x / drawSize.x) * texWidth,
                (mousePosRel.y / drawSize.y) * texHeight
            };

            // Convert from RenderTexture space to World Space (applying Camera Zoom/Pan)
            Vector2 worldPos = GetScreenToWorld2D(renderTexturePos, camera.GetCamera());

            // Raycast in the scene to find the object
            GameObject* pickedObject = activeScene.PickObject(worldPos);
            
            // Update the selection (will be nullptr if clicking on empty space, which is great for deselection)
            selectedObject = pickedObject;
        }
    }
    
    ImGui::End();
    ImGui::PopStyleVar();
}
