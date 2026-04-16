#include "../ScriptBindings.hpp"
#include <pybind11/stl.h>
#include <iostream>
#include "../../world/GameObject.hpp"
#include "../../graphics/Graphics.hpp"
#include "../../graphics/SpriteRenderer.hpp"
#include "../../graphics/SpriteSheetRenderer.hpp"
#include "../../graphics/Animation2d.hpp"
#include "../../graphics/Animator2d.hpp"
#include "graphics/Camera2d.hpp"
#include <world/ComponentRegistry.hpp>
#include "graphics/TileMap.hpp"

void BindGraphics(py::module_& m) {
    m.def("set_pixel_art_mode", [](bool enable) {
        Graphics::pixelArtMode = enable;
    });
    
    m.def("is_pixel_art_mode", []() {
        return Graphics::pixelArtMode;
    });

    py::class_<SpriteRenderer, Component>(m, "SpriteRenderer")
        .def(py::init<std::string>());

    ComponentRegistry::Register<SpriteRenderer>("SpriteRenderer", 
        [](GameObject& go, py::args args) -> py::object {
            if (args.size() < 1) {
                std::cerr << "[Python] SpriteRenderer needs a texture path!" << std::endl;
                return py::none();
            }
            std::string path = args[0].cast<std::string>();
            auto* s = go.AddComponent<SpriteRenderer>(path);
            return py::cast(s, py::return_value_policy::reference);
        }
    );

    py::class_<SpriteSheetRenderer, Component>(m, "SpriteSheetRenderer")
        .def(py::init<std::string, int, int>())
        // We bind GetFrame and SetFrame to a clean Python property '.frame'
        .def_property("frame", &SpriteSheetRenderer::GetFrame, &SpriteSheetRenderer::SetFrame)
        // Read-only property for the total frame count
        .def_property_readonly("frame_count", &SpriteSheetRenderer::GetFrameCount);

    ComponentRegistry::Register<SpriteSheetRenderer>("SpriteSheetRenderer", 
        [](GameObject& go, py::args args) -> py::object {
            if (args.size() < 3) {
                std::cerr << "[Python] SpriteSheetRenderer requires (texture_path, columns, rows)!" << std::endl;
                return py::none();
            }
            std::string path = args[0].cast<std::string>();
            int cols = args[1].cast<int>();
            int rows = args[2].cast<int>();
            auto* s = go.AddComponent<SpriteSheetRenderer>(path, cols, rows);
            return py::cast(s, py::return_value_policy::reference);
        }
    );

    py::class_<Animation2d, Component>(m, "Animation2d")
        .def(py::init<std::vector<int>, float, bool, bool, bool>(), 
             py::arg("frames"), py::arg("speed"), py::arg("loop") = true, py::arg("flip_x") = false, py::arg("flip_y") = false);

    ComponentRegistry::Register<Animation2d>("Animation2d", 
        [](GameObject& go, py::args args) -> py::object {
            if (args.size() < 2) {
                std::cerr << "[Python] Animation2d requires (frames_list, speed, [loop])!" << std::endl;
                return py::none();
            }
            std::vector<int> frames = args[0].cast<std::vector<int>>();
            float speed = args[1].cast<float>();
            bool loop = true; // Default value
            
            if (args.size() >= 3) {
                loop = args[2].cast<bool>();
            }
            
            auto* a = go.AddComponent<Animation2d>(frames, speed, loop);
            return py::cast(a, py::return_value_policy::reference);
        }
    );

    py::class_<Animator2d, Component>(m, "Animator2d")
        .def(py::init<>())
        .def("add_animation", &Animator2d::AddAnimation, 
            py::arg("name"), py::arg("frames"), py::arg("frame_duration"), py::arg("loop"),
            py::arg("flip_x") = false, py::arg("flip_y") = false)
        .def("play", &Animator2d::Play);

    ComponentRegistry::Register<Animator2d>("Animator2d", 
        [](GameObject& go, py::args args) -> py::object {
            // Animator2d does not require any arguments upon creation.
            // Animations are added later via the 'add_animation' method in Python.
            auto* animator = go.AddComponent<Animator2d>();
            
            // Return the reference to Python
            return py::cast(animator, py::return_value_policy::reference);
        }
    );

    py::enum_<Camera2dAnchor>(m, "Camera2dAnchor")
        .value("TopLeft", Camera2dAnchor::TopLeft)
        .value("Center", Camera2dAnchor::Center)
        .export_values();

    py::class_<Camera2d, Component>(m, "Camera2d")
        .def(py::init<>())
        .def_readwrite("zoom", &Camera2d::zoom)
        .def_readwrite("offset", &Camera2d::offset)
        .def_readwrite("anchor", &Camera2d::anchor)
        .def_readwrite("is_main", &Camera2d::isMain);

    ComponentRegistry::Register<Camera2d>("Camera2d",
        [](GameObject& go, py::args args) -> py::object {
            auto* cam = go.AddComponent<Camera2d>();
            return py::cast(cam, py::return_value_policy::reference);
        }
    );

    py::class_<TileLayer>(m, "TileLayer")
        .def_readwrite("name", &TileLayer::name)
        .def_readwrite("is_visible", &TileLayer::isVisible)
        .def_readwrite("is_solid", &TileLayer::isSolid);

    py::class_<TileMap, Component>(m, "TileMap")
        .def(py::init<>())
        // Expose public properties
        .def_readwrite("grid_width", &TileMap::gridWidth)
        .def_readwrite("grid_height", &TileMap::gridHeight)
        .def_readwrite("tile_spacing", &TileMap::tileSpacing)
        // Expose public methods
        .def("load_tileset", &TileMap::LoadTileset)
        .def("resize", &TileMap::ResizeMap)
        .def("add_layer", &TileMap::AddLayer)
        .def("get_tile", &TileMap::GetTile)
        .def("set_tile", &TileMap::SetTile)
        // Overloaded methods to get a layer by index or by name
        .def("get_layer", py::overload_cast<int>(&TileMap::GetLayer), py::return_value_policy::reference_internal)
        .def("get_layer", py::overload_cast<const std::string&>(&TileMap::GetLayer), py::return_value_policy::reference_internal);

    ComponentRegistry::Register<TileMap>("TileMap", 
        [](GameObject& go, py::args args) -> py::object {
            // TileMap does not require constructor arguments
            auto* tileMap = go.AddComponent<TileMap>();
            return py::cast(tileMap, py::return_value_policy::reference);
        }
    );
}
