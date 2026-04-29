#include "scripting/ScriptBindings.hpp"
#include "gui/TextRenderer.hpp"
#include "world/Component.hpp"
#include "world/ComponentRegistry.hpp"
#include "gui/Button.hpp"
#include "gui/RectTransform.hpp"
#include "gui/ImageRenderer.hpp"

void BindGUI(py::module_& m) {
    py::class_<TextRenderer, Component>(m, "TextRenderer")
        .def(py::init<>())
        .def_readwrite("text", &TextRenderer::text)
        .def_readwrite("font_size", &TextRenderer::fontSize)
        .def_readwrite("is_hud", &TextRenderer::isHUD)
        .def_property("font_path",
            [](TextRenderer& t) { return t.GetFontPath(); },
            [](TextRenderer& t, const std::string& path) { t.SetFontPath(path); }
        );

    ComponentRegistry::Register<TextRenderer>("TextRenderer",
        [](GameObject& go, py::args args) -> py::object {
            auto* t = go.AddComponent<TextRenderer>();
            return py::cast(t, py::return_value_policy::reference);
        }
    );

    py::enum_<ButtonState>(m, "ButtonState")
        .value("Normal", ButtonState::Normal)
        .value("Hovered", ButtonState::Hovered)
        .value("Pressed", ButtonState::Pressed)
        .export_values();

    py::class_<Button, Component>(m, "Button")
        .def(py::init<>())
        .def_readwrite("width", &Button::width)
        .def_readwrite("height", &Button::height)
        .def_readwrite("is_hud", &Button::isHUD)
        .def_readwrite("normal_color", &Button::normalColor)
        .def_readwrite("hover_color", &Button::hoverColor)
        .def_readwrite("pressed_color", &Button::pressedColor)
        // Methods to check interactions
        .def("is_clicked", &Button::IsClicked)
        .def("get_state", &Button::GetState);

    ComponentRegistry::Register<Button>("Button",
        [](GameObject& go, py::args args) -> py::object {
            auto* b = go.AddComponent<Button>();
            return py::cast(b, py::return_value_policy::reference);
        }
    );

    py::class_<RectTransform, Component>(m, "RectTransform")
        .def(py::init<>())
        .def_readwrite("size", &RectTransform::size)
        .def_readwrite("position", &RectTransform::position)
        .def_readwrite("anchor", &RectTransform::anchor)
        .def_readwrite("pivot", &RectTransform::pivot)
        .def("get_screen_rect", &RectTransform::GetScreenRect);

    ComponentRegistry::Register<RectTransform>("RectTransform",
        [](GameObject& go, py::args args) -> py::object {
            auto* rt = go.AddComponent<RectTransform>();
            return py::cast(rt, py::return_value_policy::reference);
        }
    );

    py::class_<ImageRenderer, Component>(m, "ImageRenderer")
        .def(py::init<const std::string&>(), py::arg("texture_path") = "")
        .def_readwrite("color", &ImageRenderer::color)
        .def_readwrite("is_hud", &ImageRenderer::isHUD)
        .def("set_texture", py::overload_cast<const std::string&>(&ImageRenderer::SetTexture), py::arg("path"));

    ComponentRegistry::Register<ImageRenderer>("ImageRenderer",
        [](GameObject& go, py::args args) -> py::object {
            std::string path = "";
            if (args.size() >= 1) path = args[0].cast<std::string>();
            
            auto* ir = go.AddComponent<ImageRenderer>(path);
            return py::cast(ir, py::return_value_policy::reference);
        }
    );
}
