#include "HierarchyPanel.hpp"
#include "commands/CommandHistory.hpp"
#include "commands/CreateObjectCommand.hpp"
#include "commands/DeleteObjectCommand.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>

// Helper to prevent infinite loops: checks if the target is a descendant of the potential ancestor
bool IsDescendant(GameObject* target, GameObject* potentialAncestor) {
    GameObject* current = target;
    while (current != nullptr) {
        if (current == potentialAncestor) return true;
        current = current->GetParent();
    }
    return false;
}

void HierarchyPanel::Draw(Scene& activeScene, GameObject*& selectedObject) {
    ImGui::Begin("Hierarchy");

    // Button to create a new root game object on the fly
    if (ImGui::Button("+ Add Game Object", ImVec2(-1, 0))) {
        GameObject* newObj = activeScene.CreateGameObject("New Game Object");
        CommandHistory::AddCommand(std::make_unique<CreateObjectCommand>(activeScene, newObj));
        selectedObject = newObj;
    }

    ImGui::Separator();

    // Deferred deletion pointer
    GameObject* objectToDelete = nullptr;

    // Iterate through ROOT objects only (objects with no parent)
    for (const auto& go : activeScene.GetGameObjects()) {
        if (go->GetParent() == nullptr) {
            DrawNode(activeScene, go.get(), selectedObject, objectToDelete);
        }
    }

    // Empty space drop target and deselection
    ImVec2 availableSpace = ImGui::GetContentRegionAvail();

    if (availableSpace.y > 0.0f) {
        ImGui::InvisibleButton("##HierarchyEmptySpace", availableSpace);

        // Click on empty space to deselect
        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            selectedObject = nullptr;
        }

        // Handle Drag & Drop (Dropping in empty space to UNPARENT an object)
        if (ImGui::BeginDragDropTarget()) {
            
            // Unparent an existing hierarchy object
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_GAMEOBJECT")) {
                GameObject* droppedNode = *(GameObject**)payload->Data;
                droppedNode->SetParent(nullptr); // Move to root
            }
            
            // Instantiate a prefab from the Content Browser
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM")) {
                std::string droppedPath = (const char*)payload->Data;
                std::filesystem::path fsPath(droppedPath);
                
                if (fsPath.extension() == ".prefab") {
                    GameObject* newObj = activeScene.Instantiate(droppedPath);
                    if (newObj) {
                        selectedObject = newObj; 
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }
    }

    // Execute the deletion safely outside the loop
    if (objectToDelete) {
        CommandHistory::AddCommand(std::make_unique<DeleteObjectCommand>(activeScene, objectToDelete));
    }

    ImGui::End();
}

void HierarchyPanel::DrawNode(Scene& activeScene, GameObject* node, GameObject*& selectedObject, GameObject*& objectToDelete) {
    ImGuiTreeNodeFlags flags = ((selectedObject == node) ? ImGuiTreeNodeFlags_Selected : 0);
    flags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen;

    bool isLeaf = node->GetChildren().empty();
    if (isLeaf) {
        // If it has no children, render it as a flat leaf without an expansion arrow
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    std::string displayName = node->name.empty() ? "<Unnamed>" : node->name;

    // Draw the tree node
    bool isOpen = ImGui::TreeNodeEx((void*)node, flags, "%s", displayName.c_str());

    // Selection
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
        selectedObject = node;
    }

    // Drag & Drop SOURCE (Picking up this node)
    if (ImGui::BeginDragDropSource()) {
        ImGui::SetDragDropPayload("HIERARCHY_GAMEOBJECT", &node, sizeof(GameObject*));
        ImGui::Text("Move %s", displayName.c_str());
        ImGui::EndDragDropSource();
    }

    // Drag & Drop TARGET (Dropping ONTO this node to make a child)
    if (ImGui::BeginDragDropTarget()) {
        
        // Handle reparenting an existing GameObject
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_GAMEOBJECT")) {
            GameObject* droppedNode = *(GameObject**)payload->Data;
            
            // Safety: A node cannot be parented to itself or to its own children
            if (droppedNode != node && !IsDescendant(node, droppedNode)) {
                droppedNode->SetParent(node);
            }
        }

        // Handle dropping a prefab onto this node
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM")) {
            std::string droppedPath = (const char*)payload->Data;
            std::filesystem::path fsPath(droppedPath);
            
            if (fsPath.extension() == ".prefab") {
                GameObject* newObj = activeScene.Instantiate(droppedPath);
                if (newObj) {
                    newObj->SetParent(node);
                    CommandHistory::AddCommand(std::make_unique<CreateObjectCommand>(activeScene, newObj));
                    selectedObject = newObj;
                }
            }
        }
        ImGui::EndDragDropTarget();
    }

    // Context Menu (Right Click)
    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::Selectable("Create Empty Child")) {
            GameObject* childObj = activeScene.CreateGameObject("New Child");
            childObj->SetParent(node);
            CommandHistory::AddCommand(std::make_unique<CreateObjectCommand>(activeScene, childObj));
            selectedObject = childObj;
        }

        ImGui::Separator();

        if (ImGui::Selectable("Save as Prefab")) {
            std::filesystem::create_directories("assets/prefabs");
            nlohmann::json prefabJson = node->Serialize();
            std::string path = "assets/prefabs/" + node->name + ".prefab";
            std::ofstream file(path);

            if (file.is_open()) {
                file << prefabJson.dump(4);
                file.close();
                std::cout << "[Editor] Saved prefab successfully: " << path << std::endl;
            } else {
                std::cerr << "[Editor] Error: Could not save prefab to " << path << std::endl;
            }
        }

        if (ImGui::Selectable("Delete Game Object")) {
            objectToDelete = node;
            if (selectedObject == node) selectedObject = nullptr;
        }
        ImGui::EndPopup();
    }

    // Render Children (Recursion)
    if (isOpen && !isLeaf) {
        // We make a temporary copy of the children vector to iterate safely, 
        // preventing iterator invalidation if the list changes during ImGui rendering.
        std::vector<GameObject*> childrenCopy = node->GetChildren();
        
        for (auto* child : childrenCopy) {
            DrawNode(activeScene, child, selectedObject, objectToDelete);
        }
        
        ImGui::TreePop(); // Close the Tree Node
    }
}
