#pragma once

#ifndef STANDALONE_MODE
#include <imgui.h>
#endif

#include <filesystem>
#include <iostream>
#include <pybind11/pybind11.h>
#include "world/Scene.hpp"
#include "world/GameObject.hpp"

namespace fs = std::filesystem;

// Define the types of scripts we can generate
enum class ScriptType {
    Component,
    ScriptableObject
};

class ProjectPanel {
    public:
        ProjectPanel() = default;
        ~ProjectPanel() = default;
    
        void Draw(Scene& activeScene, GameObject*& selectedObject, pybind11::object& selectedAsset, std::string& selectedAssetPath, const fs::path& assetsPath, std::string& currentScenePath);

    private:
        // Recursive function to read and display the folder tree
        void DrawDirectoryNode(Scene& activeScene, GameObject*& selectedObject, pybind11::object& selectedAsset, std::string& selectedAssetPath, const fs::path& path, std::string& currentScenePath);

        fs::path m_currentDirectory = "";

        // Flags to trigger the modals safely
        bool m_requestScriptModal = false;
        bool m_requestAssetModal = false;
        bool m_requestFolderModal = false;

        // Flags for Rename and Delete modals
        bool m_requestRenameModal = false;
        bool m_requestDeleteModal = false;

        // Track which type of script the modal should generate
        ScriptType m_pendingScriptType = ScriptType::Component;

        // Buffers to pre-fill the Asset Modal
        char m_assetPrefillModule[64] = "";
        char m_assetPrefillClass[64] = "";

        // Buffer for the search bar
        char m_searchBuffer[128] = "";

        // Shared state for renaming and deleting items
        fs::path m_actionTarget = ""; 
        char m_renameBuffer[128] = "";
};
