#pragma once

#include "world/GameObject.hpp"
#include <raylib.h>

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
        static void ResolveCollision(GameObject* objA, GameObject* objB);
};
