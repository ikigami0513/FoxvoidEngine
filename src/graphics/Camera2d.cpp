#include "Camera2d.hpp"
#include "physics/Transform2d.hpp"
#include "world/GameObject.hpp"
#include <raymath.h>
#include "core/Engine.hpp"
#include "world/Scene.hpp"
#include "graphics/TileMap.hpp"

#ifndef STANDALONE_MODE
#include "editor/EditorUI.hpp"
#endif

// Default constructor: 1.0x zoom, no offset, and marked as the main camera by default
Camera2d::Camera2d() 
    : zoom(1.0f), 
      offset{0.0f, 0.0f}, 
      anchor(Camera2dAnchor::Center), 
      isMain(true),
      lerpFactor(0.0f), // Default is instant follow
      useWorldBounds(false),
      worldBounds{0.0f, 0.0f, 2000.0f, 2000.0f},
      m_currentTarget{0.0f, 0.0f},
      m_isFirstFrame(true) 
{}

void Camera2d::Update(float deltaTime) {
    if (!owner) return;
    auto transform = owner->GetComponent<Transform2d>();
    if (!transform) return;

    // Prevent the camera from sliding from the origin on the very first frame of the game
    if (m_isFirstFrame) {
        m_currentTarget = transform->position;
        m_isFirstFrame = false;
    } else {
        // Smooth interpolation towards the Transform's position
        if (lerpFactor > 0.0f) {
            m_currentTarget = Vector2Lerp(m_currentTarget, transform->position, lerpFactor * deltaTime);
        } else {
            m_currentTarget = transform->position; // Instant snap
        }
    }
}

std::string Camera2d::GetName() const {
    return "Camera 2D";
}

#ifndef STANDALONE_MODE
void Camera2d::OnInspector() {
    EditorUI::DragFloat("Zoom", &zoom, 0.01f, this, 0.1f, 10.0f);
    
    const char* anchorNames[] = { "Top Left", "Center" };
    int currentAnchor = static_cast<int>(anchor);
    
    if (ImGui::Combo("Anchor", &currentAnchor, anchorNames, 2)) {
        // Track Undo/Redo for the dropdown
        nlohmann::json initialState = Serialize();
        anchor = static_cast<Camera2dAnchor>(currentAnchor);
        CommandHistory::AddCommand(std::make_unique<ModifyComponentCommand>(this, initialState, Serialize()));
    }

    // Offset is useful if you want the character to be on the left of the screen 
    // rather than perfectly in the center (e.g., in a runner game).
    EditorUI::DragFloat2("Screen Offset", &offset.x, 1.0f, this);
    
    EditorUI::Checkbox("Is Main Camera", &isMain, this);

    ImGui::Separator();
    
    // Lerp settings
    EditorUI::DragFloat("Lerp Factor", &lerpFactor, 0.1f, this, 0.0f, 50.0f);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("0 = Instant Follow. 5 = Smooth. 20 = Snappy.");

    // World Bounds settings
    EditorUI::Checkbox("Use World Bounds", &useWorldBounds, this);
    if (useWorldBounds) {
        float bounds[4] = { worldBounds.x, worldBounds.y, worldBounds.width, worldBounds.height };
        
        if (EditorUI::DragFloat4("Bounds (X,Y,W,H)", bounds, 1.0f, this)) {
            worldBounds = { bounds[0], bounds[1], bounds[2], bounds[3] };
        }

        // Auto-calculate button
        if (ImGui::Button("Auto-Calculate from Scene", ImVec2(-1, 0))) {
            if (Engine::Get()) {
                Scene& scene = Engine::Get()->GetActiveScene();
                
                // Initialize with extreme values to find the actual min/max
                float minX = 999999.0f, minY = 999999.0f;
                float maxX = -999999.0f, maxY = -999999.0f;
                bool foundAny = false;

                for (const auto& go : scene.GetGameObjects()) {
                    auto transform = go->GetComponent<Transform2d>();
                    if (!transform) continue;

                    float objMinX = transform->position.x;
                    float objMinY = transform->position.y;
                    float objMaxX = transform->position.x;
                    float objMaxY = transform->position.y;

                    // If it's a TileMap, add the total grid size to the maximum limits
                    if (auto tm = go->GetComponent<TileMap>()) {
                        objMaxX += tm->gridWidth * tm->tileSize.x * transform->scale.x;
                        objMaxY += tm->gridHeight * tm->tileSize.y * transform->scale.y;
                    }

                    if (objMinX < minX) minX = objMinX;
                    if (objMinY < minY) minY = objMinY;
                    if (objMaxX > maxX) maxX = objMaxX;
                    if (objMaxY > maxY) maxY = objMaxY;

                    foundAny = true;
                }

                // If any objects were found, apply the new limits
                if (foundAny) {
                    nlohmann::json initialState = Serialize();
                    
                    // Define the global rectangle (X, Y, Width, Height)
                    worldBounds = { minX, minY, maxX - minX, maxY - minY };
                    
                    CommandHistory::AddCommand(std::make_unique<ModifyComponentCommand>(this, initialState, Serialize()));
                }
            }
        }
    }
}
#endif

