#include "../ScriptBindings.hpp"
#include <pybind11/stl.h>
#include "../../graphics/Graphics.hpp"
#include "../../graphics/SpriteRenderer.hpp"
#include "../../graphics/SpriteSheetRenderer.hpp"
#include "../../graphics/Animation2d.hpp"

void BindGraphics(py::module_& m) {
    m.def("set_pixel_art_mode", [](bool enable) {
        Graphics::pixelArtMode = enable;
    });
    
    m.def("is_pixel_art_mode", []() {
        return Graphics::pixelArtMode;
    });

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
