#include "ScriptableObject.hpp"
#include <iostream>

#ifndef STANDALONE_MODE
#include <imgui.h>
#endif

#ifndef STANDALONE_MODE
void ScriptableObject::OnInspector() {
    // 1. Draw Native C++ Base Properties
    char nameBuffer[256];
    strncpy(nameBuffer, name.c_str(), sizeof(nameBuffer));
    if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer))) {
        name = nameBuffer;
    }

    char idBuffer[256];
    strncpy(idBuffer, assetId.c_str(), sizeof(idBuffer));
    if (ImGui::InputText("Asset ID", idBuffer, sizeof(idBuffer))) {
        assetId = idBuffer;
    }

    ImGui::Separator();
    ImGui::TextDisabled("Python Custom Properties");
    ImGui::Spacing();

    // 2. Introspect Python Properties
    try {
        // Magie de pybind11 : on récupère l'instance Python correspondant à "this"
        py::object pyInstance = py::cast(this);
        
        if (!py::hasattr(pyInstance, "__dict__")) return;
        
        py::dict attributes = pyInstance.attr("__dict__");

        for (auto item : attributes) {
            std::string key = py::str(item.first);
            
            // On ignore les variables internes et les variables déjà affichées en haut
            if (key.rfind("_", 0) == 0 || key == "asset_id" || key == "name") continue;

            py::object value = py::reinterpret_borrow<py::object>(item.second);

            // Handle Floats
            if (py::isinstance<py::float_>(value)) {
                float v = value.cast<float>();
                if (ImGui::DragFloat(key.c_str(), &v, 0.1f)) {
                    pyInstance.attr(key.c_str()) = v;
                }
            }
            // Handle Booleans
            else if (py::isinstance<py::bool_>(value)) {
                bool v = value.cast<bool>();
                if (ImGui::Checkbox(key.c_str(), &v)) {
                    pyInstance.attr(key.c_str()) = v;
                }
            }
            // Handle Integers
            else if (py::isinstance<py::int_>(value)) {
                int v = value.cast<int>();
                if (ImGui::DragInt(key.c_str(), &v, 1.0f)) {
                    pyInstance.attr(key.c_str()) = v;
                }
            }
            // Handle Strings
            else if (py::isinstance<py::str>(value)) {
                std::string v = value.cast<std::string>();
                char buffer[256];
                strncpy(buffer, v.c_str(), sizeof(buffer));
                buffer[sizeof(buffer) - 1] = '\0'; // Safety
                
                if (ImGui::InputText(key.c_str(), buffer, sizeof(buffer))) {
                    pyInstance.attr(key.c_str()) = std::string(buffer);
                }
            }
        }
    } 
    catch (const py::error_already_set& e) {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Python Introspection Error:");
        ImGui::TextWrapped("%s", e.what());
    }
}
#endif

nlohmann::json ScriptableObject::Serialize() const {
    nlohmann::json j;
    
    j["assetId"] = assetId;
    j["name"] = name;
    j["scriptName"] = scriptName;
    j["className"] = className;

    nlohmann::json properties;

    try {
        py::object pyInstance = py::cast(this);
        if (py::hasattr(pyInstance, "__dict__")) {
            py::dict attributes = pyInstance.attr("__dict__");

            for (auto item : attributes) {
                std::string key = py::str(item.first);
                
                if (key.rfind("_", 0) == 0 || key == "asset_id" || key == "name") continue;

                py::object value = py::reinterpret_borrow<py::object>(item.second);

                if (py::isinstance<py::bool_>(value)) {
                    properties[key] = value.cast<bool>();
                } else if (py::isinstance<py::float_>(value)) {
                    properties[key] = value.cast<float>();
                } else if (py::isinstance<py::int_>(value)) {
                    properties[key] = value.cast<int>();
                } else if (py::isinstance<py::str>(value)) {
                    properties[key] = value.cast<std::string>();
                }
            }
        }
    } catch (const py::error_already_set& e) {
        std::cerr << "[ScriptableObject] Auto-Serialize Error:\n" << e.what() << std::endl;
    }

    j["properties"] = properties;

    return j;
}

void ScriptableObject::Deserialize(const nlohmann::json& j) {
    if (j.contains("assetId")) assetId = j["assetId"].get<std::string>();
    if (j.contains("name")) name = j["name"].get<std::string>();
    if (j.contains("scriptName")) scriptName = j["scriptName"].get<std::string>();
    if (j.contains("className")) className = j["className"].get<std::string>();

    if (j.contains("properties")) {
        auto props = j["properties"];
        
        try {
            py::object pyInstance = py::cast(this);
            
            for (auto& el : props.items()) {
                std::string key = el.key();
                
                if (el.value().is_boolean()) {
                    pyInstance.attr(key.c_str()) = el.value().get<bool>();
                } else if (el.value().is_number_float()) {
                    pyInstance.attr(key.c_str()) = el.value().get<float>();
                } else if (el.value().is_number_integer()) {
                    pyInstance.attr(key.c_str()) = el.value().get<int>();
                } else if (el.value().is_string()) {
                    pyInstance.attr(key.c_str()) = el.value().get<std::string>();
                }
            }
        } catch (const py::error_already_set& e) {
            std::cerr << "[ScriptableObject] Deserialize Error:\n" << e.what() << std::endl;
        }
    }
}
