#include "InspectorPanel.hpp"
#include "commands/CommandHistory.hpp"
#include "commands/ChangeNameCommand.hpp"

void InspectorPanel::Draw(GameObject*& selectedObject) {
    ImGui::Begin("Inspector");

    if (selectedObject != nullptr) {
        // Rename the entity
        char nameBuffer[256];
        strncpy(nameBuffer, selectedObject->name.c_str(), sizeof(nameBuffer));
        nameBuffer[sizeof(nameBuffer) - 1] = '\0'; 
            
        // Draw the text input box (without the 'if' condition)
        ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer));

        // Static variable to hold the original name during the entire typing process
        static std::string initialName;

        // The editing lifecycle
        if (ImGui::IsItemActivated()) {
            // The user JUST clicked into the text box. Save the initial state.
            initialName = selectedObject->name;
        }

        if (ImGui::IsItemActive()) {
            // The user is actively typing. Update the object for real-time visual feedback (e.g., in the Hierarchy).
            selectedObject->name = std::string(nameBuffer); 
        }

        if (ImGui::IsItemDeactivatedAfterEdit()) {
            // The user finished editing (pressed Enter or clicked away).
            std::string finalName = std::string(nameBuffer);
            
            // Verify the name actually changed (they didn't just click in and click out)
            if (initialName != finalName) {
                // CRUCIAL TRICK: Temporarily revert the object to its old name.
                // Why? Because when we add the command right below, it instantly calls 
                // Execute(), which will re-apply the new finalName for us!
                selectedObject->name = initialName;
                
                CommandHistory::AddCommand(std::make_unique<ChangeNameCommand>(selectedObject, initialName, finalName));
            }
        } 
        else if (ImGui::IsItemDeactivated() && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            // Bonus: If the user presses Escape while typing, cancel everything!
            selectedObject->name = initialName;
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
