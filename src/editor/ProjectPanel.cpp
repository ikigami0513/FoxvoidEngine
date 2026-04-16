#include "ProjectPanel.hpp"
#include <iostream>

#ifndef STANDALONE_MODE
#include <imgui.h>
#include <extras/IconsFontAwesome6.h>
#endif

// Helper to generate a default Python script template
void CreatePythonScript(const fs::path& directory, const std::string& scriptName) {
    // Force the .py extension
    std::string filename = scriptName;
    if (filename.find(".py") == std::string::npos) {
        filename += ".py";
    }

    fs::path fullPath = directory / filename;

    // Check if file already exists to prevent accidental overwrites
    if (fs::exists(fullPath)) {
        std::cerr << "[ProjectPanel] Error: file " << filename << " already exists!" << std::endl;
        return;
    }

    std::ofstream file(fullPath);
    if (file.is_open()) {
        // Derive class name from filename (e.g., "PlayerController.py" -> "PlayerController")
        std::string className = filename.substr(0, filename.find_last_of('.'));

        // Capitalize the first letter for Python class convention
        if (!className.empty()) {
            className[0] = std::toupper(className[0]);
        }

        // Write the boilerplate code
        file << "from foxvoid import *\n\n";
        file << "class " << className << "(Component):\n";
        file << "    def __init__(self):\n";
        file << "        super().__init__()\n\n";
        file << "    def start(self):\n";
        file << "        pass\n\n";
        file << "    def update(self, delta_time: float):\n";
        file << "        pass\n";

        file.close();
        std::cout << "[ProjectPanel] Created new script: " << fullPath.string() << std::endl;
    }
    else {
        std::cerr << "[ProjectPanel] Error: Could not create file " << fullPath.string() << std::endl;
    }
}

