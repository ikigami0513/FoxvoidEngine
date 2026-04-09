#include "ScriptComponent.hpp"
#include <iostream>

ScriptComponent::ScriptComponent(const std::string& moduleName, const std::string& className)
    : m_scriptName(moduleName), m_className(className)
{
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

std::string ScriptComponent::GetName() const {
    // Show the python class name in the Inspector (e.g., "PlayerController (Script)")
    return m_className + " (Script)";
}

void ScriptComponent::OnInspector() {
    // Safety check: ensure the Python script is actually loaded and instantiated
    if (!m_instance) {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Python instance is null!");
        return;
    }

    try {
        // Fetch the internal dictionary of the Python object containing all its attributes
        py::dict attributes = m_instance.attr("__dict__");

        // Iterate through all variables defined in the Python script (e.g., self.speed = 400.0)
        for (auto item : attributes) {
            std::string key = py::str(item.first);
            
            // Skip private or internal Python variables (those starting with '_')
            if (key.rfind("_", 0) == 0) continue;

            py::object value = py::reinterpret_borrow<py::object>(item.second);

            if (py::isinstance<Component>(value)) {
                continue;
            }

            // --- TYPE CHECKING & IMGUI DRAWING ---

            // 1. Handle Floats
            if (py::isinstance<py::float_>(value)) {
                float v = value.cast<float>();
                // If the user drags the float, push the new value back to Python
                if (ImGui::DragFloat(key.c_str(), &v, 0.1f)) {
                    m_instance.attr(key.c_str()) = v;
                }
            }
            // 2. Handle Booleans (Must be checked BEFORE ints, as bool inherits from int in Python)
            else if (py::isinstance<py::bool_>(value)) {
                bool v = value.cast<bool>();
                if (ImGui::Checkbox(key.c_str(), &v)) {
                    m_instance.attr(key.c_str()) = v;
                }
            }
            // 3. Handle Integers
            else if (py::isinstance<py::int_>(value)) {
                int v = value.cast<int>();
                if (ImGui::DragInt(key.c_str(), &v, 1)) {
                    m_instance.attr(key.c_str()) = v;
                }
            }
            // 4. Handle Strings
            else if (py::isinstance<py::str>(value)) {
                std::string v = value.cast<std::string>();
                
                // ImGui requires a fixed-size C-string buffer for text inputs
                char buffer[256];
                strncpy(buffer, v.c_str(), sizeof(buffer));
                buffer[sizeof(buffer) - 1] = '\0'; // Ensure null-termination
                
                if (ImGui::InputText(key.c_str(), buffer, sizeof(buffer))) {
                    m_instance.attr(key.c_str()) = std::string(buffer);
                }
            }
            // 5. Handle unsupported or complex types (Lists, Vectors, Components)
            else {
                // Just display the type name as read-only text so the user knows it's there
                std::string typeName = py::str(value.get_type().attr("__name__"));
                ImGui::TextDisabled("%s : [%s]", key.c_str(), typeName.c_str());
            }
        }
    } 
    catch (const py::error_already_set& e) {
        // If anything goes wrong in Python, print it in red inside the Inspector
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Python Error:");
        ImGui::TextWrapped("%s", e.what());
    }
}
