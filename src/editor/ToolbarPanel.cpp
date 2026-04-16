#include "ToolbarPanel.hpp"

void ToolbarPanel::Draw(Scene& activeScene, GameObject*& selectedObject, bool& isPlaying, nlohmann::json& sceneBackup, bool& focusGameWindow) {
    ImGui::Begin("Toolbar");

    // Change button color and text based on the current state
    if (!isPlaying) {
        // Green Play Button
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
        if (ImGui::Button("PLAY")) {
            isPlaying = true;
            std::cout << "[Editor] Entered PLAY mode." << std::endl;
            
            selectedObject = nullptr;
            sceneBackup = activeScene.Serialize();
            activeScene.Start();

            // Set the flag to true so the Game view can grab focus on its next draw call
            focusGameWindow = true;
        }
        ImGui::PopStyleColor();
    } else {
        // Red Stop Button
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button("STOP")) {
            isPlaying = false;
            std::cout << "[Editor] Entered EDIT mode." << std::endl;
            
            selectedObject = nullptr;
            activeScene.Deserialize(sceneBackup, false);
        }
        ImGui::PopStyleColor();
    }
    
    ImGui::End();
}
