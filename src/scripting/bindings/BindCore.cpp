#include "../ScriptBindings.hpp"
#include <iostream>
#include <pybind11/stl.h>

#include "../../core/Engine.hpp"
#include "../../world/GameObject.hpp"
#include "../../world/Component.hpp"
#include "../../physics/Transform2d.hpp"
#include "../../graphics/SpriteRenderer.hpp"
#include "../../graphics/SpriteSheetRenderer.hpp"
#include "../../graphics/Animation2d.hpp"
#include <world/ComponentRegistry.hpp>

void BindCore(py::module_& m) {
    m.def("log", [](const std::string& msg) {
        std::cout << "[Python] " << msg << std::endl;
    });

    py::class_<Component>(m, "Component")
        .def(py::init<>())
        .def_property_readonly("game_object", [](Component& c) { return c.owner; }, py::return_value_policy::reference);

    py::class_<GameObject>(m, "GameObject")
        // We return GameObject* directly. Pybind11 will apply the reference policy automatically
        // and convert nullptr to Python's None safely.
        .def_static("instantiate", [](const std::string& name) -> GameObject* {
            std::cout << "[C++] Attempting to instantiate: " << name << std::endl;
            
            // Safety check in case the Engine singleton is null
            if (!Engine::Get()) {
                std::cerr << "[C++] FATAL ERROR: Engine::Get() returned nullptr!" << std::endl;
                return nullptr;
            }

            GameObject* newGo = Engine::Get()->GetActiveScene().CreateGameObject(name);

            if (!newGo) {
                std::cerr << "[Python] Failed to instantiate: " << name << std::endl;
            }

            return newGo;
        }, py::return_value_policy::reference)
        // Expose the Destroy method so Python can schedule objects for deletion
        .def("destroy", &GameObject::Destroy)
        .def("get_component", [](GameObject& go, py::object type_obj) -> py::object {
            // Get the string name of the requested class (e.g., "Transform2d")
            std::string type_name = py::str(py::getattr(type_obj, "__name__"));
            
            // Search the registry for the getter function
            auto it = ComponentRegistry::getters.find(type_name);
            if (it != ComponentRegistry::getters.end()) {
                return it->second(go); // Execute the lambda
            }
            
            // Return None if component is not found
            return py::none();
        })
        .def("add_component", [](GameObject& go, py::object type_obj, py::args args) -> py::object {
            std::string type_name = py::str(py::getattr(type_obj, "__name__"));
            
            // Search the registry for the adder function
            auto it = ComponentRegistry::adders.find(type_name);
            if (it != ComponentRegistry::adders.end()) {
                return it->second(go, args); // Execute the lambda with args
            }

            std::cerr << "[Python] Unknown component type: " << type_name << std::endl;
            return py::none();
        });
}
