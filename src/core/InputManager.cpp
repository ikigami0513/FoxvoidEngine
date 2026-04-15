#include "InputManager.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <filesystem>

std::unordered_map<std::string, std::vector<int>> InputManager::s_bindings;

void InputManager::BindKey(const std::string& actionName, int key) {
    // Check if the key is already bound to avoid duplicates
    auto& keys = s_bindings[actionName];
    if (std::find(keys.begin(), keys.end(), key) == keys.end()) {
        keys.push_back(key);
    }
}

void InputManager::UnbindKey(const std::string& actionName, int key) {
    if (s_bindings.find(actionName) != s_bindings.end()) {
        auto& keys = s_bindings[actionName];
        keys.erase(std::remove(keys.begin(), keys.end(), key), keys.end());
    }
}

bool InputManager::IsActionDown(const std::string& actionName) {
    if (s_bindings.find(actionName) != s_bindings.end()) {
        for (int key : s_bindings[actionName]) {
            if (IsKeyDown(key)) {
                return true; // Return true as soon as ONE bound key is down
            }
        }
    }
    return false;
}

bool InputManager::IsActionPressed(const std::string& actionName) {
    if (s_bindings.find(actionName) != s_bindings.end()) {
        for (int key : s_bindings[actionName]) {
            if (IsKeyPressed(key)) {
                return true;
            }
        }
    }
    return false;
}

std::unordered_map<std::string, std::vector<int>>& InputManager::GetBindings() {
    return s_bindings;
}

void InputManager::Save(const std::string& filepath) {
    nlohmann::json j;
    
    // Convert the unordered_map into a JSON object
    for (const auto& pair : s_bindings) {
        j[pair.first] = pair.second;
    }

    // Ensute the parent directory exists before trying to save
    std::filesystem::path pathObj(filepath);
    if (pathObj.has_parent_path()) {
        std::filesystem::create_directories(pathObj.parent_path());
    }

    std::ofstream file(filepath);
    if (file.is_open()) {
        file << j.dump(4);
        file.close();
        std::cout << "[InputManager] Saved bindings to " << filepath << std::endl;
    }
}

void InputManager::Load(const std::string& filepath) {
    std::ifstream file(filepath);
    if (file.is_open()) {
        nlohmann::json j;
        file >> j;
        
        s_bindings.clear();
        for (auto& el : j.items()) {
            // Reconstruct the vector of integers from the JSON array
            std::vector<int> keys;
            for (auto& keyJson : el.value()) {
                keys.push_back(keyJson.get<int>());
            }
            s_bindings[el.key()] = keys;
        }
        file.close();
        std::cout << "[InputManager] Loaded bindings from " << filepath << std::endl;
    } else {
        std::cerr << "[InputManager] Could not open " << filepath << " (It may not exist yet)." << std::endl;
    }
}
