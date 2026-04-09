#include "ScriptEngine.hpp"
#include <iostream>
#include <pybind11/stl.h>

#include "../world/GameObject.hpp"
#include "../world/Component.hpp"
#include <physics/Transform2d.hpp>
#include "../graphics/SpriteRenderer.hpp"
#include <graphics/SpriteSheetRenderer.hpp>
#include <graphics/Animation2d.hpp>

PYBIND11_EMBEDDED_MODULE(foxvoid, m) {
    m.def("log", [](const std::string& msg) {
        std::cout << "[Python] " << msg << std::endl;
    });

    // Create a submodule for Keys. This will act like a namespace/enum in Python.
    py::module_ keys = m.def_submodule("Keys", "Raylib Keyboard Keys");
    keys.attr("KEY_NULL")       = (int)KEY_NULL;
    keys.attr("KEY_APOSTROPHE") = (int)KEY_APOSTROPHE;
    keys.attr("KEY_COMMA")      = (int)KEY_COMMA;
    keys.attr("KEY_MINUS")      = (int)KEY_MINUS;
    keys.attr("KEY_PERIOD")     = (int)KEY_PERIOD;
    keys.attr("KEY_SLASH")      = (int)KEY_SLASH;

    keys.attr("KEY_ZERO")  = (int)KEY_ZERO;
    keys.attr("KEY_ONE")   = (int)KEY_ONE;
    keys.attr("KEY_TWO")   = (int)KEY_TWO;
    keys.attr("KEY_THREE") = (int)KEY_THREE;
    keys.attr("KEY_FOUR")  = (int)KEY_FOUR;
    keys.attr("KEY_FIVE")  = (int)KEY_FIVE;
    keys.attr("KEY_SIX")   = (int)KEY_SIX;
    keys.attr("KEY_SEVEN") = (int)KEY_SEVEN;
    keys.attr("KEY_EIGHT") = (int)KEY_EIGHT;
    keys.attr("KEY_NINE")  = (int)KEY_NINE;

    keys.attr("KEY_SEMICOLON") = (int)KEY_SEMICOLON;
    keys.attr("KEY_EQUAL")     = (int)KEY_EQUAL;

    keys.attr("KEY_A") = (int)KEY_A;
    keys.attr("KEY_B") = (int)KEY_B;
    keys.attr("KEY_C") = (int)KEY_C;
    keys.attr("KEY_D") = (int)KEY_D;
    keys.attr("KEY_E") = (int)KEY_E;
    keys.attr("KEY_F") = (int)KEY_F;
    keys.attr("KEY_G") = (int)KEY_G;
    keys.attr("KEY_H") = (int)KEY_H;
    keys.attr("KEY_I") = (int)KEY_I;
    keys.attr("KEY_J") = (int)KEY_J;
    keys.attr("KEY_K") = (int)KEY_K;
    keys.attr("KEY_L") = (int)KEY_L;
    keys.attr("KEY_M") = (int)KEY_M;
    keys.attr("KEY_N") = (int)KEY_N;
    keys.attr("KEY_O") = (int)KEY_O;
    keys.attr("KEY_P") = (int)KEY_P;
    keys.attr("KEY_Q") = (int)KEY_Q;
    keys.attr("KEY_R") = (int)KEY_R;
    keys.attr("KEY_S") = (int)KEY_S;
    keys.attr("KEY_T") = (int)KEY_T;
    keys.attr("KEY_U") = (int)KEY_U;
    keys.attr("KEY_V") = (int)KEY_V;
    keys.attr("KEY_W") = (int)KEY_W;
    keys.attr("KEY_X") = (int)KEY_X;
    keys.attr("KEY_Y") = (int)KEY_Y;
    keys.attr("KEY_Z") = (int)KEY_Z;

    keys.attr("KEY_LEFT_BRACKET")  = (int)KEY_LEFT_BRACKET;
    keys.attr("KEY_BACKSLASH")     = (int)KEY_BACKSLASH;
    keys.attr("KEY_RIGHT_BRACKET") = (int)KEY_RIGHT_BRACKET;
    keys.attr("KEY_GRAVE")         = (int)KEY_GRAVE;

    keys.attr("KEY_SPACE")        = (int)KEY_SPACE;
    keys.attr("KEY_ESCAPE")       = (int)KEY_ESCAPE;
    keys.attr("KEY_ENTER")        = (int)KEY_ENTER;
    keys.attr("KEY_TAB")          = (int)KEY_TAB;
    keys.attr("KEY_BACKSPACE")    = (int)KEY_BACKSPACE;
    keys.attr("KEY_INSERT")       = (int)KEY_INSERT;
    keys.attr("KEY_DELETE")       = (int)KEY_DELETE;
    keys.attr("RIGHT")            = (int)KEY_RIGHT;
    keys.attr("LEFT")             = (int)KEY_LEFT;
    keys.attr("DOWN")             = (int)KEY_DOWN;
    keys.attr("UP")               = (int)KEY_UP;
    keys.attr("KEY_PAGE_UP")      = (int)KEY_PAGE_UP;
    keys.attr("KEY_PAGE_DOWN")    = (int)KEY_PAGE_DOWN;
    keys.attr("KEY_HOME")         = (int)KEY_HOME;
    keys.attr("KEY_END")          = (int)KEY_END;
    keys.attr("KEY_CAPS_LOCK")    = (int)KEY_CAPS_LOCK;
    keys.attr("KEY_SCROLL_LOCK")  = (int)KEY_SCROLL_LOCK;
    keys.attr("KEY_NUM_LOCK")     = (int)KEY_NUM_LOCK;
    keys.attr("KEY_PRINT_SCREEN") = (int)KEY_PRINT_SCREEN;
    keys.attr("KEY_PAUSE")        = (int)KEY_PAUSE;

    keys.attr("KEY_F1")  = (int)KEY_F1;
    keys.attr("KEY_F2")  = (int)KEY_F2;
    keys.attr("KEY_F3")  = (int)KEY_F3;
    keys.attr("KEY_F4")  = (int)KEY_F4;
    keys.attr("KEY_F5")  = (int)KEY_F5;
    keys.attr("KEY_F6")  = (int)KEY_F6;
    keys.attr("KEY_F7")  = (int)KEY_F7;
    keys.attr("KEY_F8")  = (int)KEY_F8;
    keys.attr("KEY_F9")  = (int)KEY_F9;
    keys.attr("KEY_F10") = (int)KEY_F10;
    keys.attr("KEY_F11") = (int)KEY_F11;
    keys.attr("KEY_F12") = (int)KEY_F12;

    keys.attr("KEY_LEFT_SHIFT")    = (int)KEY_LEFT_SHIFT;
    keys.attr("KEY_LEFT_CONTROL")  = (int)KEY_LEFT_CONTROL;
    keys.attr("KEY_LEFT_ALT")      = (int)KEY_LEFT_ALT;
    keys.attr("KEY_LEFT_SUPER")    = (int)KEY_LEFT_SUPER;
    keys.attr("KEY_RIGHT_SHIFT")   = (int)KEY_RIGHT_SHIFT;
    keys.attr("KEY_RIGHT_CONTROL") = (int)KEY_RIGHT_CONTROL;
    keys.attr("KEY_RIGHT_ALT")     = (int)KEY_RIGHT_ALT;
    keys.attr("KEY_RIGHT_SUPER")   = (int)KEY_RIGHT_SUPER;
    keys.attr("KEY_KB_MENU")       = (int)KEY_KB_MENU;

    keys.attr("KEY_KP_0") = (int)KEY_KP_0;
    keys.attr("KEY_KP_1") = (int)KEY_KP_1;
    keys.attr("KEY_KP_2") = (int)KEY_KP_2;
    keys.attr("KEY_KP_3") = (int)KEY_KP_3;
    keys.attr("KEY_KP_4") = (int)KEY_KP_4;
    keys.attr("KEY_KP_5") = (int)KEY_KP_5;
    keys.attr("KEY_KP_6") = (int)KEY_KP_6;
    keys.attr("KEY_KP_7") = (int)KEY_KP_7;
    keys.attr("KEY_KP_8") = (int)KEY_KP_8;
    keys.attr("KEY_KP_9") = (int)KEY_KP_9;

    keys.attr("KEY_KP_DECIMAL")  = (int)KEY_KP_DECIMAL;
    keys.attr("KEY_KP_DIVIDE")   = (int)KEY_KP_DIVIDE;
    keys.attr("KEY_KP_MULTIPLY") = (int)KEY_KP_MULTIPLY;
    keys.attr("KEY_KP_SUBTRACT") = (int)KEY_KP_SUBTRACT;
    keys.attr("KEY_KP_ADD")      = (int)KEY_KP_ADD;
    keys.attr("KEY_KP_ENTER")    = (int)KEY_KP_ENTER;
    keys.attr("KEY_KP_EQUAL")    = (int)KEY_KP_EQUAL;

    keys.attr("KEY_BACK")        = (int)KEY_BACK;
    keys.attr("KEY_MENU")        = (int)KEY_MENU;
    keys.attr("KEY_VOLUME_UP")   = (int)KEY_VOLUME_UP;
    keys.attr("KEY_VOLUME_DOWN") = (int)KEY_VOLUME_DOWN;

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

    py::class_<Vector2>(m, "Vector2")
        .def(py::init<>()) // Default constructor
        .def(py::init<float, float>(), py::arg("x"), py::arg("y")) // Constructor with params
        // def_readwrite exposes the public member variable directly with direct memory access
        .def_readwrite("x", &Vector2::x)
        .def_readwrite("y", &Vector2::y);

    py::class_<Component>(m, "Component")
        .def(py::init<>())
        .def_property_readonly("game_object", [](Component& c) { return c.owner; }, py::return_value_policy::reference);

    py::class_<Transform2d, Component>(m, "Transform2d")
        .def(py::init<float, float>(), py::arg("x") = 0.0f, py::arg("y") = 0.0f)
        // Bind the actual Vector2 and float members
        .def_readwrite("position", &Transform2d::position)
        .def_readwrite("scale", &Transform2d::scale)
        .def_readwrite("rotation", &Transform2d::rotation);

    py::class_<SpriteRenderer, Component>(m, "SpriteRenderer")
        .def(py::init<std::string>());

    py::class_<SpriteSheetRenderer, Component>(m, "SpriteSheetRenderer")
        .def(py::init<std::string, int, int>())
        // We bind GetFrame and SetFrame to a clean Python property '.frame'
        .def_property("frame", &SpriteSheetRenderer::GetFrame, &SpriteSheetRenderer::SetFrame)
        // Read-only property for the total frame count
        .def_property_readonly("frame_count", &SpriteSheetRenderer::GetFrameCount);

    py::class_<Animation2d, Component>(m, "Animation2d")
        .def(py::init<std::vector<int>, float, bool>(), 
             py::arg("frames"), py::arg("speed"), py::arg("loop") = true);
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
