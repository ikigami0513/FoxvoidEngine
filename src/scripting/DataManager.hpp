#pragma once

#include <string>
#include <unordered_map>
#include <pybind11/pybind11.h>

namespace py = pybind11;

class [[gnu::visibility("default")]] DataManager {
    public:
        // Loads an asset from disk, or returns the cached version if already loaded
        static py::object LoadAsset(const std::string& filepath);

        // Saves a Python ScriptableObject instance back to disk as JSON
        static void SaveAsset(py::object asset, const std::string& filepath);

        // Clears the cache (useful when reloading a scene or closing the game)
        static void ClearCache();

    private:
        // Maps the filepath to the active Python instance
        static std::unordered_map<std::string, py::object> s_assetCache;
};
