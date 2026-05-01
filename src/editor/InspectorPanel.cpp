#include "InspectorPanel.hpp"
#include "commands/CommandHistory.hpp"
#include "commands/ChangeNameCommand.hpp"
#include <scripting/ScriptableObject.hpp>
#include <scripting/DataManager.hpp>

void InspectorPanel::Draw(GameObject*& selectedObject, py::object& selectedAsset, std::string& selectedAssetPath) {
    ImGui::Begin("Inspector");

    // ==========================================
    // MODE 1: SCRIPTABLE OBJECT (ASSET) INSPECTION
    // ==========================================
    if (!selectedAsset.is_none()) {
        try {
            // Cast the Python object to our C++ base class to access its methods
            ScriptableObject* nativeObj = selectedAsset.cast<ScriptableObject*>();
            if (nativeObj) {
                ImGui::TextDisabled("Asset Path: %s", selectedAssetPath.c_str());
                ImGui::Separator();
                ImGui::Spacing();

                // Call the introspection logic we built earlier
                nativeObj->OnInspector();

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // Centered Save Button
                float windowWidth = ImGui::GetWindowSize().x;
                float buttonWidth = 150.0f;
                ImGui::SetCursorPosX((windowWidth - buttonWidth) * 0.5f);
                
                // When clicked, write the JSON back to the disk
                if (ImGui::Button("Save Asset", ImVec2(buttonWidth, 0))) {
                    DataManager::SaveAsset(selectedAsset, selectedAssetPath);
                }
            }
        } catch (const py::error_already_set& e) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Asset Cast Error:");
            ImGui::TextWrapped("%s", e.what());
        }
    }
    // ==========================================
    // MODE 2: GAMEOBJECT INSPECTION
    // ==========================================
    else if (selectedObject != nullptr) {
        // Rename the entity
        char nameBuffer[256];
        strncpy(nameBuffer, selectedObject->name.c_str(), sizeof(nameBuffer));
        nameBuffer[sizeof(nameBuffer) - 1] = '\0'; 
            
        // Draw the text input box
        ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer));

        static std::string initialName;

        // The editing lifecycle
        if (ImGui::IsItemActivated()) {
            initialName = selectedObject->name;
        }

        if (ImGui::IsItemActive()) {
            selectedObject->name = std::string(nameBuffer); 
        }

        if (ImGui::IsItemDeactivatedAfterEdit()) {
            std::string finalName = std::string(nameBuffer);
            
            if (initialName != finalName) {
                selectedObject->name = initialName;
                CommandHistory::AddCommand(std::make_unique<ChangeNameCommand>(selectedObject, initialName, finalName));
            }
        } 
        else if (ImGui::IsItemDeactivated() && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            selectedObject->name = initialName;
        }

        ImGui::Separator();

        // Display existing components
        Component* componentToRemove = nullptr;

        for (const auto& comp : selectedObject->GetAllComponents()) {
            ImGui::PushID(comp.get());
                
            bool isOpen = ImGui::CollapsingHeader(comp->GetName().c_str(), ImGuiTreeNodeFlags_DefaultOpen);

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
        float buttonWidth = 200.0f;
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
    // ==========================================
    // MODE 3: NOTHING SELECTED
    // ==========================================
    else {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Select an object or an asset.");
    }

    ImGui::End();
}
