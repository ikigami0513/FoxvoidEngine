#include "SpriteRenderer.hpp"
#include "../world/GameObject.hpp"
#include "../physics/Transform2d.hpp"
#include <iostream>

SpriteRenderer::SpriteRenderer(const std::string& texturePath) {
    // Load the image into GPU memory
    m_texture = LoadTexture(texturePath.c_str());
    m_transform = nullptr;
}

SpriteRenderer::~SpriteRenderer() {
    // Crucial: Free GPU memory when the component or GameObject is destroyed
    UnloadTexture(m_texture);
}

void SpriteRenderer::Start() {
    // Cache the Transform2d so we don't have to search for it every single frame
    m_transform = owner->GetComponent<Transform2d>();

    if (!m_transform) {
        std::cerr << "[SpriteRenderer] Warning: No Transform2d found on GameObject!" << std::endl;
    }
}

void SpriteRenderer::Render() {
    if (m_transform) {
        // Draw the texture at the entity's position
        // Raylib's basic DrawTexture expects integer coordinates
        DrawTexture(
            m_texture, 
            static_cast<int>(m_transform->position.x), 
            static_cast<int>(m_transform->position.y), 
            WHITE // Tint color (WHITE means unmodified)
        );
    }
}
