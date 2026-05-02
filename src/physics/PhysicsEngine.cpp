#include "PhysicsEngine.hpp"
#include "physics/Transform2d.hpp"
#include "physics/RigidBody2d.hpp"
#include "physics/RectCollider.hpp"
#include "graphics/TileMap.hpp"
#include "world/Scene.hpp"
#include <raymath.h>
#include <cmath>
#include <limits>
#include "PolygonCollider.hpp"

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

            // Apply the velocity to the actual position in GLOBAL SPACE
            Vector2 globalPos = transform->GetGlobalPosition();
            globalPos.x += rb->velocity.x * deltaTime;
            globalPos.y += rb->velocity.y * deltaTime;
            transform->SetGlobalPosition(globalPos);
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

    // Step 4: Dispatch events to scripts
    for (const auto& event : collisionEvents) {
        GameObject* entity = event.first;
        const Collision2D& colData = event.second;
        
        // Tells the GameObject it collided, so it warns its components
        entity->OnCollision(colData);
    }
}

bool PhysicsEngine::ResolveCollision(GameObject* objA, GameObject* objB, Vector2& outNormalA, Vector2& outNormalB) {
    std::vector<Vector2> polyA, polyB;
    bool isTriggerA = false, isTriggerB = false;

    // Fetch collider data universally (works for Rect AND Polygon colliders)
    if (!GetObjectColliderData(objA, polyA, isTriggerA)) return false;
    if (!GetObjectColliderData(objB, polyB, isTriggerB)) return false;
    
    Vector2 mtv; // Minimum Translation Vector
    if (SATCollision(polyA, polyB, mtv)) {
        if (isTriggerA || isTriggerB) {
            outNormalA = { 0.0f, 0.0f };
            outNormalB = { 0.0f, 0.0f };
            return true;
        }

        auto tA = objA->GetComponent<Transform2d>();
        auto tB = objB->GetComponent<Transform2d>();
        auto rbA = objA->GetComponent<RigidBody2d>();
        auto rbB = objB->GetComponent<RigidBody2d>();

        bool isKinematicA = !rbA || rbA->isKinematic;
        bool isKinematicB = !rbB || rbB->isKinematic;

        if (isKinematicA && isKinematicB) return true;

        // The collision normal is simply the direction of the MTV
        Vector2 normal = Vector2Normalize(mtv);
        outNormalA = {-normal.x, -normal.y};
        outNormalB = normal;

        // If the normal pushes upwards (negative Y in Raylib), we are touching the ground
        if (normal.y < -0.5f && rbA) rbA->isGrounded = true;
        if (normal.y > 0.5f && rbB) rbB->isGrounded = true;

        Vector2 posA = tA->GetGlobalPosition();
        Vector2 posB = tB->GetGlobalPosition();

        if (!isKinematicA && isKinematicB) {
            tA->SetGlobalPosition(Vector2Subtract(posA, mtv));
            // For now, we kill all velocity on impact. 
            // Later, we can do a proper projection of the velocity vector along the wall!
            if (rbA) { rbA->velocity.x = 0; rbA->velocity.y = 0; }
        } 
        else if (isKinematicA && !isKinematicB) {
            tB->SetGlobalPosition(Vector2Add(posB, mtv));
            if (rbB) { rbB->velocity.x = 0; rbB->velocity.y = 0; }
        } 
        else {
            // Split mass: push both objects away from each other equally
            tA->SetGlobalPosition(Vector2Subtract(posA, {mtv.x * 0.5f, mtv.y * 0.5f}));
            tB->SetGlobalPosition(Vector2Add(posB, {mtv.x * 0.5f, mtv.y * 0.5f}));
            if (rbA) { rbA->velocity.x = 0; rbA->velocity.y = 0; }
            if (rbB) { rbB->velocity.x = 0; rbB->velocity.y = 0; }
        }

        return true;
    }
    return false;
}

