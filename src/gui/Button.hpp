#pragma once

#include "world/Component.hpp"
#include <raylib.h>
#include <nlohmann/json.hpp>
#include <string>

enum class ButtonState {
    Normal,
    Hovered,
    Pressed
};

class Button : public Component {
    public:
        float width;
        float height;
        bool isHUD;

        // Visual feedback colors (modifies a GuiRect if one is attached)
        Color normalColor;
        Color hoverColor;
        Color pressedColor;

        Button();
        ~Button() override = default;

        void Update(float deltaTime) override;

        std::string GetName() const override;

#ifndef STANDALONE_MODE
        void OnInspector() override;
#endif

        nlohmann::json Serialize() const override;
        void Deserialize(const nlohmann::json& j) override;

        // API for Python to check if the button was clicked this frame
        bool IsClicked() const;
        
        // Expose state if needed
        ButtonState GetState() const { return m_state; }

    private:
        ButtonState m_state;
        bool m_wasClicked;
};
