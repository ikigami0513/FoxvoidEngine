#pragma once
#include <pybind11/pybind11.h>
#include <unordered_map>
#include <functional>
#include <string>
#include "../world/GameObject.hpp"

namespace py = pybind11;

// Forward declarations of binding functions
void BindInput(py::module_& m);
void BindMathAndPhysics(py::module_& m);
void BindGraphics(py::module_& m);
void BindCore(py::module_& m);

// Define function signatures for getters and adders
using ComponentGetter = std::function<py::object(GameObject&)>;
using ComponentAdder = std::function<py::object(GameObject&, py::args)>;

class ComponentRegistry {
    public:
        // C++17 inline statics: No need to instantiate them in a .cpp file!
        inline static std::unordered_map<std::string, ComponentGetter> getters;
        inline static std::unordered_map<std::string, ComponentAdder> adders;

        template<typename T>
        static void Register(const std::string& typeName, ComponentAdder adderLogic) {
            // Automatically generate the GET logic for this type
            getters[typeName] = [](GameObject& go) -> py::object {
                T* comp = go.GetComponent<T>();
                if (comp) return py::cast(comp, py::return_value_policy::reference);
                return py::none();
            };

            // Store the custom ADD logic provided by the component's module
            adders[typeName] = adderLogic;
        }
};
