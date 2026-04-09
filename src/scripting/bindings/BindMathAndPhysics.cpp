#include "../ScriptBindings.hpp"
#include <raylib.h>
#include "../../physics/Transform2d.hpp"

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
}
