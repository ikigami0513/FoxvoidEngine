#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include <pybind11/pybind11.h>

namespace py = pybind11;

class [[gnu::visibility("default")]] ScriptableObject {
    public:
        virtual ~ScriptableObject() = default;

        std::string assetId = "";

        std::string name = "New Scriptable Object";

        std::string scriptName = "";
        std::string className = "";

#ifndef STANDALONE_MODE
        // Dynamically inspects the Python object and draws its variables
        virtual void OnInspector();
#endif

        virtual nlohmann::json Serialize() const;
        virtual void Deserialize(const nlohmann::json& j);
};
