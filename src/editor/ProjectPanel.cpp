#include "ProjectPanel.hpp"

void ProjectPanel::Draw(Scene& activeScene, GameObject*& selectedObject, const fs::path& assetsPath) {
    ImGui::Begin("Project");

    ImGui::TextDisabled("Path: %s", assetsPath.string().c_str());
    ImGui::Separator();

    DrawDirectoryNode(activeScene, selectedObject, assetsPath);

    ImGui::End();
}

void ProjectPanel::DrawDirectoryNode(Scene& activeScene, GameObject*& selectedObject, const fs::path& path) {
    // If the directory does not exist, stop right here
    if (!fs::exists(path)) return;

    // Iterate through all items in the current directory
    for (const auto& entry : fs::directory_iterator(path)) {
        const auto& filename = entry.path().filename().string();
        std::string extension = entry.path().extension().string();

        // Filter exclusions

        // Ignore Python cache directories
        if (entry.is_directory() && filename == "__pycache__") continue;

        // Ignore compiled python files
        if (!entry.is_directory() && extension == ".pyc") continue;

        // Ignore hidden files/folders (starting with a dot, like .git or .vscode)
        if (!filename.empty() && filename[0] == '.') continue;

        if (entry.is_directory()) {
            // It's a FOLDER
            // Create a collapsible tree node
            if (ImGui::TreeNodeEx(filename.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth)) {
                // Recursive call to read the contents of this subfolder
                DrawDirectoryNode(activeScene, selectedObject, entry.path()); 
                ImGui::TreePop(); // Close the node
            }
        } else {
            // It's a FILE
            // Draw it as a "Leaf" (no children to expand)
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
            
            // Add a small visual icon prefix based on the file type (optional but looks nice)
            std::string displayName = "  " + filename;

            if (extension == ".scene") displayName = "[Scene] " + filename;
            else if (extension == ".prefab") displayName = "[Prefab] " + filename;
            else if (extension == ".py") displayName = "[Script] " + filename;
            else if (extension == ".png") displayName = "[Image] " + filename;

            ImGui::TreeNodeEx(displayName.c_str(), flags);
            
            // Drag and drop source
            // If the user clicks and drags this item, initialize the drag payload
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                // Get the relative path of the file (e.g., "assets/textures/player.png")
                std::string itemPath = entry.path().string();

                // Set the payload data. 
                // "CONTENT_BROWSER_ITEM" is our custom identifier.
                // We add +1 to the size to ensure the null-terminator ('\0') is included.
                ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", itemPath.c_str(), itemPath.size() + 1);
                
                // Display a tooltip right next to the mouse cursor while dragging
                ImGui::Text("Drop %s", filename.c_str());
                
                ImGui::EndDragDropSource();
            }

            // INTERACTION: What happens when we interact with this file?
            if (ImGui::IsItemHovered()) {
                // Double left-click
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    
                    // If it's a scene, load it!
                    if (extension == ".scene") {
                        std::cout << "[Editor] Loading scene via Project Browser: " << filename << std::endl;
                        
                        // Safety: Clear the editor selection before changing the scene
                        selectedObject = nullptr; 
                        
                        // Load the new scene
                        activeScene.LoadFromFile(entry.path().string());
                    }
                }
            }
        }
    }
}
