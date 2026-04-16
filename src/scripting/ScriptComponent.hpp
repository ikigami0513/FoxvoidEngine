#pragma once

#include "../world/Component.hpp"
#include <string>
#include <pybind11/pybind11.h>
#include <nlohmann/json.hpp>

namespace py = pybind11;

class [[gnu::visibility("default")]] ScriptComponent : public Component {
    public:
        ScriptComponent() = default;

        ~ScriptComponent() override;

        ScriptComponent(const std::string& moduleName, const std::string& className);
        
        void Start() override;
        void Update(float deltaTime) override;

        void OnCollision(const Collision2D& collision) override;

        std::string GetName() const override;

#ifndef STANDALONE_MODE
        // Dynamically inspects the Python object and draws its variables
        void OnInspector() override;
#endif

        nlohmann::json Serialize() const override;
        void Deserialize(const nlohmann::json& j) override;

    private:
        // Executes the reload via Python's importlib
        void HotReload();

        std::string m_scriptName;
        std::string m_className;

        py::object m_instance; 

        std::string m_scriptFilePath;
        std::filesystem::file_time_type m_lastWriteTime;
};
