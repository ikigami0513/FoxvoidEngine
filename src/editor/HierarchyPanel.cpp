#include "HierarchyPanel.hpp"

void HierarchyPanel::Draw(Scene& activeScene, GameObject*& selectedObject) {
    ImGui::Begin("Hierarchy");

    // Button to create a new game object on the fly
    if (ImGui::Button("+ Add Game Object", ImVec2(-1, 0))) {
        selectedObject = activeScene.CreateGameObject("New Game Object");
    }

    ImGui::Separator();

    // Iterate through all alive objects in the active scene
    for (const auto& go : activeScene.GetGameObjects()) {
        bool isSelected = (selectedObject == go.get());

        ImGui::PushID(go.get());
        std::string displayName = go->name.empty() ? "<Unnamed>" : go->name;

        if (ImGui::Selectable(displayName.c_str(), isSelected)) {
            selectedObject = go.get();
        }

        // Context Menu (Right Click)
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::Selectable("Delete Entity")) {
                go->Destroy();
                // Safety: Clear selection if the deleted object was selected
                if (isSelected) selectedObject = nullptr;
            }
            ImGui::EndPopup();
        }

        ImGui::PopID();
    }

    // Click on empty space to deselect
    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        selectedObject = nullptr;
    }

    ImGui::End();
}
