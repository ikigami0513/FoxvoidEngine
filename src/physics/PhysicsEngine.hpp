#pragma once

#include "world/GameObject.hpp"
#include <raylib.h>
#include "Collision2D.hpp"

class Scene;

class PhysicsEngine {
    public:
        // Global gravity force.
        static Vector2 GlobalGravity;

        // The main function to call every frame in our game loop
        static void Update(Scene& scene, float deltaTime);

        // Draws the collision boxes for debugging purposes
        static void RenderDebug(Scene& scene);

    private:
        // Helper function to calculate AABB overlap and push objects apart
        static bool ResolveCollision(GameObject* objA, GameObject* objB, Vector2& outNormalA, Vector2& outNormalB);

        // Helper function to resolve collision against static TileMap geometry
        static bool ResolveTileCollision(GameObject* obj, const Rectangle& tileRect, Vector2& outNormal);
};
