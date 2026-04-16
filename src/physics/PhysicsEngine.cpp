#include "PhysicsEngine.hpp"
#include "physics/Transform2d.hpp"
#include "physics/RigidBody2d.hpp"
#include "physics/RectCollider.hpp"
#include "graphics/TileMap.hpp"
#include "world/Scene.hpp"
#include <raymath.h>
#include <cmath>
#include <limits>

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
            // Reset grounded state before collision checks
            rb->isGrounded = false;

            // Apply global gravity based on the object's personal scale
            rb->velocity.x += GlobalGravity.x * rb->gravityScale * deltaTime;
            rb->velocity.y += GlobalGravity.y * rb->gravityScale * deltaTime;

            // Apply the velocity to the actual position
            transform->position.x += rb->velocity.x * deltaTime;
            transform->position.y += rb->velocity.y * deltaTime;
        }
    }

    // Step 2: Collect all solid TileMap geometry
    std::vector<Rectangle> solidTiles;
    for (const auto& go : gameObjects) {
        if (auto tileMap = go->GetComponent<TileMap>()) {
            auto rects = tileMap->GetCollisionRects();
            solidTiles.insert(solidTiles.end(), rects.begin(), rects.end());
        }
    }

    // Event Queue to store collisions for this frame
    // We store who collided (GameObject) and what they hit (Collision2D)
    std::vector<std::pair<GameObject*, Collision2D>> collisionEvents;

    // Step 3: Collision resolution (O(N²) checks)
    for (size_t i = 0; i < gameObjects.size(); ++i) {
        auto objA = gameObjects[i].get();

        // Dynamic Objects vs Static TileMap
        // We only test objects that have a collider against the solid tiles
        for (const Rectangle& tileRect : solidTiles) {
            Vector2 normal = { 0.0f, 0.0f };
            if (ResolveTileCollision(gameObjects[i].get(), tileRect, normal)) {
                collisionEvents.push_back({ objA, Collision2D{ nullptr, normal } });
            }
        }

        // Object vs Object (O(N²) checks)
        for (size_t j = i + 1; j < gameObjects.size(); ++j) {
            auto objB = gameObjects[j].get();
            Vector2 normalA = { 0.0f, 0.0f };
            Vector2 normalB = { 0.0f, 0.0f };
            
            if (ResolveCollision(objA, objB, normalA, normalB)) {
                collisionEvents.push_back({ objA, Collision2D{ objB, normalA } });
                collisionEvents.push_back({ objB, Collision2D{ objA, normalB } });
            }
        }
    }

    // Step 4: Dispath events to scripts
    for (const auto& event : collisionEvents) {
        GameObject* entity = event.first;
        const Collision2D& colData = event.second;
        
        // Tells the GameObject it collided, so it warns its components
        entity->OnCollision(colData);
    }
}

bool PhysicsEngine::ResolveCollision(GameObject* objA, GameObject* objB, Vector2& outNormalA, Vector2& outNormalB) {
    auto colA = objA->GetComponent<RectCollider>();
    auto colB = objB->GetComponent<RectCollider>();

    // Both objects must have colliders to collide
    if (!colA || !colB) return false;

    auto tA = objA->GetComponent<Transform2d>();
    auto tB = objB->GetComponent<Transform2d>();
    if (!tA || !tB) return false;

    auto rbA = objA->GetComponent<RigidBody2d>();
    auto rbB = objB->GetComponent<RigidBody2d>();

    // Define the actual world-space rectangles (centered around the transform)
    float scaledWidthA = colA->size.x * tA->scale.x;
    float scaledHeightA = colA->size.y * tA->scale.y;
    Rectangle recA = {
        (tA->position.x + colA->offset.x) - (scaledWidthA / 2.0f),
        (tA->position.y + colA->offset.y) - (scaledHeightA / 2.0f),
        scaledWidthA,
        scaledHeightA
    };

    float scaledWidthB = colB->size.x * tB->scale.x;
    float scaledHeightB = colB->size.y * tB->scale.y;
    Rectangle recB = {
        (tB->position.x + colB->offset.x) - (scaledWidthB / 2.0f),
        (tB->position.y + colB->offset.y) - (scaledHeightB / 2.0f),
        scaledWidthB,
        scaledHeightB
    };

    // Check if they overlap using Raylib
    if (CheckCollisionRecs(recA, recB)) {
        // Handle Triggers: Detect overlap but do NOT apply physics push
        if (colA->isTrigger || colB->isTrigger) {
            outNormalA = { 0.0f, 0.0f };
            outNormalB = { 0.0f, 0.0f };
            return true;
        }

        auto rbA = objA->GetComponent<RigidBody2d>();
        auto rbB = objB->GetComponent<RigidBody2d>();

        bool isKinematicA = !rbA || rbA->isKinematic;
        bool isKinematicB = !rbB || rbB->isKinematic;

        if (isKinematicA && isKinematicB) return true;
        
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
                outNormalA = { sign, 0.0f };
                outNormalB = { -sign, 0.0f };

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
                outNormalA = { 0.0f, sign };
                outNormalB = { 0.0f, -sign };

                if (sign < 0.0f && rbA) rbA->isGrounded = true;
                if (sign > 0.0f && rbB) rbB->isGrounded = true;

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

            return true;
        }
    }

    return false;
}

