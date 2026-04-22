#include "ProjectPanel.hpp"
#include "scripting/DataManager.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>
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

// Helper function to handle Drag and Drop payload moving
void HandleDragAndDropMove(const fs::path& targetDirectory) {
    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM")) {
        std::string sourcePathStr = (const char*)payload->Data;
        fs::path sourcePath = sourcePathStr;
        
        // Construct the new path
        fs::path destinationPath = targetDirectory / sourcePath.filename();

        // Prevent moving a folder into itself or if the file is already there
        if (sourcePath != destinationPath && destinationPath.string().find(sourcePath.string()) == std::string::npos) {
            try {
                fs::rename(sourcePath, destinationPath);
                std::cout << "[ProjectPanel] Moved: " << sourcePath.filename() << " -> " << destinationPath.string() << std::endl;
            } catch (const fs::filesystem_error& e) {
                std::cerr << "[ProjectPanel] Failed to move item: " << e.what() << std::endl;
            }
        }
    }
}

void ProjectPanel::Draw(Scene& activeScene, GameObject*& selectedObject, pybind11::object& selectedAsset, std::string& selectedAssetPath, const fs::path& assetsPath, std::string& currentScenePath) {
    ImGui::Begin("Project");

    if (m_currentDirectory.empty()) {
        m_currentDirectory = assetsPath;
    }

    // Global Right-Click menu for the panel background (Root directory creation)
    if (ImGui::BeginPopupContextWindow("ProjectPanelContext")) {
        if (ImGui::BeginMenu(ICON_FA_PLUS " Create")) {
            if (ImGui::MenuItem(ICON_FA_FOLDER " Folder")) {
                m_currentDirectory = assetsPath; // Force creation at root
                m_requestFolderModal = true;
            }
            ImGui::Separator();
            if (ImGui::BeginMenu(ICON_FA_FILE_CODE " Python Script")) {
                if (ImGui::MenuItem("Component")) {
                    m_currentDirectory = assetsPath; // Force creation at root
                    m_pendingScriptType = ScriptType::Component;
                    m_requestScriptModal = true;
                }
                if (ImGui::MenuItem("Scriptable Object Class")) {
                    m_currentDirectory = assetsPath; // Force creation at root
                    m_pendingScriptType = ScriptType::ScriptableObject;
                    m_requestScriptModal = true;
                }
                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }
        ImGui::EndPopup();
    }

    // Simple Header
    ImGui::TextDisabled("Root: %s", assetsPath.string().c_str());

    // The search bar
    ImGui::InputTextWithHint("##Search", ICON_FA_MAGNIFYING_GLASS " Search...", m_searchBuffer, IM_ARRAYSIZE(m_searchBuffer));

    ImGui::Separator();

    // Allow dropping items into the empty space of the panel to move them to the root
    if (ImGui::BeginDragDropTarget()) {
        HandleDragAndDropMove(assetsPath);
        ImGui::EndDragDropTarget();
    }

    // Viewport
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4, 4));

    // Only draw the Tree View now
    DrawDirectoryNode(activeScene, selectedObject, selectedAsset, selectedAssetPath, assetsPath, currentScenePath);

    ImGui::PopStyleVar();

    // Trigger the modals at the root id stack safely
    if (m_requestScriptModal) {
        ImGui::OpenPopup("New Python Script");
        m_requestScriptModal = false;
    }
    if (m_requestAssetModal) {
        ImGui::OpenPopup("New Scriptable Object");
        m_requestAssetModal = false;
    }
    if (m_requestFolderModal) {
        ImGui::OpenPopup("New Folder");
        m_requestFolderModal = false;
    }
    if (m_requestRenameModal) {
        ImGui::OpenPopup("Rename Item");
        m_requestRenameModal = false;
    }
    if (m_requestDeleteModal) {
        ImGui::OpenPopup("Delete Item");
        m_requestDeleteModal = false;
    }

    // Modal: Python Script
    if (ImGui::BeginPopupModal("New Python Script", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        static char scriptName[64] = "NewScript";
        ImGui::InputText("Name", scriptName, IM_ARRAYSIZE(scriptName));
        
        ImGui::TextDisabled("Template: %s", m_pendingScriptType == ScriptType::Component ? "Component" : "Scriptable Object");
        ImGui::Spacing();
                
        if (ImGui::Button("Create", ImVec2(120, 0))) {
            CreatePythonScript(m_currentDirectory, scriptName, m_pendingScriptType);
            ImGui::CloseCurrentPopup();
            
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
        
        ImGui::BeginDisabled();
        ImGui::InputText("Module", m_assetPrefillModule, sizeof(m_assetPrefillModule));
        ImGui::InputText("Class", m_assetPrefillClass, sizeof(m_assetPrefillClass));
        ImGui::EndDisabled();
                
        if (ImGui::Button("Create", ImVec2(120, 0))) {
            CreateAssetFile(m_currentDirectory, assetFileName, assetId, m_assetPrefillModule, m_assetPrefillClass);
            ImGui::CloseCurrentPopup();
            
            strcpy(assetFileName, "NewData");
            strcpy(assetId, "data_01");
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    // Modal: Folder
    if (ImGui::BeginPopupModal("New Folder", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        static char folderName[64] = "NewFolder";
        ImGui::InputText("Folder Name", folderName, IM_ARRAYSIZE(folderName));
        ImGui::Spacing();
                
        if (ImGui::Button("Create", ImVec2(120, 0))) {
            fs::path newFolderPath = m_currentDirectory / folderName;
            if (!fs::exists(newFolderPath)) {
                fs::create_directory(newFolderPath);
                std::cout << "[ProjectPanel] Created new folder: " << newFolderPath.string() << std::endl;
            } else {
                std::cerr << "[ProjectPanel] Error: Folder already exists!" << std::endl;
            }
            ImGui::CloseCurrentPopup();
            strcpy(folderName, "NewFolder");
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    // Rename modal
    if (ImGui::BeginPopupModal("Rename Item", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputText("New Name", m_renameBuffer, IM_ARRAYSIZE(m_renameBuffer));
        ImGui::Spacing();
                
        if (ImGui::Button("Rename", ImVec2(120, 0))) {
            // Construct the new path in the same parent directory
            fs::path newPath = m_actionTarget.parent_path() / m_renameBuffer;
            
            if (!fs::exists(newPath) && !std::string(m_renameBuffer).empty()) {
                try {
                    fs::rename(m_actionTarget, newPath);
                } catch (const fs::filesystem_error& e) {
                    std::cerr << "[ProjectPanel] Rename error: " << e.what() << std::endl;
                }
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    // Delete modal
    if (ImGui::BeginPopupModal("Delete Item", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Are you sure you want to delete:\n%s ?", m_actionTarget.filename().string().c_str());
        ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "This action cannot be undone.");
        ImGui::Spacing();
                
        if (ImGui::Button("Delete", ImVec2(120, 0))) {
            try {
                // remove_all recursively deletes folders and files safely
                fs::remove_all(m_actionTarget); 
            } catch (const fs::filesystem_error& e) {
                std::cerr << "[ProjectPanel] Delete error: " << e.what() << std::endl;
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    ImGui::End();
}

void ProjectPanel::DrawDirectoryNode(Scene& activeScene, GameObject*& selectedObject, pybind11::object& selectedAsset, std::string& selectedAssetPath, const fs::path& path, std::string& currentScenePath) {
    if (!fs::exists(path)) return;

    for (const auto& entry : fs::directory_iterator(path)) {
        const auto& filename = entry.path().filename().string();
        std::string extension = entry.path().extension().string();

        if (entry.is_directory() && filename == "__pycache__") continue;
        if (!entry.is_directory() && extension == ".pyc") continue;
        if (!filename.empty() && filename[0] == '.') continue;

        if (entry.is_directory()) {
            std::string folderName = ICON_FA_FOLDER " " + filename;
            bool isOpen = ImGui::TreeNodeEx(folderName.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth);

            if (ImGui::BeginDragDropTarget()) {
                HandleDragAndDropMove(entry.path());
                ImGui::EndDragDropTarget();
            }

            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                std::string itemPath = entry.path().string();
                ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", itemPath.c_str(), itemPath.size() + 1);
                ImGui::Text("%s Move %s", ICON_FA_FOLDER, filename.c_str());
                ImGui::EndDragDropSource();
            }

            ImGui::PushID(entry.path().string().c_str());
            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem(ICON_FA_PEN " Rename")) {
                    m_actionTarget = entry.path();
                    strncpy(m_renameBuffer, filename.c_str(), sizeof(m_renameBuffer));
                    m_requestRenameModal = true;
                }
                if (ImGui::MenuItem(ICON_FA_TRASH " Delete")) {
                    m_actionTarget = entry.path();
                    m_requestDeleteModal = true;
                }

                ImGui::Separator();

                if (ImGui::MenuItem(ICON_FA_FOLDER " Create Folder")) {
                    m_currentDirectory = entry.path();
                    m_requestFolderModal = true;
                }
                ImGui::Separator();
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

            if (isOpen) {
                DrawDirectoryNode(activeScene, selectedObject, selectedAsset, selectedAssetPath, entry.path(), currentScenePath); 
                ImGui::TreePop();
            }
        } else {
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
            
            std::string searchQuery = m_searchBuffer;
            if (!searchQuery.empty()) {
                std::string filenameLower = filename;
                std::string searchLower = searchQuery;
                
                // Convert both to lowercase for case-insensitive search
                std::transform(filenameLower.begin(), filenameLower.end(), filenameLower.begin(), ::tolower);
                std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);
                
                // If the filename does not contain the search string, skip drawing it
                if (filenameLower.find(searchLower) == std::string::npos) {
                    continue; 
                }
            }

            std::string icon = ICON_FA_FILE;
            if (extension == ".scene") icon = ICON_FA_CUBES;
            else if (extension == ".prefab") icon = ICON_FA_BOX;
            else if (extension == ".py") icon = ICON_FA_FILE_CODE;
            else if (extension == ".png") icon = ICON_FA_IMAGE;
            else if (extension == ".asset") icon = ICON_FA_DATABASE;

            std::string displayName = icon + "  " + filename;
            ImGui::TreeNodeEx(displayName.c_str(), flags);

            if (ImGui::BeginPopupContextItem("FileContextMenu")) {
                if (ImGui::MenuItem(ICON_FA_PEN " Rename")) {
                    m_actionTarget = entry.path();
                    strncpy(m_renameBuffer, filename.c_str(), sizeof(m_renameBuffer));
                    m_requestRenameModal = true;
                }
                if (ImGui::MenuItem(ICON_FA_TRASH " Delete")) {
                    m_actionTarget = entry.path();
                    m_requestDeleteModal = true;
                }
            
                if (extension == ".py") {
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
                }
                ImGui::EndPopup();
            }

            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                std::string itemPath = entry.path().string();
                ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", itemPath.c_str(), itemPath.size() + 1);
                ImGui::Text("%s Move %s", icon.c_str(), filename.c_str());
                ImGui::EndDragDropSource();
            }

            if (ImGui::IsItemHovered()) {
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    if (extension == ".scene") {
                        std::cout << "[Editor] Loading scene via Project Browser: " << filename << std::endl;
                        selectedObject = nullptr; 
                        currentScenePath = entry.path().string();
                        activeScene.LoadFromFile(currentScenePath, false);
                    } else if (extension == ".asset") {
                        std::cout << "[Editor] Inspecting asset: " << filename << std::endl;
                        selectedObject = nullptr;
                        selectedAssetPath = entry.path().string();
                        selectedAsset = DataManager::LoadAsset(selectedAssetPath);
                    }
                }
            }
        }
    }
}
