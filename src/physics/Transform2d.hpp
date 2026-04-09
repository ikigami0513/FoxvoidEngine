#pragma once

#include "world/Component.hpp"
#include <raylib.h>

class Transform2d : public Component {
    public:
        Vector2 position;
        float rotation;  // In degrees, as Raylib expects degrees for drawing
        Vector2 scale;

        // Constructor with default values
        Transform2d(float x = 0.0f, float y = 0.0f) 
            : position{x, y}, rotation(0.0f), scale{1.0f, 1.0f} {}
};