bool PhysicsEngine::ResolveTileCollision(GameObject* obj, const Rectangle& tileRect, Vector2& outNormal) {
    auto col = obj->GetComponent<RectCollider>();
    auto t = obj->GetComponent<Transform2d>();
    auto rb = obj->GetComponent<RigidBody2d>();

    // We only resolve if the object has a physical presence
    if (!col || !t) return false;

    // Define the dynamic object's world-space rectangle
    float scaledWidth = col->size.x * t->scale.x;
    float scaledHeight = col->size.y * t->scale.y;
    Rectangle rec = {
        (t->position.x + col->offset.x) - (scaledWidth / 2.0f),
        (t->position.y + col->offset.y) - (scaledHeight / 2.0f),
        scaledWidth,
        scaledHeight
    };

    // Check overlap
    if (CheckCollisionRecs(rec, tileRect)) {
        // Triggers detect the tile, but aren't pushed by it
        if (col->isTrigger) {
            outNormal = { 0.0f, 0.0f };
            return true; 
        }

        if (!rb || rb->isKinematic) return false;

        float center_x = rec.x + rec.width / 2.0f;
        float center_y = rec.y + rec.height / 2.0f;
        float tileCenter_x = tileRect.x + tileRect.width / 2.0f;
        float tileCenter_y = tileRect.y + tileRect.height / 2.0f;

        float overlapX = (rec.width / 2.0f + tileRect.width / 2.0f) - std::abs(center_x - tileCenter_x);
        float overlapY = (rec.height / 2.0f + tileRect.height / 2.0f) - std::abs(center_y - tileCenter_y);

        if (overlapX > 0 && overlapY > 0) {
            if (overlapX < overlapY) {
                // Horizontal collision (Hit a wall)
                float sign = (center_x < tileCenter_x) ? -1.0f : 1.0f;
                t->position.x += overlapX * sign;
                rb->velocity.x = 0;
                outNormal = { sign, 0.0f };
            } else {
                // Vertical collision (Hit the floor or ceiling)
                float sign = (center_y < tileCenter_y) ? -1.0f : 1.0f;
                t->position.y += overlapY * sign;
                rb->velocity.y = 0;
                outNormal = { 0.0f, sign };

                if (sign < 0.0f) {
                    rb->isGrounded = true;
                }
            }
            return true;
        }
    }
    
    return false;
}

void PhysicsEngine::RenderDebug(Scene& scene) {
    const auto& gameObjects = scene.GetGameObjects();

    for (const auto& go : gameObjects) {
        auto col = go->GetComponent<RectCollider>();
        auto transform = go->GetComponent<Transform2d>();

        if (col && transform) {
            // Calculate the exact AABB centered around the transform position
            float scaledWidth = col->size.x * transform->scale.x;
            float scaledHeight = col->size.y * transform->scale.y;

            // Shift the top-left corner back by half the width and height
            Rectangle rec = {
                (transform->position.x + col->offset.x) - (scaledWidth / 2.0f),
                (transform->position.y + col->offset.y) - (scaledHeight / 2.0f),
                scaledWidth,
                scaledHeight
            };

            // Choose color: Yellow for triggers, Green for solid physical colliders
            Color debugColor = col->isTrigger ? YELLOW : GREEN;

            // Draw the outline of the collider
            DrawRectangleLinesEx(rec, 2.0f, debugColor);

            // Draw a tiny crosshair at the actual position of the GameObject 
            // This is super helpful to visualize how the 'offset' is moving the collider
            DrawLine(transform->position.x - 5, transform->position.y, transform->position.x + 5, transform->position.y, RED);
            DrawLine(transform->position.x, transform->position.y - 5, transform->position.x, transform->position.y + 5, RED);
        }

        // Draw TileMap Solid Layers Geometry
        if (auto tileMap = go->GetComponent<TileMap>()) {
            std::vector<Rectangle> solidTiles = tileMap->GetCollisionRects();
            for (const auto& tileRect : solidTiles) {
                // Use a distinctive color (e.g. SKYBLUE) for level geometry
                DrawRectangleLinesEx(tileRect, 1.5f, SKYBLUE);
            }
        }
    }
}

