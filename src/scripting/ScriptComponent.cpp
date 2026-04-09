#include "ScriptComponent.hpp"
#include <iostream>
#include "../world/GameObject.hpp"
#include "../physics/Transform2d.hpp"

ScriptComponent::ScriptComponent(const std::string& moduleName, const std::string& className) {
    try {
        py::module_ mod = py::module_::import(moduleName.c_str());
        
        py::object cls = mod.attr(className.c_str());
        
        m_instance = cls(); 
        
    } catch (const py::error_already_set& e) {
        std::cerr << "[ScriptComponent] Constructor Error:\n" << e.what() << std::endl;
    }
}

void ScriptComponent::Start() {
    if (!m_instance) return;

    try {
        Component* nativeComponent = m_instance.cast<Component*>();

        if (nativeComponent) {
            // We bypass Python entirely and set the C++ pointer directly.
            // When Python scripts call 'self.game_object', it will read this exact pointer!
            nativeComponent->owner = this->owner;
        }

        // Call the start method in Python
        if (py::hasattr(m_instance, "start")) {
            m_instance.attr("start")();
        }
    } catch (const py::error_already_set& e) {
        std::cerr << "[ScriptComponent] Start Error:\n" << e.what() << std::endl;
    }
}

void ScriptComponent::Update(float deltaTime) {
    if (!m_instance) return;

    try {
        if (py::hasattr(m_instance, "update")) {
            m_instance.attr("update")(deltaTime);
        }
    } catch (const py::error_already_set& e) {
        std::cerr << "[ScriptComponent] Update Error:\n" << e.what() << std::endl;
    }
}
