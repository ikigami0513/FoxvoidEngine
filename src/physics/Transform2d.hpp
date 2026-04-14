#pragma once

#include "world/Component.hpp"
#include <raylib.h>
#include <imgui.h>
#include <editor/commands/CommandHistory.hpp>
#include <editor/commands/ModifyComponentCommand.hpp>

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
            // Static variable to hold the JSON state before the user starts dragging a value
            static nlohmann::json initialState;

            // --- POSITION ---
            ImGui::DragFloat2("Position", &position.x, 0.1f);
            
            if (ImGui::IsItemActivated()) {
                // The user just clicked on the Position slider. Save the whole component state!
                initialState = Serialize();
            }
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                // The user released the mouse. Push the command!
                CommandHistory::AddCommand(std::make_unique<ModifyComponentCommand>(this, initialState, Serialize()));
            }

            // --- SCALE ---
            ImGui::DragFloat2("Scale", &scale.x, 0.1f);
            
            if (ImGui::IsItemActivated()) {
                initialState = Serialize();
            }
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                CommandHistory::AddCommand(std::make_unique<ModifyComponentCommand>(this, initialState, Serialize()));
            }

            // --- ROTATION ---
            ImGui::DragFloat("Rotation", &rotation, 1.0f);
            
            if (ImGui::IsItemActivated()) {
                initialState = Serialize();
            }
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                CommandHistory::AddCommand(std::make_unique<ModifyComponentCommand>(this, initialState, Serialize()));
            }
        }

        nlohmann::json Serialize() const override {
            return {
                { "type", "Transform2d" },
                { "x", position.x },
                { "y", position.y },
                { "scaleX", scale.x },
                { "scaleY", scale.y },
                { "rotation", rotation }
            };
        }

        void Deserialize(const nlohmann::json& j) override {
            position.x = j.value("x", 0.0f);
            position.y = j.value("y", 0.0f);
            scale.x    = j.value("scaleX", 1.0f);
            scale.y    = j.value("scaleY", 1.0f);
            rotation   = j.value("rotation", 0.0f);
        }
};