// A simple implementation of Line-vs-AABB using Raylib maths
bool PhysicsEngine::CheckLineBoxIntersection(Vector2 p1, Vector2 p2, Rectangle box, Vector2& hitPoint, Vector2& hitNormal) {
    // We use Raylib's internal CheckCollisionLines to test the ray against the 4 edges of the box.
    // We keep the intersection that is closest to p1.
    
    Vector2 boxCorners[4] = {
        { box.x, box.y },                           // Top-Left
        { box.x + box.width, box.y },               // Top-Right
        { box.x + box.width, box.y + box.height },  // Bottom-Right
        { box.x, box.y + box.height }               // Bottom-Left
    };
    
    Vector2 boxNormals[4] = {
        { 0, -1 }, // Top edge normal
        { 1, 0 },  // Right edge normal
        { 0, 1 },  // Bottom edge normal
        { -1, 0 }  // Left edge normal
    };

    bool hitSomething = false;
    float closestDist = std::numeric_limits<float>::infinity();

    for (int i = 0; i < 4; i++) {
        Vector2 edgeStart = boxCorners[i];
        Vector2 edgeEnd = boxCorners[(i + 1) % 4];
        Vector2 currentHitPoint;

        // CheckCollisionLines returns true if the two line segments cross
        if (CheckCollisionLines(p1, p2, edgeStart, edgeEnd, &currentHitPoint)) {
            float dist = Vector2Distance(p1, currentHitPoint);
            if (dist < closestDist) {
                closestDist = dist;
                hitPoint = currentHitPoint;
                hitNormal = boxNormals[i];
                hitSomething = true;
            }
        }
    }
    return hitSomething;
}

RaycastHit PhysicsEngine::Raycast(Scene& scene, Vector2 origin, Vector2 direction, float maxDistance) {
    RaycastHit result = { false, nullptr, {0,0}, {0,0}, maxDistance };
    
    // Normalize direction to be safe
    direction = Vector2Normalize(direction);
    Vector2 destination = { origin.x + direction.x * maxDistance, origin.y + direction.y * maxDistance };

    float closestHitDist = maxDistance;

    // Check GameObjects (RectColliders)
    for (const auto& go : scene.GetGameObjects()) {
        auto col = go->GetComponent<RectCollider>();
        auto t = go->GetComponent<Transform2d>();
        
        // Skip triggers if you want, or add a parameter to the function to include them!
        if (!col || !t || col->isTrigger) continue;

        float scaledWidth = col->size.x * t->scale.x;
        float scaledHeight = col->size.y * t->scale.y;
        Rectangle rec = {
            (t->position.x + col->offset.x) - (scaledWidth / 2.0f),
            (t->position.y + col->offset.y) - (scaledHeight / 2.0f),
            scaledWidth,
            scaledHeight
        };

        Vector2 hitPoint, hitNormal;
        if (CheckLineBoxIntersection(origin, destination, rec, hitPoint, hitNormal)) {
            float dist = Vector2Distance(origin, hitPoint);
            if (dist < closestHitDist) {
                closestHitDist = dist;
                result.hit = true;
                result.collider = go.get();
                result.point = hitPoint;
                result.normal = hitNormal;
                result.distance = dist;
            }
        }
    }

    // Check TileMaps
    for (const auto& go : scene.GetGameObjects()) {
        if (auto tileMap = go->GetComponent<TileMap>()) {
            
            // Get all solid blocks from this specific TileMap
            std::vector<Rectangle> solidTiles = tileMap->GetCollisionRects();
            
            for (const Rectangle& tileRect : solidTiles) {
                Vector2 hitPoint, hitNormal;
                
                // Test the ray against this specific tile
                if (CheckLineBoxIntersection(origin, destination, tileRect, hitPoint, hitNormal)) {
                    float dist = Vector2Distance(origin, hitPoint);
                    
                    // Same logic: keep it if it's the closest thing hit so far
                    if (dist < closestHitDist) {
                        closestHitDist = dist;
                        result.hit = true;
                        
                        // We associate the hit with the GameObject that owns the TileMap
                        result.collider = go.get(); 
                        result.point = hitPoint;
                        result.normal = hitNormal;
                        result.distance = dist;
                    }
                }
            }
        }
    }

    return result;
}
