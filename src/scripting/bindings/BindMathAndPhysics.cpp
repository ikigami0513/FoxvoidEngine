#include "../ScriptBindings.hpp"
#include <raylib.h>
#include <iostream>
#include "../../world/GameObject.hpp"
#include "../../physics/Transform2d.hpp"
#include <world/ComponentRegistry.hpp>

void BindMathAndPhysics(py::module_& m) {
    py::class_<Vector2>(m, "Vector2")
        .def(py::init<>()) // Default constructor
        .def(py::init<float, float>(), py::arg("x"), py::arg("y")) // Constructor with params
        // def_readwrite exposes the public member variable directly with direct memory access
        .def_readwrite("x", &Vector2::x)
        .def_readwrite("y", &Vector2::y);

    py::class_<Transform2d, Component>(m, "Transform2d")
        .def(py::init<float, float>(), py::arg("x") = 0.0f, py::arg("y") = 0.0f)
        // Bind the actual Vector2 and float members
        .def_readwrite("position", &Transform2d::position)
        .def_readwrite("scale", &Transform2d::scale)
        .def_readwrite("rotation", &Transform2d::rotation);

    ComponentRegistry::Register<Transform2d>("Transform2d", 
        [](GameObject& go, py::args args) -> py::object {
            float x = 0.0f, y = 0.0f;
            
            // Extract optional X and Y if Python provided them
            if (args.size() >= 1) x = args[0].cast<float>();
            if (args.size() >= 2) y = args[1].cast<float>();
            
            auto* t = go.AddComponent<Transform2d>(x, y);
            return py::cast(t, py::return_value_policy::reference);
        }
    );
}
