#include "PhysicsEngine.hpp"
#include "physics/Transform2d.hpp"
#include "physics/RigidBody2d.hpp"
#include "physics/RectCollider.hpp"
#include "world/Scene.hpp"
#include <raymath.h>
#include <cmath>

// Initialize global gravity (981 pixels/sec² pointing down)
Vector2 PhysicsEngine::GlobalGravity = { 0.0f, 981.0f };

void PhysicsEngine::Update(Scene& scene, float deltaTime) {
    // Get all game objects from the scene
    const auto& gameObjects = scene.GetGameObjects();

    // Step 1: Integration (Apply Gravity and Velocity)
    for (const auto& go : gameObjects) {
        auto rb = go->GetComponent<RigidBody2d>();
        auto transform = go->GetComponent<Transform2d>();

        // Only move objects that have physics and are not kinematic
        if (rb && transform && !rb->isKinematic) {
            // Apply global gravity based on the object's personal scale
            rb->velocity.x += GlobalGravity.x * rb->gravityScale * deltaTime;
            rb->velocity.y += GlobalGravity.y * rb->gravityScale * deltaTime;

            // Apply the velocity to the actual position
            transform->position.x += rb->velocity.x * deltaTime;
            transform->position.y += rb->velocity.y * deltaTime;
        }
    }

    // Step 2: Collision resolution (O(N²) checks)
    for (size_t i = 0; i < gameObjects.size(); ++i) {
        for (size_t j = i + 1; j < gameObjects.size(); ++j) {
            ResolveCollision(gameObjects[i].get(), gameObjects[j].get());
        }
    }
}

void PhysicsEngine::ResolveCollision(GameObject* objA, GameObject* objB) {
    auto colA = objA->GetComponent<RectCollider>();
    auto colB = objB->GetComponent<RectCollider>();

    // Both objects must have colliders to collide
    if (!colA || !colB) return;

    // Triggers don't physically push each other
    if (colA->isTrigger || colB->isTrigger) return;

    auto tA = objA->GetComponent<Transform2d>();
    auto tB = objB->GetComponent<Transform2d>();
    if (!tA || !tB) return;

    auto rbA = objA->GetComponent<RigidBody2d>();
    auto rbB = objB->GetComponent<RigidBody2d>();

    // Determine if objects can be pushed. 
    // An object without a RigidBody is considered kinematic (like a solid wall).
    bool isKinematicA = !rbA || rbA->isKinematic;
    bool isKinematicB = !rbB || rbB->isKinematic;

    // If both are solid/kinematic, they can't push each other anyway
    if (isKinematicA && isKinematicB) return;

    // Define the actual world-space rectangles
    Rectangle recA = {
        tA->position.x + colA->offset.x,
        tA->position.y + colA->offset.y,
        colA->size.x * tA->scale.x,
        colA->size.y * tA->scale.y
    };

    Rectangle recB = {
        tB->position.x + colB->offset.x,
        tB->position.y + colB->offset.y,
        colB->size.x * tB->scale.x,
        colB->size.y * tB->scale.y
    };

    // Check if they overlap using Raylib
    if (CheckCollisionRecs(recA, recB)) {
        
        // Calculate the center points of both rectangles
        float centerA_x = recA.x + recA.width / 2.0f;
        float centerB_x = recB.x + recB.width / 2.0f;
        float centerA_y = recA.y + recA.height / 2.0f;
        float centerB_y = recB.y + recB.height / 2.0f;

        // Calculate the depth of the penetration on both axes
        float overlapX = (recA.width / 2.0f + recB.width / 2.0f) - std::abs(centerA_x - centerB_x);
        float overlapY = (recA.height / 2.0f + recB.height / 2.0f) - std::abs(centerA_y - centerB_y);

        // We only resolve if there is a true overlap
        if (overlapX > 0 && overlapY > 0) {
            
            // The golden rule of AABB collision: 
            // Always push objects out along the axis of LEAST penetration.
            if (overlapX < overlapY) {
                // Resolve on the X-axis (Horizontal collision)
                float sign = (centerA_x < centerB_x) ? -1.0f : 1.0f;
                
                if (!isKinematicA && isKinematicB) {
                    tA->position.x += overlapX * sign;
                    if (rbA) rbA->velocity.x = 0; // Stop momentum on impact
                } 
                else if (isKinematicA && !isKinematicB) {
                    tB->position.x -= overlapX * sign;
                    if (rbB) rbB->velocity.x = 0;
                } 
                else {
                    // Both dynamic: split the push equally (or by mass later!)
                    tA->position.x += (overlapX / 2.0f) * sign;
                    tB->position.x -= (overlapX / 2.0f) * sign;
                    if (rbA) rbA->velocity.x = 0;
                    if (rbB) rbB->velocity.x = 0;
                }
            } 
            else {
                // Resolve on the Y-axis (Vertical collision, e.g., hitting the floor)
                float sign = (centerA_y < centerB_y) ? -1.0f : 1.0f;

                if (!isKinematicA && isKinematicB) {
                    tA->position.y += overlapY * sign;
                    if (rbA) rbA->velocity.y = 0;
                } 
                else if (isKinematicA && !isKinematicB) {
                    tB->position.y -= overlapY * sign;
                    if (rbB) rbB->velocity.y = 0;
                } 
                else {
                    tA->position.y += (overlapY / 2.0f) * sign;
                    tB->position.y -= (overlapY / 2.0f) * sign;
                    if (rbA) rbA->velocity.y = 0;
                    if (rbB) rbB->velocity.y = 0;
                }
            }
        }
    }
}