void ProjectPanel::Draw(Scene& activeScene, GameObject*& selectedObject, const fs::path& assetsPath, std::string& currentScenePath) {
    ImGui::Begin("Project");

    // Initialize the current directory the very first time the panel is drawn
    if (m_currentDirectory.empty()) {
        m_currentDirectory = assetsPath;
    }

    // Toolbar
    // Button to switch views
    if (ImGui::Button(m_isTreeView ? ICON_FA_FOLDER_OPEN " Switch to Explorer" : ICON_FA_SITEMAP " Switch to Tree")) {
        m_isTreeView = !m_isTreeView;
    }

    ImGui::SameLine();

    if (!m_isTreeView) {
        // Explorer View Toolbar: We need a "Back" button to go up a directory
        ImGui::BeginDisabled(m_currentDirectory == assetsPath); // Disable if we are at the root
        if (ImGui::Button(ICON_FA_ARROW_LEFT)) {
            m_currentDirectory = m_currentDirectory.parent_path();
        }
        ImGui::EndDisabled();

        ImGui::SameLine();
        ImGui::TextDisabled("%s", m_currentDirectory.string().c_str());
    } else {
        // Tree View Toolbar: Just show the root path
        ImGui::TextDisabled("Path: %s", assetsPath.string().c_str());
    }

    ImGui::Separator();

    // Viewport
    // Add a little padding inside the window
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4, 4));

    if (m_isTreeView) {
        DrawDirectoryNode(activeScene, selectedObject, assetsPath, currentScenePath);
    } else {
        DrawExplorerView(activeScene, selectedObject, currentScenePath);
    }

    ImGui::PopStyleVar();

    // Trigger the modal at the root id stack
    if (m_requestScriptModal) {
        ImGui::OpenPopup("New Python Script");
        m_requestScriptModal = false; // Reset the flag immediately
    }

    // Modal
    if (ImGui::BeginPopupModal("New Python Script", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        static char scriptName[64] = "NewScript";
        ImGui::InputText("Name", scriptName, IM_ARRAYSIZE(scriptName));
                
        if (ImGui::Button("Create", ImVec2(120, 0))) {
            CreatePythonScript(m_currentDirectory, scriptName);
            ImGui::CloseCurrentPopup();
            
            // Reset buffer for the next time
            scriptName[0] = '\0';
            strcpy(scriptName, "NewScript");
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    ImGui::End();
}

void ProjectPanel::DrawExplorerView(Scene& activeScene, GameObject*& selectedObject, std::string& currentScenePath) {
    // If the directory does not exist, stop right here
    if (!fs::exists(m_currentDirectory)) return;

    // Global Right-Click menu for the current directory
    if (ImGui::BeginPopupContextWindow("ExplorerContextPopup")) {
        if (ImGui::BeginMenu(ICON_FA_PLUS " Create")) {
            if (ImGui::MenuItem(ICON_FA_FILE_CODE " Python Script")) {
                m_requestScriptModal = true;
            }
            ImGui::EndMenu();
        }
        ImGui::EndPopup();
    }

    // Iterate through all items in the current directory
    for (const auto& entry : fs::directory_iterator(m_currentDirectory)) {
        const auto& filename = entry.path().filename().string();
        std::string extension = entry.path().extension().string();

        // Filter exclusions
        // Ignore Python cache directories
        if (entry.is_directory() && filename == "__pycache__") continue;
        // Ignore compiled python files
        if (!entry.is_directory() && extension == ".pyc") continue;
        // Ignore hidden files/folders (starting with a dot, like .git or .vscode)
        if (!filename.empty() && filename[0] == '.') continue;

        bool isDir = entry.is_directory();
        
        // Add a small visual icon prefix based on the file type
        std::string icon = isDir ? ICON_FA_FOLDER : ICON_FA_FILE;
        if (!isDir) {
            if (extension == ".scene") icon = ICON_FA_CUBES;
            else if (extension == ".prefab") icon = ICON_FA_BOX;
            else if (extension == ".py") icon = ICON_FA_FILE_CODE;
            else if (extension == ".png") icon = ICON_FA_IMAGE;
        }

        std::string displayName = icon + "  " + filename;

        // Draw the item as a selectable row
        if (ImGui::Selectable(displayName.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
            // INTERACTION: What happens when we interact with this file?
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                if (isDir) {
                    // It's a FOLDER -> Enter the folder!
                    m_currentDirectory /= filename; 
                } else if (extension == ".scene") {
                    // It's a FILE -> If it's a scene, load it!
                    std::cout << "[Editor] Loading scene via Project Browser: " << filename << std::endl;
                    
                    // Safety: Clear the editor selection before changing the scene
                    selectedObject = nullptr; 
                    currentScenePath = entry.path().string();
                    
                    // Load the new scene
                    activeScene.LoadFromFile(currentScenePath);
                }
            }
        }

        // Drag and drop source (For Files Only in Explorer view)
        // If the user clicks and drags this item, initialize the drag payload
        if (!isDir && ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
            // Get the relative path of the file (e.g., "assets/textures/player.png")
            std::string itemPath = entry.path().string();

            // Set the payload data. 
            // "CONTENT_BROWSER_ITEM" is our custom identifier.
            // We add +1 to the size to ensure the null-terminator ('\0') is included.
            ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", itemPath.c_str(), itemPath.size() + 1);
            
            // Display a tooltip right next to the mouse cursor while dragging
            ImGui::Text("%s Drop %s", icon.c_str(), filename.c_str());
            
            ImGui::EndDragDropSource();
        }
    }
}

void ProjectPanel::DrawDirectoryNode(Scene& activeScene, GameObject*& selectedObject, const fs::path& path, std::string& currentScenePath) {
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
            std::string folderName = ICON_FA_FOLDER " " + filename;
            
            bool isOpen = ImGui::TreeNodeEx(folderName.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth);

            // Context menu for specific folders in Tree View
            // Push an ID so multiple folders don't share the same popup state
            ImGui::PushID(entry.path().string().c_str());
            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem(ICON_FA_FILE_CODE " Create Python Script")) {
                    // Update our tracked directory to the one clicked, then open the global modal
                    m_currentDirectory = entry.path();
                    m_requestScriptModal = true;
                }
                ImGui::EndPopup();
            }
            ImGui::PopID();

            // Create a collapsible tree node
            if (isOpen) {
                // Recursive call to read the contents of this subfolder
                DrawDirectoryNode(activeScene, selectedObject, entry.path(), currentScenePath); 
                ImGui::TreePop(); // Close the node
            }
        } else {
            // It's a FILE
            // Draw it as a "Leaf" (no children to expand)
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
            
            // Add a small visual icon prefix based on the file type (optional but looks nice)
            std::string icon = ICON_FA_FILE;
            if (extension == ".scene") icon = ICON_FA_CUBES;
            else if (extension == ".prefab") icon = ICON_FA_BOX;
            else if (extension == ".py") icon = ICON_FA_FILE_CODE;
            else if (extension == ".png") icon = ICON_FA_IMAGE;

            std::string displayName = icon + "  " + filename;
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
                ImGui::Text("%s Drop %s", icon.c_str(), filename.c_str());
                
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
                        currentScenePath = entry.path().string();
                        
                        // Load the new scene
                        activeScene.LoadFromFile(currentScenePath);
                    }
                }
            }
        }
    }
}
