#pragma once

#include "world/Component.hpp"
#include "world/GameObject.hpp"
#include "physics/Transform2d.hpp"
#include <raylib.h>
#include <imgui.h>

class ShapeRenderer : public Component {
    public:
        Color color;
        float width;
        float height;

        ShapeRenderer(float w = 50.0f, float h = 50.0f, Color c = WHITE) 
            : width(w), height(h), color(c) {}

        // We only override Render, as this component doesn't need to update logic
        void Render() override {
            // 1. Fetch the Transform component from the parent GameObject
            Transform2d* transform = owner->GetComponent<Transform2d>();
            
            // 2. Safety check: only draw if the GameObject actually has a Transform
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

        std::string GetName() const override {
            return "Shape Renderer";
        }

        void OnInspector() override {
            ImGui::DragFloat("Width", &width, 1.0f, 0.0f, 10000.0f);
            ImGui::DragFloat("Height", &height, 1.0f, 0.0f, 10000.0f);

            // ImGui expects colors as float[4] (0.0f to 1.0f)
            // Raylib stores them as unsigned char (0 to 255)
            float c[4] = { 
                color.r / 255.0f, 
                color.g / 255.0f, 
                color.b / 255.0f, 
                color.a / 255.0f 
            };

            // ImGui::ColorEdit4 provides a nice color picker
            if (ImGui::ColorEdit4("Color", c)) {
                // Convert back to Raylib format if the user changed the color
                color.r = static_cast<unsigned char>(c[0] * 255.0f);
                color.g = static_cast<unsigned char>(c[1] * 255.0f);
                color.b = static_cast<unsigned char>(c[2] * 255.0f);
                color.a = static_cast<unsigned char>(c[3] * 255.0f);
            }
        }

        nlohmann::json Serialize() const override {
            return {
                { "type", "ShapeRenderer" },
                { "width", width },
                { "height", height },
                { "color", { color.r, color.g, color.b, color.a }}
            };
        }

        void Deserialize(const nlohmann::json& j) override {
            width = j.value("width", 50.0f);
            height = j.value("height", 50.0f);
            
            // Safely extract the color array
            if (j.contains("color") && j["color"].is_array() && j["color"].size() == 4) {
                color.r = j["color"][0];
                color.g = j["color"][1];
                color.b = j["color"][2];
                color.a = j["color"][3];
            }
        }
};
