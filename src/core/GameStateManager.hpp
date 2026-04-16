#pragma once

#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>

class GameStateManager {
    public:
        // The 4 main dictionaries for our global variables
        static std::unordered_map<std::string, int> Ints;
        static std::unordered_map<std::string, float> Floats;
        static std::unordered_map<std::string, bool> Bools;
        static std::unordered_map<std::string, std::string> Strings;

        // File I/O
        static void Load(const std::string& path = "assets/settings/globals.json");
        static void Save(const std::string& path = "assets/settings/globals.json");

        // Helpers for Python Bindings
        static void SetInt(const std::string& key, int value) { Ints[key] = value; }
        static int GetInt(const std::string& key, int defaultVal = 0) {
            return Ints.contains(key) ? Ints[key] : defaultVal;
        }

        static void SetFloat(const std::string& key, float value) { Floats[key] = value; }
        static float GetFloat(const std::string& key, float defaultVal = 0.0f) {
            return Floats.contains(key) ? Floats[key] : defaultVal;
        }

        static void SetBool(const std::string& key, bool value) { Bools[key] = value; }
        static bool GetBool(const std::string& key, bool defaultVal = false) {
            return Bools.contains(key) ? Bools[key] : defaultVal;
        }

        static void SetString(const std::string& key, const std::string& value) { Strings[key] = value; }
        static std::string GetString(const std::string& key, const std::string& defaultVal = "") {
            return Strings.contains(key) ? Strings[key] : defaultVal;
        }
};
