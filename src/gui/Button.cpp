#include "Button.hpp"
#include "physics/Transform2d.hpp"
#include "graphics/ShapeRenderer.hpp"
#include "world/GameObject.hpp"
#include "core/Engine.hpp"

#ifndef STANDALONE_MODE
#include "editor/EditorUI.hpp"
#endif

Button::Button() 
    : width(100.0f), height(40.0f), isHUD(true), 
      normalColor(GRAY), hoverColor(LIGHTGRAY), pressedColor(DARKGRAY),
      m_state(ButtonState::Normal), m_wasClicked(false) {}

void Button::Update(float deltaTime) {
    if (!owner) return;
    
    auto transform = owner->GetComponent<Transform2d>();
    if (!transform) return;

    m_wasClicked = false;

    // Centered Bounding Box
    // Because ShapeRenderer draws from the center, we must shift the hitbox 
    // backwards by half its width and height so the click area matches the visual shape.
    float scaledWidth = width * transform->scale.x;
    float scaledHeight = height * transform->scale.y;
    
    Rectangle bounds = {
        transform->position.x - (scaledWidth / 2.0f),
        transform->position.y - (scaledHeight / 2.0f),
        scaledWidth,
        scaledHeight
    };

    // Get the correct mouse position depending on HUD or World space
    Vector2 mousePos = GetMousePosition();
    
    if (!isHUD) {
        if (Engine::Get()) {
            Camera2D cam = Engine::Get()->GetActiveScene().GetMainCamera(GetScreenWidth(), GetScreenHeight());
            mousePos = GetScreenToWorld2D(mousePos, cam);
        }
    }

    // Check interactions
    if (CheckCollisionPointRec(mousePos, bounds)) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            m_state = ButtonState::Pressed;
        } else if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            m_state = ButtonState::Hovered;
            m_wasClicked = true; 
        } else {
            m_state = ButtonState::Hovered;
        }
    } else {
        m_state = ButtonState::Normal;
    }

    // Update the color of a ShapeRenderer if one is attached
    if (auto shape = owner->GetComponent<ShapeRenderer>()) {
        switch (m_state) {
            case ButtonState::Normal:  shape->color = normalColor; break;
            case ButtonState::Hovered: shape->color = hoverColor; break;
            case ButtonState::Pressed: shape->color = pressedColor; break;
        }
    }
}  

bool Button::IsClicked() const {
    return m_wasClicked;
}

std::string Button::GetName() const {
    return "Button";
}

#ifndef STANDALONE_MODE
void Button::OnInspector() {
    EditorUI::DragFloat("Hitbox Width", &width, 1.0f, this, 0.0f, 2000.0f);
    EditorUI::DragFloat("Hitbox Height", &height, 1.0f, this, 0.0f, 2000.0f);
    
    ImGui::Separator();
    
    EditorUI::ColorEdit4("Normal Color", &normalColor, this);
    EditorUI::ColorEdit4("Hover Color", &hoverColor, this);
    EditorUI::ColorEdit4("Pressed Color", &pressedColor, this);

    ImGui::Separator();
    EditorUI::Checkbox("Is HUD (Screen Space)", &isHUD, this);
    
    // Debug info
    ImGui::TextDisabled(m_state == ButtonState::Normal ? "State: Normal" : 
                       (m_state == ButtonState::Hovered ? "State: Hovered" : "State: Pressed"));
}
#endif

nlohmann::json Button::Serialize() const {
    return {
        {"type", "Button"},
        {"width", width},
        {"height", height},
        {"isHUD", isHUD},
        {"n_r", normalColor.r}, {"n_g", normalColor.g}, {"n_b", normalColor.b}, {"n_a", normalColor.a},
        {"h_r", hoverColor.r}, {"h_g", hoverColor.g}, {"h_b", hoverColor.b}, {"h_a", hoverColor.a},
        {"p_r", pressedColor.r}, {"p_g", pressedColor.g}, {"p_b", pressedColor.b}, {"p_a", pressedColor.a}
    };
}

void Button::Deserialize(const nlohmann::json& j) {
    width = j.value("width", 100.0f);
    height = j.value("height", 40.0f);
    isHUD = j.value("isHUD", true);
    
    // Explicitly cast the integers returned by JSON to unsigned char
    // to prevent C++ narrowing conversion errors
    normalColor = { 
        static_cast<unsigned char>(j.value("n_r", 130)), 
        static_cast<unsigned char>(j.value("n_g", 130)), 
        static_cast<unsigned char>(j.value("n_b", 130)), 
        static_cast<unsigned char>(j.value("n_a", 255)) 
    };
    
    hoverColor = { 
        static_cast<unsigned char>(j.value("h_r", 200)), 
        static_cast<unsigned char>(j.value("h_g", 200)), 
        static_cast<unsigned char>(j.value("h_b", 200)), 
        static_cast<unsigned char>(j.value("h_a", 255)) 
    };
    
    pressedColor = { 
        static_cast<unsigned char>(j.value("p_r", 80)), 
        static_cast<unsigned char>(j.value("p_g", 80)), 
        static_cast<unsigned char>(j.value("p_b", 80)), 
        static_cast<unsigned char>(j.value("p_a", 255)) 
    };
}
