#pragma once

#include <pybind11/pybind11.h>
#include <unordered_map>
#include <functional>
#include <string>

#include "../world/GameObject.hpp"

namespace py = pybind11;

// Define function signatures for getters and adders
using ComponentGetter = std::function<py::object(GameObject&)>;
using ComponentAdder = std::function<py::object(GameObject&, py::args)>;

// Pure C++ Factory Signature
using ComponentFactory = std::function<Component*(GameObject&)>;

class ComponentRegistry {
    public:
        // C++17 inline statics: No need to instantiate them in a .cpp file!
        inline static std::unordered_map<std::string, ComponentGetter> getters;
        inline static std::unordered_map<std::string, ComponentAdder> adders;

        // Factory map and UI list
        inline static std::unordered_map<std::string, ComponentFactory> factories;

        // Vector to keep track of available components for the ImGui dropdown
        inline static std::vector<std::string> registeredTypes;

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

            // Register for C++ engine automatically
            RegisterCPP<T>(typeName);
        }

        // Pure C++ registration
        template<typename T>
        static void RegisterCPP(const std::string& typeName) {
            // Prevent duplicate entries in the UI list if registered multiple times
            if (factories.find(typeName) == factories.end()) {
                registeredTypes.push_back(typeName);
            }

            // Store the factory lambda
            factories[typeName] = [](GameObject& go) -> Component* {
                // Instantiates the component using its default constructor
                return go.AddComponent<T>();
            };
        }
};

