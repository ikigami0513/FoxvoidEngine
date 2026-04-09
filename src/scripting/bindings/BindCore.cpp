#include "../ScriptBindings.hpp"
#include <iostream>
#include <pybind11/stl.h>

#include "../../world/GameObject.hpp"
#include "../../world/Component.hpp"
#include "../../physics/Transform2d.hpp"
#include "../../graphics/SpriteRenderer.hpp"
#include "../../graphics/SpriteSheetRenderer.hpp"
#include "../../graphics/Animation2d.hpp"

void BindCore(py::module_& m) {
    m.def("log", [](const std::string& msg) {
        std::cout << "[Python] " << msg << std::endl;
    });

    py::class_<Component>(m, "Component")
        .def(py::init<>())
        .def_property_readonly("game_object", [](Component& c) { return c.owner; }, py::return_value_policy::reference);

    py::class_<GameObject>(m, "GameObject")
        .def("get_component", [](GameObject& go, py::object type_obj) -> py::object {
            // Get the string name of the requested class (e.g., "Transform2d")
            std::string type_name = py::str(py::getattr(type_obj, "__name__"));
            
            if (type_name == "Transform2d") {
                Transform2d* t = go.GetComponent<Transform2d>();
                if (t) return py::cast(t, py::return_value_policy::reference);
            }
            else if (type_name == "SpriteRenderer") {
                SpriteRenderer* s = go.GetComponent<SpriteRenderer>();
                if (s) return py::cast(s, py::return_value_policy::reference);
            }
            else if (type_name == "SpriteSheetRenderer") {
                SpriteSheetRenderer* s = go.GetComponent<SpriteSheetRenderer>();
                if (s) return py::cast(s, py::return_value_policy::reference);
            }
            else if (type_name == "Animation2d") {
                Animation2d* a = go.GetComponent<Animation2d>();
                if (a) return py::cast(a, py::return_value_policy::reference);
            }
            
            // Return None if component is not found
            return py::none();
        })
        .def("add_component", [](GameObject& go, py::object type_obj, py::args args) -> py::object {
            std::string type_name = py::str(py::getattr(type_obj, "__name__"));
            
            if (type_name == "Transform2d") {
                // Extract optional arguments (x, y) if Python provided them
                float x = 0.0f, y = 0.0f;
                if (args.size() >= 1) x = args[0].cast<float>();
                if (args.size() >= 2) y = args[1].cast<float>();
                
                Transform2d* t = go.AddComponent<Transform2d>(x, y);
                return py::cast(t, py::return_value_policy::reference);
            }
            else if (type_name == "SpriteRenderer") {
                // Ensure Python provided the mandatory texture path
                if (args.size() < 1) {
                    std::cerr << "[Python] SpriteRenderer requires a texture path string!" << std::endl;
                    return py::none();
                }
                std::string path = args[0].cast<std::string>();
                SpriteRenderer* s = go.AddComponent<SpriteRenderer>(path);
                return py::cast(s, py::return_value_policy::reference);
            }
            else if (type_name == "SpriteSheetRenderer") {
                // Ensure Python provided the mandatory path, cols, and rows
                if (args.size() < 3) {
                    std::cerr << "[Python] SpriteSheetRenderer requires (texture_path, columns, rows)!" << std::endl;
                    return py::none();
                }
                std::string path = args[0].cast<std::string>();
                int cols = args[1].cast<int>();
                int rows = args[2].cast<int>();
                SpriteSheetRenderer* s = go.AddComponent<SpriteSheetRenderer>(path, cols, rows);
                return py::cast(s, py::return_value_policy::reference);
            }
            else if (type_name == "Animation2d") {
                // Ensure Python provided at least the frames and speed
                if (args.size() < 2) {
                    std::cerr << "[Python] Animation2d requires (frames_list, speed, [loop])!" << std::endl;
                    return py::none();
                }
                // Automatically casts Python list to std::vector<int> thanks to stl.h
                std::vector<int> frames = args[0].cast<std::vector<int>>();
                float speed = args[1].cast<float>();
                bool loop = true; // Default value
                
                if (args.size() >= 3) {
                    loop = args[2].cast<bool>();
                }
                
                Animation2d* a = go.AddComponent<Animation2d>(frames, speed, loop);
                return py::cast(a, py::return_value_policy::reference);
            }

            return py::none();
        });
}