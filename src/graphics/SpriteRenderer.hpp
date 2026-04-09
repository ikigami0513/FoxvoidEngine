#pragma once

#include "../world/Component.hpp"
#include <string>
#include <raylib.h>

class Transform2d; // Forward declaration to avoid circular includes

class SpriteRenderer : public Component {
    public:
        // Loads the texture from the given file path
        SpriteRenderer(const std::string& texturePath);

        // Virtual destructor to ensure the texture is unloaded safely
        ~SpriteRenderer() override;

        void Start() override;
        void Render() override;

    private:
        // Raylib's native texture structure
        Texture2D m_texture;

        // Cached pointer to the transform for fast position reading every frame
        Transform2d* m_transform;
};
