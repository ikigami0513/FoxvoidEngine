#include "DataManager.hpp"
#include "ScriptableObject.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

// Initialize the static cache
std::unordered_map<std::string, py::object> DataManager::s_assetCache;

py::object DataManager::LoadAsset(const std::string& filepath) {
    // 1. Check if the asset is already loaded in memory
    if (s_assetCache.find(filepath) != s_assetCache.end()) {
        return s_assetCache[filepath];
    }

    // 2. Read the .asset (JSON) file
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "[DataManager] Failed to open asset file: " << filepath << std::endl;
        return py::none();
    }

    nlohmann::json j;
    try {
        file >> j;
    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "[DataManager] JSON Parse Error in " << filepath << ": " << e.what() << std::endl;
        return py::none();
    }
    file.close();

    // 3. Extract the Python script/class names
    std::string scriptName = j.value("scriptName", "");
    std::string className = j.value("className", "");

    if (scriptName.empty() || className.empty()) {
        std::cerr << "[DataManager] Invalid asset data (missing scriptName or className): " << filepath << std::endl;
        return py::none();
    }

    // 4. Instantiate the Python class and Deserialize
    try {
        py::module_ mod = py::module_::import(scriptName.c_str());
        py::object pyInstance = mod.attr(className.c_str())();

        // Cast to C++ ScriptableObject pointer to call Deserialize
        ScriptableObject* nativeObj = pyInstance.cast<ScriptableObject*>();
        if (nativeObj) {
            nativeObj->Deserialize(j);
        }

        // 5. Store in cache and return
        s_assetCache[filepath] = pyInstance;
        return pyInstance;

    } catch (const py::error_already_set& e) {
        std::cerr << "[DataManager] Failed to instantiate Python asset from " << filepath << ":\n" << e.what() << std::endl;
        return py::none();
    }
}

void DataManager::SaveAsset(py::object asset, const std::string& filepath) {
    if (asset.is_none()) return;

    try {
        ScriptableObject* nativeObj = asset.cast<ScriptableObject*>();
        if (nativeObj) {
            nlohmann::json j = nativeObj->Serialize();

            std::ofstream file(filepath);
            if (file.is_open()) {
                // Save with 4 spaces indentation for readability
                file << j.dump(4);
                file.close();
            } else {
                std::cerr << "[DataManager] Failed to write to asset file: " << filepath << std::endl;
            }
        }
    } catch (const py::error_already_set& e) {
        std::cerr << "[DataManager] Error saving asset to " << filepath << ":\n" << e.what() << std::endl;
    }
}

void DataManager::ClearCache() {
    s_assetCache.clear();
}
