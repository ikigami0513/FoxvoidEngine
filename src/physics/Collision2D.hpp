#pragma once

#include <raylib.h>

class GameObject;

// Data structure to send to our python scripts
struct Collision2D {
    GameObject* other; // The object we hit (can be nullptr if we hit the TileMap)
    Vector2 normal; // The direction of the surface we hit
};
