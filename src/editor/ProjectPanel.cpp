#include "ProjectPanel.hpp"
#include "scripting/DataManager.hpp"
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

#ifndef STANDALONE_MODE
#include <imgui.h>
#include <extras/IconsFontAwesome6.h>
#endif

bool IsScriptableObjectFile(const fs::path& filepath) {
    std::ifstream file(filepath);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            if (line.find("class ") != std::string::npos) {
                if (line.find("(ScriptableObject)") != std::string::npos) {
                    return true;
                }
                return false;
            }
        }
    }
    return false;
}

// Helper to generate a default Python script template
void CreatePythonScript(const fs::path& directory, const std::string& scriptName, ScriptType type) {
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
        
        if (type == ScriptType::Component) {
            file << "class " << className << "(Component):\n";
            file << "    def __init__(self):\n";
            file << "        super().__init__()\n\n";
            file << "    def start(self):\n";
            file << "        pass\n\n";
            file << "    def update(self, delta_time: float):\n";
            file << "        pass\n";
        } else if (type == ScriptType::ScriptableObject) {
            file << "class " << className << "(ScriptableObject):\n";
            file << "    def __init__(self):\n";
            file << "        super().__init__()\n";
            file << "        # Add your custom data properties here\n";
        }

        file.close();
        std::cout << "[ProjectPanel] Created new script: " << fullPath.string() << std::endl;
    }
    else {
        std::cerr << "[ProjectPanel] Error: Could not create file " << fullPath.string() << std::endl;
    }
}

std::string ExtractClassNameFromFile(const fs::path& filepath) {
    std::ifstream file(filepath);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            // Looking for "class ClassName(ScriptableObject):"
            size_t classPos = line.find("class ");
            if (classPos != std::string::npos) {
                size_t start = classPos + 6;
                size_t end = line.find_first_of("(: \r\n", start);
                if (end != std::string::npos) {
                    return line.substr(start, end - start);
                }
            }
        }
    }
    return ""; // Fallback
}

// Helper to generate a default ScriptableObject (.asset) JSON file
void CreateAssetFile(const fs::path& directory, const std::string& filename, const std::string& assetId, const std::string& scriptName, const std::string& className) {
    // Force the .asset extension
    std::string finalFilename = filename;
    if (finalFilename.find(".asset") == std::string::npos) {
        finalFilename += ".asset";
    }

    fs::path fullPath = directory / finalFilename;

    // Check if file already exists
    if (fs::exists(fullPath)) {
        std::cerr << "[ProjectPanel] Error: file " << finalFilename << " already exists!" << std::endl;
        return;
    }

    // Build the initial JSON structure representing the ScriptableObject
    nlohmann::json j;
    j["assetId"] = assetId;
    j["name"] = filename; 
    j["scriptName"] = scriptName;
    j["className"] = className;
    j["properties"] = nlohmann::json::object(); // Empty properties dictionary to start

    std::ofstream file(fullPath);
    if (file.is_open()) {
        // Save with 4 spaces of indentation for readability
        file << j.dump(4);
        file.close();
        std::cout << "[ProjectPanel] Created new asset: " << fullPath.string() << std::endl;
    } else {
        std::cerr << "[ProjectPanel] Error: Could not create file " << fullPath.string() << std::endl;
    }
}