nlohmann::json Camera2d::Serialize() const {
    return {
        { "type", "Camera2d" },
        { "zoom", zoom },
        { "offsetX", offset.x },
        { "offsetY", offset.y },
        { "isMain", isMain },
        { "lerpFactor", lerpFactor },
        { "useWorldBounds", useWorldBounds },
        { "boundsX", worldBounds.x },
        { "boundsY", worldBounds.y },
        { "boundsW", worldBounds.width },
        { "boundsH", worldBounds.height }
    };
}

void Camera2d::Deserialize(const nlohmann::json& j) {
    zoom = j.value("zoom", 1.0f);
    offset.x = j.value("offsetX", 0.0f);
    offset.y = j.value("offsetY", 0.0f);
    isMain = j.value("isMain", true);

    lerpFactor = j.value("lerpFactor", 0.0f);
    useWorldBounds = j.value("useWorldBounds", false);
    
    worldBounds.x = j.value("boundsX", 0.0f);
    worldBounds.y = j.value("boundsY", 0.0f);
    worldBounds.width = j.value("boundsW", 2000.0f);
    worldBounds.height = j.value("boundsH", 2000.0f);

    // Reset first frame flag on load
    m_isFirstFrame = true;
}

Camera2D Camera2d::GetCamera(float screenWidth, float screenHeight) const {
    Camera2D cam = { 0 };
    cam.zoom = zoom;

    // Calculate base anchor offset
    Vector2 baseOffset = { 0.0f, 0.0f };
    if (anchor == Camera2dAnchor::Center) {
        baseOffset.x = screenWidth / 2.0f;
        baseOffset.y = screenHeight / 2.0f;
    }

    cam.offset.x = baseOffset.x + offset.x;
    cam.offset.y = baseOffset.y + offset.y;

    // Set target to the smoothly interpolated position
    cam.target = m_currentTarget;

    // Apply rotation
    if (owner) {
        auto transform = owner->GetComponent<Transform2d>();
        if (transform) cam.rotation = transform->rotation;
    }

    // Clamping logic
    if (useWorldBounds) {
        // Find the absolute edges of the camera view in World Space
        float halfVisibleWidth = (baseOffset.x) / zoom;
        float halfVisibleHeight = (baseOffset.y) / zoom;
        float remainVisibleWidth = (screenWidth - baseOffset.x) / zoom;
        float remainVisibleHeight = (screenHeight - baseOffset.y) / zoom;

        // Calculate limits so the screen edges never exit the bounding box
        float minX = worldBounds.x + halfVisibleWidth;
        float maxX = worldBounds.x + worldBounds.width - remainVisibleWidth;
        
        float minY = worldBounds.y + halfVisibleHeight;
        float maxY = worldBounds.y + worldBounds.height - remainVisibleHeight;

        // Edge case: If the screen is actually bigger than the defined bounds, 
        // we prevent max from being smaller than min (which causes visual glitches)
        if (minX > maxX) maxX = minX;
        if (minY > maxY) maxY = minY;

        cam.target.x = Clamp(cam.target.x, minX, maxX);
        cam.target.y = Clamp(cam.target.y, minY, maxY);
    }

    return cam;
}
