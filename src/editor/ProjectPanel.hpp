#pragma once

#ifndef STANDALONE_MODE
#include <imgui.h>
#endif

#include <filesystem>
#include <iostream>
#include <pybind11/pybind11.h>
#include "../world/Scene.hpp"
#include "../world/GameObject.hpp"

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
    
        // UPDATED: Added selectedAsset and selectedAssetPath to the signature
        void Draw(Scene& activeScene, GameObject*& selectedObject, pybind11::object& selectedAsset, std::string& selectedAssetPath, const fs::path& assetsPath, std::string& currentScenePath);

    private:
        // Recursive function to read and display the folder tree
        void DrawDirectoryNode(Scene& activeScene, GameObject*& selectedObject, pybind11::object& selectedAsset, std::string& selectedAssetPath, const fs::path& path, std::string& currentScenePath);

        void DrawExplorerView(Scene& activeScene, GameObject*& selectedObject, pybind11::object& selectedAsset, std::string& selectedAssetPath, std::string& currentScenePath);

        bool m_isTreeView = true; // Start in Tree view by default
        fs::path m_currentDirectory = "";

        // Flag to trigger the modal safely
        bool m_requestScriptModal = false;

        // Flag for the Scriptable Object Modal
        bool m_requestAssetModal = false;

        // Track which type of script the modal should generate
        ScriptType m_pendingScriptType = ScriptType::Component;

        // Buffers to pre-fill the Asset Modal
        char m_assetPrefillModule[64] = "";
        char m_assetPrefillClass[64] = "";
};
