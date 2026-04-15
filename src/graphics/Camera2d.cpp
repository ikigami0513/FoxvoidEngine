#include "Camera2d.hpp"
#include "editor/EditorUI.hpp"
#include "physics/Transform2d.hpp"
#include "world/GameObject.hpp"

// Default constructor: 1.0x zoom, no offset, and marked as the main camera by default
Camera2d::Camera2d() : zoom(1.0f), offset{0.0f, 0.0f}, anchor(Camera2dAnchor::Center), isMain(true) {}

std::string Camera2d::GetName() const {
    return "Camera 2D";
}

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
}

nlohmann::json Camera2d::Serialize() const {
    return {
        { "type", "Camera2d" },
        { "zoom", zoom },
        { "offsetX", offset.x },
        { "offsetY", offset.y },
        { "isMain", isMain }
    };
}

void Camera2d::Deserialize(const nlohmann::json& j) {
    zoom = j.value("zoom", 1.0f);
    offset.x = j.value("offsetX", 0.0f);
    offset.y = j.value("offsetY", 0.0f);
    isMain = j.value("isMain", true);
}

Camera2D Camera2d::GetCamera(float screenWidth, float screenHeight) const {
    Camera2D cam = { 0 };
    cam.zoom = zoom;

    // Calculate the base offset based on the chosen anchor
    Vector2 baseOffset = { 0.0f, 0.0f };
    if (anchor == Camera2dAnchor::Center) {
        baseOffset.x = screenWidth / 2.0f;
        baseOffset.y = screenHeight / 2.0f;
    }

    // Add the user's custom offset on top of the anchor
    cam.offset.x = baseOffset.x + offset.x;
    cam.offset.y = baseOffset.y + offset.y;

    // Safety check: Ensure the component is attached to a GameObject
    if (owner) {
        auto transform = owner->GetComponent<Transform2d>();
        if (transform) {
            // The camera "looks at" the position of the GameObject it is attached to
            cam.target = transform->position;
            cam.rotation = transform->rotation;
        }
    }

    return cam;
}
