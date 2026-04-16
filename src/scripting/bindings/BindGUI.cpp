#include "scripting/ScriptBindings.hpp"
#include "gui/TextRenderer.hpp"
#include "world/Component.hpp"
#include "world/ComponentRegistry.hpp"

void BindGUI(py::module_& m) {
    py::class_<TextRenderer, Component>(m, "TextRenderer")
        .def(py::init<>())
        .def_readwrite("text", &TextRenderer::text)
        .def_readwrite("font_size", &TextRenderer::fontSize)
        .def_readwrite("is_hud", &TextRenderer::isHUD)
        .def_property("font_path",
            [](TextRenderer& t) { return t.fontPath; },
            [](TextRenderer& t, const std::string& path) { t.SetFontPath(path); }
        );

    ComponentRegistry::Register<TextRenderer>("TextRenderer",
        [](GameObject& go, py::args args) -> py::object {
            auto* t = go.AddComponent<TextRenderer>();
            return py::cast(t, py::return_value_policy::reference);
        }
    );
}
