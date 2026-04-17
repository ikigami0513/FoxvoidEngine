#pragma once

#include "world/Component.hpp"
#include "world/GameObject.hpp"
#include "physics/Transform2d.hpp"
#include <raylib.h>

#ifndef STANDALONE_MODE
#include "editor/EditorUI.hpp"
#include <imgui.h>
#endif

class ShapeRenderer : public Component {
    public:
        Color color;
        float width;
        float height;
        bool isHUD;

        ShapeRenderer(float w = 50.0f, float h = 50.0f, Color c = WHITE, bool hud = false) 
            : width(w), height(h), color(c), isHUD(hud) {}

        // We only override Render, as this component doesn't need to update logic
        void Render() override {
            // Skip rendering in world space if marked as HUD
            if (isHUD || !owner) return;

            // Fetch the Transform component from the parent GameObject
            Transform2d* transform = owner->GetComponent<Transform2d>();
            
            // Safety check: only draw if the GameObject actually has a Transform
            if (transform != nullptr) {
                
                // Create a Raylib Rectangle definition
                // We multiply width and height by the transform's scale
                Rectangle rec = { 
                    transform->position.x, 
                    transform->position.y, 
                    width * transform->scale.x, 
                    height * transform->scale.y 
                };
                
                // Define the origin point for rotation (center of the rectangle)
                Vector2 origin = { rec.width / 2.0f, rec.height / 2.0f };
                
                // Draw the rotated rectangle
                DrawRectanglePro(rec, origin, transform->rotation, color);
            }
        }

        void RenderHUD() override {
            if (!isHUD || !owner) return;

            Transform2d* transform = owner->GetComponent<Transform2d>();
            
            if (transform != nullptr) {
                Rectangle rec = { 
                    transform->position.x, 
                    transform->position.y, 
                    width * transform->scale.x, 
                    height * transform->scale.y 
                };
                
                Vector2 origin = { rec.width / 2.0f, rec.height / 2.0f };
                DrawRectanglePro(rec, origin, transform->rotation, color);
            }
        }

        std::string GetName() const override {
            return "Shape Renderer";
        }

#ifndef STANDALONE_MODE
        void OnInspector() override {
            EditorUI::DragFloat("Width", &width, 1.0f, this, 0.0f, 10000.0f);
            EditorUI::DragFloat("Height", &height, 1.0f, this, 0.0f, 10000.0f);

            EditorUI::ColorEdit4("Color", &color, this);

            ImGui::Separator();
            EditorUI::Checkbox("Is HUB (Screen Space)", &isHUD, this);
        }
#endif

        nlohmann::json Serialize() const override {
            return {
                { "type", "ShapeRenderer" },
                { "width", width },
                { "height", height },
                { "color", { color.r, color.g, color.b, color.a }},
                { "isHUD", isHUD }
            };
        }

        void Deserialize(const nlohmann::json& j) override {
            width = j.value("width", 50.0f);
            height = j.value("height", 50.0f);
            isHUD = j.value("isHUD", false);
            
            // Safely extract the color array
            if (j.contains("color") && j["color"].is_array() && j["color"].size() == 4) {
                color.r = j["color"][0];
                color.g = j["color"][1];
                color.b = j["color"][2];
                color.a = j["color"][3];
            }
        }
};
