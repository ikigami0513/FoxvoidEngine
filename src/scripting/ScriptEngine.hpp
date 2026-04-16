#pragma once

#include <string>
#include <filesystem>
// Essential pybind11 headers for embedding the Python interpreter
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>

namespace py = pybind11;

class ScriptEngine {
public:
    // Initializes the Python interpreter
    static void Initialize();

    // Safely shuts down the Python interpreter
    static void Shutdown();

    // Appends a directory to Python's sys.path
    static void AddScriptPath(const std::filesystem::path& path);

    // Imports a Python module (equivalent to a .py file)
    static void LoadModule(const std::string& moduleName);

private:
    // Pointer to keep the interpreter alive for the duration of the game
    static py::scoped_interpreter* s_interpreter;
};
