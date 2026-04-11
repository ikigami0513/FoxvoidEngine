#include "SceneViewPanel.hpp"
#include <rlImGui.h>
#include "ImGuizmo.h"
#include <raymath.h>
#include "extras/IconsFontAwesome6.h"
#include "commands/CommandHistory.hpp"
#include "commands/Transform2dCommand.hpp"

void SceneViewPanel::Draw(RenderTexture2D& sceneTexture, EditorCamera& camera, Scene& activeScene, GameObject*& selectedObject) {
    // Remove inner margins (padding) so the render texture touches the window borders
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Scene View");

    // Gizmo operation state
    // We store the current tool (Translate, Rotate, Scale)
    static ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::TRANSLATE;

    ImGuiIO& io = ImGui::GetIO();
    if (!io.WantTextInput) {
        if (ImGui::IsKeyPressed(ImGuiKey_W)) currentGizmoOperation = ImGuizmo::TRANSLATE;
        if (ImGui::IsKeyPressed(ImGuiKey_E)) currentGizmoOperation = ImGuizmo::ROTATE;
        if (ImGui::IsKeyPressed(ImGuiKey_R)) currentGizmoOperation = ImGuizmo::SCALE;
    }

    // Check if the user's mouse is currently hovering this specific ImGui window
    bool isHovered = ImGui::IsWindowHovered();

    // Don't move the camera if we are currently dragging the Gizmo
    if (!ImGuizmo::IsUsing()) {
        // Pass the hover state to the camera so it only pans/zooms when appropriate
        camera.Update(isHovered);
    }

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

        ImVec2 imagePosAbsolute = ImGui::GetCursorScreenPos();

        // Draw the image with the correctly scaled size
        Rectangle sourceRec = { 0.0f, 0.0f, texWidth, -texHeight };
        rlImGuiImageRect(&sceneTexture.texture, (int)drawSize.x, (int)drawSize.y, sourceRec);

        // We must capture if the image is hovered right here before drawing anything else on top of it 
        bool isImageHovered = ImGui::IsItemHovered();

        // Floating gizmo toolbar
        // Save cursor position to draw the toolbar at the top left of the scene view
        ImVec2 toolbarPos = ImVec2(10.0f, ImGui::GetCursorStartPos().y + 10.0f);
        ImGui::SetCursorPos(toolbarPos);

        // Styling the floating background
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 0.85f));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);

        if (ImGui::BeginChild("GizmoToolbar", ImVec2(146, 48), false, ImGuiWindowFlags_NoScrollbar)) {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
            ImGui::SetCursorPos(ImVec2(4, 4)); // Small internal padding

            // Scale up the font size specifically for this floating window (makes icons bigger)
            ImGui::SetWindowFontScale(1.3f);

            // Lambda helper to draw styled toggle buttons
            auto drawGizmoButton = [](const char* label, ImGuizmo::OPERATION op, ImGuizmo::OPERATION& current) {
                bool isSelected = (current == op);
                if (isSelected) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.90f, 0.45f, 0.10f, 1.0f)); // Foxvoid Orange
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.00f, 0.55f, 0.20f, 1.0f));
                }
                
                if (ImGui::Button(label, ImVec2(40, 40))) current = op;
                
                if (isSelected) ImGui::PopStyleColor(2);
            };

            // Draw the 3 buttons with Icons
            drawGizmoButton(ICON_FA_ARROWS_UP_DOWN_LEFT_RIGHT, ImGuizmo::TRANSLATE, currentGizmoOperation);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Translate (W)");
            ImGui::SameLine();
            
            drawGizmoButton(ICON_FA_ROTATE, ImGuizmo::ROTATE, currentGizmoOperation);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Rotate (E)");
            ImGui::SameLine();
            
            drawGizmoButton(ICON_FA_EXPAND, ImGuizmo::SCALE, currentGizmoOperation);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Scale (R)");
            ImGui::PopStyleVar(2);
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();

        // Gizmos
        if (selectedObject) {
            auto transform = selectedObject->GetComponent<Transform2d>();
            if (transform) {
                // Setup ImGuizmo environment
                ImGuizmo::SetOrthographic(true); // We are in 2D
                ImGuizmo::SetDrawlist();
                ImGuizmo::SetRect(imagePosAbsolute.x, imagePosAbsolute.y, drawSize.x, drawSize.y);

                // Extract Raylib camera matrices
                Camera2D cam2d = camera.GetCamera();
                Matrix viewMatrix = GetCameraMatrix2D(cam2d);
                Matrix projMatrix = MatrixOrtho(0.0f, texWidth, texHeight, 0.0f, -1.0f, 1.0f);

                // Raylib matrices are column-major, just like OpenGL/ImGuizmo expects.
                float view[16] = {
                    viewMatrix.m0, viewMatrix.m1, viewMatrix.m2, viewMatrix.m3,
                    viewMatrix.m4, viewMatrix.m5, viewMatrix.m6, viewMatrix.m7,
                    viewMatrix.m8, viewMatrix.m9, viewMatrix.m10, viewMatrix.m11,
                    viewMatrix.m12, viewMatrix.m13, viewMatrix.m14, viewMatrix.m15
                };
                
                float proj[16] = {
                    projMatrix.m0, projMatrix.m1, projMatrix.m2, projMatrix.m3,
                    projMatrix.m4, projMatrix.m5, projMatrix.m6, projMatrix.m7,
                    projMatrix.m8, projMatrix.m9, projMatrix.m10, projMatrix.m11,
                    projMatrix.m12, projMatrix.m13, projMatrix.m14, projMatrix.m15
                };

                // Extract the object's local Transform into a 4x4 matrix
                float translation[3] = { transform->position.x, transform->position.y, 0.0f };
                float rotation[3] = { 0.0f, 0.0f, transform->rotation };
                float scale[3] = { transform->scale.x, transform->scale.y, 1.0f };
                
                float objMatrix[16];
                ImGuizmo::RecomposeMatrixFromComponents(translation, rotation, scale, objMatrix);

                // Draw and interact with the Gizmo
                // We use WORLD mode for 2D because local rotation can skew 2D scaling weirdly
                ImGuizmo::Manipulate(view, proj, currentGizmoOperation, ImGuizmo::WORLD, objMatrix);

                // Undo / Redo gizmo tracking
                static bool wasUsingGizmo = false;
                static Transform2dState initialTransformState;

                if (ImGuizmo::IsUsing()) {
                    if (!wasUsingGizmo) {
                        // The user JUST clicked on the Gizmo! Save the current state.
                        initialTransformState = { transform->position, transform->rotation, transform->scale };
                        wasUsingGizmo = true;
                    }

                    // Apply the continuous movement to the transform
                    ImGuizmo::DecomposeMatrixToComponents(objMatrix, translation, rotation, scale);
                    transform->position.x = translation[0];
                    transform->position.y = translation[1];
                    transform->rotation = rotation[2];
                    transform->scale.x = scale[0];
                    transform->scale.y = scale[1];

                } else {
                    if (wasUsingGizmo) {
                        // The user JUST released the mouse click! The drag is over.
                        // We record this movement as a single undoable action.
                        
                        Transform2dState currentState = { transform->position, transform->rotation, transform->scale };

                        // We check if it actually moved to avoid empty commands
                        if (initialTransformState.position.x != currentState.position.x ||
                            initialTransformState.position.y != currentState.position.y ||
                            initialTransformState.rotation != currentState.rotation ||
                            initialTransformState.scale.x != currentState.scale.x ||
                            initialTransformState.scale.y != currentState.scale.y)
                        {
                            // Important: We bypass AddCommand because AddCommand calls Execute().
                            // The object is ALREADY in the new position, we just want to push it to history.
                            
                            // To do this cleanly, let's create a custom pointer and push it directly (we'll need a small update to CommandHistory for this later, or we can just use AddCommand and let it set the position again, which is harmless and easier!)
                            CommandHistory::AddCommand(std::make_unique<Transform2dCommand>(selectedObject, initialTransformState, currentState));
                        }

                        wasUsingGizmo = false;
                    }
                }
            }
        }

        // Mouse picking logic
        // Only pick if we hover the image and click the Left Mouse Button
        if (!ImGuizmo::IsOver() && isImageHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
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
