#include "ScriptEngine.hpp"
#include <iostream>
#include "../world/GameObject.hpp"
#include "../world/Component.hpp"
#include <physics/Transform2d.hpp>
#include "../graphics/SpriteRenderer.hpp"
#include <graphics/SpriteSheetRenderer.hpp>

PYBIND11_EMBEDDED_MODULE(foxvoid, m) {
    m.def("log", [](const std::string& msg) {
        std::cout << "[Python] " << msg << std::endl;
    });

    // Create a submodule for Keys. This will act like a namespace/enum in Python.
    py::module_ keys = m.def_submodule("Keys", "Raylib Keyboard Keys");
    keys.attr("RIGHT") = (int)KEY_RIGHT;
    keys.attr("LEFT")  = (int)KEY_LEFT;
    keys.attr("DOWN")  = (int)KEY_DOWN;
    keys.attr("UP")    = (int)KEY_UP;
    keys.attr("SPACE") = (int)KEY_SPACE;

    py::module_ input = m.def_submodule("Input", "Engine Input API");
    input.def("is_key_down", [](int key) { return IsKeyDown(key); });
    input.def("is_key_pressed", [](int key) { return IsKeyPressed(key); });

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

            return py::none();
        });

    py::class_<Component>(m, "Component")
        .def(py::init<>())
        .def_property_readonly("game_object", [](Component& c) { return c.owner; }, py::return_value_policy::reference);

    py::class_<Transform2d, Component>(m, "Transform2d")
        .def_property("x", 
            [](Transform2d& t) { return t.position.x; }, 
            [](Transform2d& t, float v) { t.position.x = v; })
        .def_property("y", 
            [](Transform2d& t) { return t.position.y; }, 
            [](Transform2d& t, float v) { t.position.y = v; });

    py::class_<SpriteRenderer, Component>(m, "SpriteRenderer")
        .def(py::init<std::string>());

    py::class_<SpriteSheetRenderer, Component>(m, "SpriteSheetRenderer")
        .def(py::init<std::string, int, int>())
        // We bind GetFrame and SetFrame to a clean Python property '.frame'
        .def_property("frame", &SpriteSheetRenderer::GetFrame, &SpriteSheetRenderer::SetFrame)
        // Read-only property for the total frame count
        .def_property_readonly("frame_count", &SpriteSheetRenderer::GetFrameCount);
}

py::scoped_interpreter* ScriptEngine::s_interpreter = nullptr;

void ScriptEngine::Initialize() {
    // Prevent double initialization
    if (!s_interpreter) {
        try {
            // 1. Start the Python interpreter
            s_interpreter = new py::scoped_interpreter();
            
            // 2. Import the 'sys' module to configure the path
            py::module_ sys = py::module_::import("sys");
            
            // 3. Append our specific scripts folder to sys.path
            // This allows us to just say "import main" to load "assets/scripts/main.py"
            sys.attr("path").attr("append")("assets/scripts");

            std::cout << "[ScriptEngine] Python (pybind11) Initialized." << std::endl;
        } catch (const py::error_already_set& e) {
            std::cerr << "[ScriptEngine] Failed to initialize Python:\n" << e.what() << std::endl;
        }
    }
}

void ScriptEngine::Shutdown() {
    if (s_interpreter) {
        // Deleting the scoped_interpreter safely finalizes Python
        delete s_interpreter;
        s_interpreter = nullptr;
        std::cout << "[ScriptEngine] Python Shutdown complete." << std::endl;
    }
}

void ScriptEngine::LoadModule(const std::string& moduleName) {
    try {
        // Dynamically import the python module
        py::module_ mod = py::module_::import(moduleName.c_str());
        
        std::cout << "[ScriptEngine] Python module loaded successfully: " << moduleName << std::endl;
    } catch (const py::error_already_set& e) {
        // pybind11 provides excellent error traces directly from Python
        std::cerr << "[ScriptEngine] Python Error while loading module '" << moduleName << "':\n" << e.what() << std::endl;
    }
}
