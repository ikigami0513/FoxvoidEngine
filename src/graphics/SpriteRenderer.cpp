#include "SpriteRenderer.hpp"
#include "Graphics.hpp"
#include "../world/GameObject.hpp"
#include "../physics/Transform2d.hpp"
#include <iostream>

SpriteRenderer::SpriteRenderer(const std::string& texturePath) {
    // Load the image into GPU memory
    m_texture = Graphics::LoadTextureFiltered(texturePath);
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
        // Source Rectangle: The entire texture
        Rectangle sourceRec = { 0.0f, 0.0f, (float)m_texture.width, (float)m_texture.height };
    
        // Destination Rectangle: Position and scaled size
        Rectangle destRec = {
            m_transform->position.x,
            m_transform->position.y,
            m_texture.width * m_transform->scale.x,
            m_texture.height * m_transform->scale.y
        };

        // Origin: Center of the scaled texture for correct rotation
        Vector2 origin = { destRec.width / 2.0f, destRec.height / 2.0f };

        // 4. Draw with full transform support
        DrawTexturePro(m_texture, sourceRec, destRec, origin, m_transform->rotation, WHITE);
    }
}
