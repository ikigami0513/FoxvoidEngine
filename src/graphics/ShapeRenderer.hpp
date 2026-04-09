#pragma once

#include "world/Component.hpp"
#include "world/GameObject.hpp"
#include "physics/Transform2d.hpp"
#include <raylib.h>

class ShapeRenderer : public Component {
    public:
        Color color;
        float width;
        float height;

        ShapeRenderer(float w, float h, Color c) 
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
};