bool PhysicsEngine::ResolveTileCollision(GameObject* obj, const Rectangle& tileRect, Vector2& outNormal) {
    std::vector<Vector2> polyA;
    bool isTrigger = false;

    // Abstract fetch
    if (!GetObjectColliderData(obj, polyA, isTrigger)) return false;
    std::vector<Vector2> polyTile = GetRectangleVertices(tileRect);
    
    Vector2 mtv;
    if (SATCollision(polyA, polyTile, mtv)) {
        if (isTrigger) {
            outNormal = {0.0f, 0.0f};
            return true;
        }

        auto t = obj->GetComponent<Transform2d>();
        auto rb = obj->GetComponent<RigidBody2d>();

        if (!rb || rb->isKinematic) return false;

        Vector2 normal = Vector2Normalize(mtv);
        outNormal = {-normal.x, -normal.y};

        if (normal.y < -0.5f) rb->isGrounded = true;

        Vector2 pos = t->GetGlobalPosition();
        t->SetGlobalPosition(Vector2Subtract(pos, mtv));
        
        // Basic velocity cancellation depending on the hit axis
        if (std::abs(normal.x) > 0.5f) rb->velocity.x = 0;
        if (std::abs(normal.y) > 0.5f) rb->velocity.y = 0;

        return true;
    }
    return false;
}

