#include "GameStateManager.hpp"
#include <fstream>
#include <iostream>

// Initialize static members
std::unordered_map<std::string, int> GameStateManager::Ints;
std::unordered_map<std::string, float> GameStateManager::Floats;
std::unordered_map<std::string, bool> GameStateManager::Bools;
std::unordered_map<std::string, std::string> GameStateManager::Strings;

void GameStateManager::Load(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return; // If file doesn't exist yet, just start empty

    nlohmann::json j;
    try {
        file >> j;
        
        if (j.contains("Ints")) Ints = j["Ints"].get<std::unordered_map<std::string, int>>();
        if (j.contains("Floats")) Floats = j["Floats"].get<std::unordered_map<std::string, float>>();
        if (j.contains("Bools")) Bools = j["Bools"].get<std::unordered_map<std::string, bool>>();
        if (j.contains("Strings")) Strings = j["Strings"].get<std::unordered_map<std::string, std::string>>();
        
        std::cout << "[GameState] Loaded globals from " << path << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[GameState] Error loading JSON: " << e.what() << std::endl;
    }
}

void GameStateManager::Save(const std::string& path) {
    nlohmann::json j;
    j["Ints"] = Ints;
    j["Floats"] = Floats;
    j["Bools"] = Bools;
    j["Strings"] = Strings;

    std::ofstream file(path);
    if (file.is_open()) {
        // Pretty print with an indent of 4 spaces
        file << j.dump(4);
    }
}
