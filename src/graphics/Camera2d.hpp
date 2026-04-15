#pragma once

#include "world/Component.hpp"
#include <raylib.h>
#include <nlohmann/json.hpp>

// Define the possible anchor points for the camera
enum class Camera2dAnchor {
    TopLeft,
    Center
};

class Camera2d : public Component {
    public:
        float zoom;
        Vector2 offset; // Custom shift relative to the anchor
        Camera2dAnchor anchor; // The base screen position
        bool isMain; // Determines if this is the primary camera used to render the Game View

        Camera2d();

        std::string GetName() const override;
        void OnInspector() override;

        nlohmann::json Serialize() const override;
        void Deserialize(const nlohmann::json& j) override;

        // Generates the Raylib Camera2D struct dynamically based on the GameObject's Transform2d
        Camera2D GetCamera(float screenWidth, float screenHeight) const;
};
