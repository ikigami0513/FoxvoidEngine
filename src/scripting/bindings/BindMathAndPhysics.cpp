#include "scripting/ScriptBindings.hpp"
#include <raylib.h>
#include <iostream>
#include "world/GameObject.hpp"
#include "physics/Transform2d.hpp"
#include <world/ComponentRegistry.hpp>
#include "physics/RectCollider.hpp"
#include "physics/RigidBody2d.hpp"
#include "physics/Collision2D.hpp"
#include <physics/PhysicsEngine.hpp>
#include <core/Engine.hpp>

void BindMathAndPhysics(py::module_& m) {
    py::class_<Vector2>(m, "Vector2")
        .def(py::init<>()) // Default constructor
        .def(py::init<float, float>(), py::arg("x"), py::arg("y")) // Constructor with params
        // def_readwrite exposes the public member variable directly with direct memory access
        .def_readwrite("x", &Vector2::x)
        .def_readwrite("y", &Vector2::y);

    py::class_<Rectangle>(m, "Rectangle")
        .def(py::init<>())
        .def(py::init<float, float, float, float>(), 
             py::arg("x"), py::arg("y"), py::arg("width"), py::arg("height"))
        .def_readwrite("x", &Rectangle::x)
        .def_readwrite("y", &Rectangle::y)
        .def_readwrite("width", &Rectangle::width)
        .def_readwrite("height", &Rectangle::height);

    py::class_<Transform2d, Component>(m, "Transform2d")
        .def(py::init<float, float>(), py::arg("x") = 0.0f, py::arg("y") = 0.0f)
        
        // Local properties
        .def_readwrite("position", &Transform2d::position)
        .def_readwrite("scale", &Transform2d::scale)
        .def_readwrite("rotation", &Transform2d::rotation)
        .def_readwrite("z_index", &Transform2d::zIndex)

        // Global Getters
        .def("get_global_position", &Transform2d::GetGlobalPosition)
        .def("get_global_rotation", &Transform2d::GetGlobalRotation)
        .def("get_global_scale", &Transform2d::GetGlobalScale)
        .def("get_global_z_index", &Transform2d::GetGlobalZIndex)
        
        // Global Setters
        .def("set_global_position", &Transform2d::SetGlobalPosition, py::arg("target_global_pos"))
        .def("set_global_rotation", &Transform2d::SetGlobalRotation, py::arg("target_global_rot"))
        .def("set_global_scale", &Transform2d::SetGlobalScale, py::arg("target_global_scale"));

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

    py::class_<Collision2D>(m, "Collision2D")
        .def_readwrite("other", &Collision2D::other, py::return_value_policy::reference)
        .def_readwrite("normal", &Collision2D::normal);

    py::class_<RectCollider, Component>(m, "RectCollider")
        .def(py::init<float, float>(), py::arg("width") = 50.0f, py::arg("height") = 50.0f)
        .def_readwrite("size", &RectCollider::size)
        .def_readwrite("offset", &RectCollider::offset)
        .def_readwrite("is_trigger", &RectCollider::isTrigger);

    ComponentRegistry::Register<RectCollider>("RectCollider",
        [](GameObject& go, py::args args) -> py::object {
            float width = 50.0f;
            float height = 50.0f;

            // Extract optional width and height if Python provided them
            if (args.size() >= 1) width = args[0].cast<float>();
            if (args.size() >= 2) height = args[1].cast<float>();

            auto* t = go.AddComponent<RectCollider>(width, height);
            return py::cast(t, py::return_value_policy::reference);
        }
    );

    py::class_<RigidBody2d, Component>(m, "RigidBody2d")
        .def(py::init<>())
        .def_readwrite("velocity", &RigidBody2d::velocity)
        .def_readwrite("mass", &RigidBody2d::mass)
        .def_readwrite("gravity_scale", &RigidBody2d::gravityScale)
        .def_readwrite("is_kinematic", &RigidBody2d::isKinematic)
        .def_readonly("is_grounded", &RigidBody2d::isGrounded);

    ComponentRegistry::Register<RigidBody2d>("RigidBody2d",
        [](GameObject& go, py::args args) -> py::object {
            auto* t = go.AddComponent<RigidBody2d>();
            return py::cast(t, py::return_value_policy::reference);
        }
    );

    py::class_<RaycastHit>(m, "RaycastHit")
        .def_readonly("hit", &RaycastHit::hit)
        .def_readonly("collider", &RaycastHit::collider) // Returns the GameObject
        .def_property_readonly("point", [](const RaycastHit& r) { return py::make_tuple(r.point.x, r.point.y); })
        .def_readonly("distance", &RaycastHit::distance);

    py::class_<PhysicsEngine>(m, "Physics")
        // Use a lambda to inject the active scene from the Engine
        .def_static("raycast", [](py::tuple origin, py::tuple direction, float distance) {
            Vector2 o = { origin[0].cast<float>(), origin[1].cast<float>() };
            Vector2 d = { direction[0].cast<float>(), direction[1].cast<float>() };
            Scene& activeScene = Engine::Get()->GetActiveScene();
            return PhysicsEngine::Raycast(activeScene, o, d, distance);
        });
}