// UPDATED SIGNATURE
void ProjectPanel::Draw(Scene& activeScene, GameObject*& selectedObject, pybind11::object& selectedAsset, std::string& selectedAssetPath, const fs::path& assetsPath, std::string& currentScenePath) {
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
        // UPDATED CALL
        DrawDirectoryNode(activeScene, selectedObject, selectedAsset, selectedAssetPath, assetsPath, currentScenePath);
    } else {
        // UPDATED CALL
        DrawExplorerView(activeScene, selectedObject, selectedAsset, selectedAssetPath, currentScenePath);
    }

    ImGui::PopStyleVar();

    // Trigger the modals at the root id stack safely
    if (m_requestScriptModal) {
        ImGui::OpenPopup("New Python Script");
        m_requestScriptModal = false; // Reset the flag immediately
    }

    if (m_requestAssetModal) {
        ImGui::OpenPopup("New Scriptable Object");
        m_requestAssetModal = false; // Reset the flag immediately
    }

    // Modal
    if (ImGui::BeginPopupModal("New Python Script", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        static char scriptName[64] = "NewScript";
        ImGui::InputText("Name", scriptName, IM_ARRAYSIZE(scriptName));
        
        ImGui::TextDisabled("Template: %s", m_pendingScriptType == ScriptType::Component ? "Component" : "Scriptable Object");
        ImGui::Spacing();
                
        if (ImGui::Button("Create", ImVec2(120, 0))) {
            CreatePythonScript(m_currentDirectory, scriptName, m_pendingScriptType);
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

    // Modal: Scriptable Object (.asset)
    if (ImGui::BeginPopupModal("New Scriptable Object", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        static char assetFileName[64] = "NewData";
        static char assetId[64] = "data_01";

        ImGui::InputText("File Name (.asset)", assetFileName, IM_ARRAYSIZE(assetFileName));
        ImGui::InputText("Asset ID", assetId, IM_ARRAYSIZE(assetId));
        
        ImGui::Separator();
        ImGui::TextDisabled("Python Binding");
        
        // We disable these fields because they are automatically filled by right-clicking the script!
        ImGui::BeginDisabled();
        ImGui::InputText("Module", m_assetPrefillModule, sizeof(m_assetPrefillModule));
        ImGui::InputText("Class", m_assetPrefillClass, sizeof(m_assetPrefillClass));
        ImGui::EndDisabled();
                
        if (ImGui::Button("Create", ImVec2(120, 0))) {
            CreateAssetFile(m_currentDirectory, assetFileName, assetId, m_assetPrefillModule, m_assetPrefillClass);
            ImGui::CloseCurrentPopup();
            
            // Reset buffers for the next time
            strcpy(assetFileName, "NewData");
            strcpy(assetId, "data_01");
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    ImGui::End();
}

// UPDATED SIGNATURE
void ProjectPanel::DrawExplorerView(Scene& activeScene, GameObject*& selectedObject, pybind11::object& selectedAsset, std::string& selectedAssetPath, std::string& currentScenePath) {
    // If the directory does not exist, stop right here
    if (!fs::exists(m_currentDirectory)) return;

    // Global Right-Click menu for the current directory
    if (ImGui::BeginPopupContextWindow("ExplorerContextPopup")) {
        if (ImGui::BeginMenu(ICON_FA_PLUS " Create")) {
            // Sub-menu for Python Scripts
            if (ImGui::BeginMenu(ICON_FA_FILE_CODE " Python Script")) {
                if (ImGui::MenuItem("Component")) {
                    m_pendingScriptType = ScriptType::Component;
                    m_requestScriptModal = true;
                }
                if (ImGui::MenuItem("Scriptable Object Class")) {
                    m_pendingScriptType = ScriptType::ScriptableObject;
                    m_requestScriptModal = true;
                }
                ImGui::EndMenu();
            }
            
            // Note: We removed the direct Asset creation from the global menu here
            // to force the user to right-click on a script instead, preventing empty/invalid bindings.
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
            else if (extension == ".asset") icon = ICON_FA_DATABASE;
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
                    activeScene.LoadFromFile(currentScenePath, false);
                } else if (extension == ".asset") {
                    // NEW: Load the Data Asset into the Inspector
                    std::cout << "[Editor] Inspecting asset: " << filename << std::endl;
                    
                    selectedObject = nullptr; // Prioritize asset over hierarchy object
                    selectedAssetPath = entry.path().string();
                    selectedAsset = DataManager::LoadAsset(selectedAssetPath);
                }
            }
        }

        // Context menu specifically for Python scripts (Explorer View)
        if (!isDir && extension == ".py") {
            if (ImGui::BeginPopupContextItem()) {
                
                if (IsScriptableObjectFile(entry.path())) {
                    if (ImGui::MenuItem(ICON_FA_DATABASE " Create Asset from this Script")) {
                        // 1. Get the module name (filename without .py)
                        std::string modName = fs::path(filename).stem().string();
                        strncpy(m_assetPrefillModule, modName.c_str(), sizeof(m_assetPrefillModule));

                        // 2. Read the file to find the class name
                        std::string clsName = ExtractClassNameFromFile(entry.path());
                        if (clsName.empty()) {
                            // Fallback: capitalize filename if extraction failed
                            clsName = modName;
                            if (!clsName.empty()) clsName[0] = std::toupper(clsName[0]);
                        }
                        strncpy(m_assetPrefillClass, clsName.c_str(), sizeof(m_assetPrefillClass));

                        // 3. Trigger the modal
                        m_requestAssetModal = true;
                    }
                }
                else {
                    ImGui::TextDisabled(ICON_FA_GEARS " Component Script");
                }
                
                ImGui::EndPopup();
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

// UPDATED SIGNATURE
void ProjectPanel::DrawDirectoryNode(Scene& activeScene, GameObject*& selectedObject, pybind11::object& selectedAsset, std::string& selectedAssetPath, const fs::path& path, std::string& currentScenePath) {
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
                // Sub-menu for Python Scripts in Tree View
                if (ImGui::BeginMenu(ICON_FA_FILE_CODE " Create Python Script")) {
                    if (ImGui::MenuItem("Component")) {
                        m_currentDirectory = entry.path();
                        m_pendingScriptType = ScriptType::Component;
                        m_requestScriptModal = true;
                    }
                    if (ImGui::MenuItem("Scriptable Object Class")) {
                        m_currentDirectory = entry.path();
                        m_pendingScriptType = ScriptType::ScriptableObject;
                        m_requestScriptModal = true;
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndPopup();
            }
            ImGui::PopID();

            // Create a collapsible tree node
            if (isOpen) {
                // Recursive call to read the contents of this subfolder
                // UPDATED CALL
                DrawDirectoryNode(activeScene, selectedObject, selectedAsset, selectedAssetPath, entry.path(), currentScenePath); 
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
            else if (extension == ".asset") icon = ICON_FA_DATABASE;

            std::string displayName = icon + "  " + filename;
            ImGui::TreeNodeEx(displayName.c_str(), flags);
            
            // Context menu specifically for Python scripts (Tree View)
            if (!entry.is_directory() && extension == ".py") {
                if (ImGui::BeginPopupContextItem()) {
                    
                    if (IsScriptableObjectFile(entry.path())) {
                        if (ImGui::MenuItem(ICON_FA_DATABASE " Create Asset from this Script")) {
                            m_currentDirectory = entry.path().parent_path();
                            
                            std::string modName = fs::path(filename).stem().string();
                            strncpy(m_assetPrefillModule, modName.c_str(), sizeof(m_assetPrefillModule));

                            std::string clsName = ExtractClassNameFromFile(entry.path());
                            if (clsName.empty()) {
                                clsName = modName;
                                if (!clsName.empty()) clsName[0] = std::toupper(clsName[0]);
                            }
                            strncpy(m_assetPrefillClass, clsName.c_str(), sizeof(m_assetPrefillClass));

                            m_requestAssetModal = true;
                        }
                    }
                    else {
                        ImGui::TextDisabled(ICON_FA_GEARS " Component Script");
                    }

                    ImGui::EndPopup();
                }
            }

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
                        activeScene.LoadFromFile(currentScenePath, false);
                    } else if (extension == ".asset") {
                        // NEW: Load the Data Asset into the Inspector
                        std::cout << "[Editor] Inspecting asset: " << filename << std::endl;
                        
                        selectedObject = nullptr; // Prioritize asset over hierarchy object
                        selectedAssetPath = entry.path().string();
                        selectedAsset = DataManager::LoadAsset(selectedAssetPath);
                    }
                }
            }
        }
    }
}
