#pragma once

#include "world/Component.hpp"
#include <raylib.h>
#include <imgui.h>

class Transform2d : public Component {
    public:
        Vector2 position;
        float rotation;  // In degrees, as Raylib expects degrees for drawing
        Vector2 scale;

        // Constructor with default values
        Transform2d(float x = 0.0f, float y = 0.0f) 
            : position{x, y}, rotation(0.0f), scale{1.0f, 1.0f} {}

        std::string GetName() const override {
            return "Transform 2D";
        }

        void OnInspector() override {
            // ImGui::DragFloat2 allows modifying an X/Y struct easily
            // The 0.1f is the speed at which the value changes when dragging
            ImGui::DragFloat2("Position", &position.x, 0.1f);
            ImGui::DragFloat2("Scale", &scale.x, 0.1f);

            // ImGui::DragFloat is for single values
            ImGui::DragFloat("Rotation", &rotation, 1.0f);
        }
};
