#pragma once

#include "../world/Component.hpp"
#include <string>
#include <pybind11/pybind11.h>
#include <nlohmann/json.hpp>

namespace py = pybind11;

class ScriptComponent : public Component {
    public:
        ScriptComponent() = default;

        ~ScriptComponent() override;

        ScriptComponent(const std::string& moduleName, const std::string& className);
        
        void Start() override;
        void Update(float deltaTime) override;

        std::string GetName() const override;

        // Dynamically inspects the Python object and draws its variables
        void OnInspector() override;

        nlohmann::json Serialize() const override;
        void Deserialize(const nlohmann::json& j) override;

    private:
        std::string m_scriptName;
        std::string m_className;

        py::object m_instance; 
};
