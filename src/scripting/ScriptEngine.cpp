#include "ScriptEngine.hpp"
#include "ScriptBindings.hpp"
#include <iostream>

PYBIND11_EMBEDDED_MODULE(foxvoid, m) {
    BindCore(m);
    BindInput(m);
    BindMathAndPhysics(m);
    BindGraphics(m);
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
