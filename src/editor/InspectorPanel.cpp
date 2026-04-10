#include "InspectorPanel.hpp"

void InspectorPanel::Draw(GameObject*& selectedObject) {
    ImGui::Begin("Inspector");

    if (selectedObject != nullptr) {
        // Rename the entity
        char nameBuffer[256];
        strncpy(nameBuffer, selectedObject->name.c_str(), sizeof(nameBuffer));
        nameBuffer[sizeof(nameBuffer) - 1] = '\0'; 
            
        if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer))) {
            selectedObject->name = std::string(nameBuffer);
        }

        ImGui::Separator();

        // Display existing components
        Component* componentToRemove = nullptr;

        for (const auto& comp : selectedObject->GetComponents()) {
            ImGui::PushID(comp.get());
                
            bool isOpen = ImGui::CollapsingHeader(comp->GetName().c_str(), ImGuiTreeNodeFlags_DefaultOpen);

            // Context Menu for removing components
            if (ImGui::BeginPopupContextItem()) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
                if (ImGui::Selectable("Remove Component")) {
                    componentToRemove = comp.get();
                }
                ImGui::PopStyleColor();
                ImGui::EndPopup();
            }

            if (isOpen) {
                comp->OnInspector();
            }
                
            ImGui::PopID();
        }

        if (componentToRemove != nullptr) {
            selectedObject->RemoveComponent(componentToRemove);
        }

        ImGui::Separator();
        ImGui::Spacing();

        // Add new components
        float windowWidth = ImGui::GetWindowSize().x;
        float buttonWidth = 150.0f;
        ImGui::SetCursorPosX((windowWidth - buttonWidth) * 0.5f);

        if (ImGui::Button("Add Component", ImVec2(buttonWidth, 0))) {
            ImGui::OpenPopup("AddComponentPopup");
        }

        if (ImGui::BeginPopup("AddComponentPopup")) {
            for (const std::string& typeName : ComponentRegistry::registeredTypes) {
                if (ImGui::Selectable(typeName.c_str())) {
                    ComponentRegistry::factories[typeName](*selectedObject);
                }
            }
            ImGui::EndPopup();
        }
    }
    else {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Select an object in the Hierarchy.");
    }

    ImGui::End();
}