void PhysicsEngine::RenderDebug(Scene& scene) {
    const auto& gameObjects = scene.GetGameObjects();

    for (const auto& go : gameObjects) {
        std::vector<Vector2> vertices;
        bool isTrigger = false;
        
        // Abstract drawing: It will draw ANY collider shape perfectly
        if (GetObjectColliderData(go.get(), vertices, isTrigger)) {
            Color debugColor = isTrigger ? YELLOW : GREEN;
            
            // Loop through all vertices and draw lines between them
            for (size_t i = 0; i < vertices.size(); i++) {
                DrawLineEx(vertices[i], vertices[(i + 1) % vertices.size()], 2.0f, debugColor);
            }

            if (auto t = go->GetComponent<Transform2d>()) {
                Vector2 pos = t->GetGlobalPosition();
                DrawLine(pos.x - 5, pos.y, pos.x + 5, pos.y, RED);
                DrawLine(pos.x, pos.y - 5, pos.x, pos.y + 5, RED);
            }
        }

        // Draw TileMap Solid Layers Geometry
        if (auto tileMap = go->GetComponent<TileMap>()) {
            std::vector<Rectangle> solidTiles = tileMap->GetCollisionRects();
            for (const auto& tileRect : solidTiles) {
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

        Vector2 pos = t->GetGlobalPosition();
        Vector2 scale = t->GetGlobalScale();

        float scaledWidth = col->size.x * std::abs(scale.x);
        float scaledHeight = col->size.y * std::abs(scale.y);
        Rectangle rec = {
            (pos.x + col->offset.x) - (scaledWidth / 2.0f),
            (pos.y + col->offset.y) - (scaledHeight / 2.0f),
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

std::vector<Vector2> PhysicsEngine::GetColliderVertices(RectCollider* col, Transform2d* t) {
    std::vector<Vector2> vertices(4);
    
    Vector2 pos = t->GetGlobalPosition();
    Vector2 scale = t->GetGlobalScale();
    float rot = t->GetGlobalRotation() * DEG2RAD; // Raylib DEG2RAD macro converts degrees to radians
    
    float cosR = std::cos(rot);
    float sinR = std::sin(rot);
    
    float sw = col->size.x * std::abs(scale.x);
    float sh = col->size.y * std::abs(scale.y);
    float ox = col->offset.x * std::abs(scale.x);
    float oy = col->offset.y * std::abs(scale.y);

    // The 4 local corners centered around the origin (0,0)
    Vector2 localPoints[4] = {
        {-sw / 2.0f + ox, -sh / 2.0f + oy}, // Top-Left
        { sw / 2.0f + ox, -sh / 2.0f + oy}, // Top-Right
        { sw / 2.0f + ox,  sh / 2.0f + oy}, // Bottom-Right
        {-sw / 2.0f + ox,  sh / 2.0f + oy}  // Bottom-Left
    };

    // Apply the rotation matrix and translate to the global position
    for(int i = 0; i < 4; i++) {
        vertices[i].x = pos.x + (localPoints[i].x * cosR - localPoints[i].y * sinR);
        vertices[i].y = pos.y + (localPoints[i].x * sinR + localPoints[i].y * cosR);
    }
    return vertices;
}

bool PhysicsEngine::GetObjectColliderData(GameObject* obj, std::vector<Vector2>& outVertices, bool& outIsTrigger) {
    auto t = obj->GetComponent<Transform2d>();
    if (!t) return false;

    // 1. Check if the object has a standard RectCollider
    if (auto rectCol = obj->GetComponent<RectCollider>()) {
        outVertices = GetColliderVertices(rectCol, t); // Reuse your existing logic!
        outIsTrigger = rectCol->isTrigger;
        return true;
    }

    // 2. Check if the object has a custom PolygonCollider
    if (auto polyCol = obj->GetComponent<PolygonCollider>()) {
        outVertices.clear();
        outVertices.reserve(polyCol->localVertices.size());
        
        Vector2 pos = t->GetGlobalPosition();
        Vector2 scale = t->GetGlobalScale();
        float rot = t->GetGlobalRotation() * DEG2RAD;
        
        float cosR = std::cos(rot);
        float sinR = std::sin(rot);
        
        // Apply Transform mathematics to every single vertex in the polygon
        for (const auto& localV : polyCol->localVertices) {
            // Apply scale
            float sx = (localV.x + polyCol->offset.x) * std::abs(scale.x);
            float sy = (localV.y + polyCol->offset.y) * std::abs(scale.y);
            
            // Apply rotation and translation to move to global space
            float gx = pos.x + (sx * cosR - sy * sinR);
            float gy = pos.y + (sx * sinR + sy * cosR);
            
            outVertices.push_back({gx, gy});
        }
        
        outIsTrigger = polyCol->isTrigger;
        return true;
    }

    return false; // No collider found
}

std::vector<Vector2> PhysicsEngine::GetRectangleVertices(const Rectangle& rect) {
    return {
        { rect.x, rect.y },
        { rect.x + rect.width, rect.y },
        { rect.x + rect.width, rect.y + rect.height },
        { rect.x, rect.y + rect.height }
    };
}

Vector2 PhysicsEngine::GetPolygonCenter(const std::vector<Vector2>& vertices) {
    Vector2 center = {0, 0};
    for (const auto& v : vertices) {
        center.x += v.x;
        center.y += v.y;
    }
    center.x /= vertices.size();
    center.y /= vertices.size();
    return center;
}

void PhysicsEngine::ProjectPolygon(const std::vector<Vector2>& vertices, Vector2 axis, float& min, float& max) {
    min = Vector2DotProduct(vertices[0], axis);
    max = min;
    for (size_t i = 1; i < vertices.size(); i++) {
        float projection = Vector2DotProduct(vertices[i], axis);
        if (projection < min) min = projection;
        if (projection > max) max = projection;
    }
}

bool PhysicsEngine::SATCollision(const std::vector<Vector2>& polyA, const std::vector<Vector2>& polyB, Vector2& outMTV) {
    float minOverlap = std::numeric_limits<float>::infinity();
    Vector2 smallestAxis = {0, 0};

    std::vector<std::vector<Vector2>> polygons = {polyA, polyB};

    // For each polygon...
    for (int p = 0; p < 2; p++) {
        const auto& poly = polygons[p];
        
        // ... test the normal of each edge
        for (size_t i = 0; i < poly.size(); i++) {
            Vector2 p1 = poly[i];
            Vector2 p2 = poly[(i + 1) % poly.size()];
            
            // Edge vector
            Vector2 edge = Vector2Subtract(p2, p1);
            // The normal is the perpendicular vector to the edge (flipped x and y)
            Vector2 normal = Vector2Normalize({-edge.y, edge.x});

            float minA, maxA, minB, maxB;
            ProjectPolygon(polyA, normal, minA, maxA);
            ProjectPolygon(polyB, normal, minB, maxB);

            // If there is a gap on this axis, THEN THERE IS NO COLLISION AT ALL!
            if (minA >= maxB || minB >= maxA) {
                return false;
            }

            // Calculate the actual overlap depth
            float overlap = std::min(maxA, maxB) - std::max(minA, minB);
            
            // Keep track of the axis with the smallest overlap (Minimum Translation Vector)
            if (overlap < minOverlap) {
                minOverlap = overlap;
                smallestAxis = normal;
            }
        }
    }

    // Ensure the push vector always points from A to B
    Vector2 centerA = GetPolygonCenter(polyA);
    Vector2 centerB = GetPolygonCenter(polyB);
    Vector2 dir = Vector2Subtract(centerB, centerA);
    
    if (Vector2DotProduct(smallestAxis, dir) < 0) {
        smallestAxis = {-smallestAxis.x, -smallestAxis.y};
    }

    outMTV = {smallestAxis.x * minOverlap, smallestAxis.y * minOverlap};
    return true;
}
