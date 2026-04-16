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

        // Casts a ray against all colliders and tilemaps in the scene.
        // Returns the closest hit, if any.
        static RaycastHit Raycast(Scene& scene, Vector2 origin, Vector2 direction, float maxDistance);

        // Draws the collision boxes for debugging purposes
        static void RenderDebug(Scene& scene);

    private:
        // Helper function to calculate AABB overlap and push objects apart
        static bool ResolveCollision(GameObject* objA, GameObject* objB, Vector2& outNormalA, Vector2& outNormalB);

        // Helper function to resolve collision against static TileMap geometry
        static bool ResolveTileCollision(GameObject* obj, const Rectangle& tileRect, Vector2& outNormal);

        // Helper to check line vs AABB intersection
        static bool CheckLineBoxIntersection(Vector2 p1, Vector2 p2, Rectangle box, Vector2& hitPoint, Vector2& hitNormal);
};
