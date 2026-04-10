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
            if (ImGui::Selectable("Save as Prefab")) {
                // Ensure the prefabs directory exists before trying to save
                std::filesystem::create_directories("assets/prefabs");

                // Serialize only this specific GameObject, not the whole scene
                nlohmann::json prefabJson = go->Serialize();

                // Create the file path using the object's name
                std::string path = "assets/prefabs/" + go->name + ".prefab";
                std::ofstream file(path);

                if (file.is_open()) {
                    // Save the JSON with an indentation of 4 spaces for readability
                    file << prefabJson.dump(4);
                    file.close();
                    std::cout << "[Editor] Saved prefab successfully: " << path << std::endl;
                }
                else {
                    std::cerr << "[Editor] Error: Could not save prefab to " << path << std::endl;
                }
            }

            if (ImGui::Selectable("Delete Game Object")) {
                go->Destroy();
                // Safety: Clear selection if the deleted object was selected
                if (isSelected) selectedObject = nullptr;
            }
            ImGui::EndPopup();
        }

        // Allow dropping a prefab directly onto a existing item in the list
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM")) {
                std::string droppedPath = (const char*)payload->Data;
                std::filesystem::path fsPath(droppedPath);
                
                // If it's a Prefab file, spawn it!
                if (fsPath.extension() == ".prefab") {
                    GameObject* newObj = activeScene.Instantiate(droppedPath);
                    if (newObj) selectedObject = newObj; // Auto-select the newly spawned prefab
                }
            }
            ImGui::EndDragDropTarget();
        } 

        ImGui::PopID();
    }

    // Empty space drop target and deselection
    // Get the remaining empty space in the Hierarchy window
    ImVec2 availableSpace = ImGui::GetContentRegionAvail();

    // We only create the invisible button if there is actual empty space left
    if (availableSpace.y > 0.0f) {
        // Create an invisible button that fills the rest of the window
        ImGui::InvisibleButton("##HierarchyEmptySpace", availableSpace);

        // Click on empty space to deselect
        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            selectedObject = nullptr;
        }

        // Handle Drag & Drop (Dropping in empty space)
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM")) {
                std::string droppedPath = (const char*)payload->Data;
                std::filesystem::path fsPath(droppedPath);
                
                // Instantiate the prefab if a JSON file was dropped
                if (fsPath.extension() == ".prefab") {
                    GameObject* newObj = activeScene.Instantiate(droppedPath);
                    
                    // Automatically select the new object to immediately see it in the Inspector
                    if (newObj) {
                        selectedObject = newObj; 
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }
    }

    ImGui::End();
}
