#include "SpriteSheetRenderer.hpp"
#include "../world/GameObject.hpp"
#include "../physics/Transform2d.hpp"
#include "Graphics.hpp"
#include <iostream>

SpriteSheetRenderer::SpriteSheetRenderer(const std::string& texturePath, int columns, int rows)
    : m_columns(columns), m_rows(rows), m_currentFrame(0), m_transform(nullptr) 
{
    m_texture = Graphics::LoadTextureFiltered(texturePath);
}

SpriteSheetRenderer::~SpriteSheetRenderer() {
    // Free GPU memory
    UnloadTexture(m_texture);
}

void SpriteSheetRenderer::Start() {
    m_transform = owner->GetComponent<Transform2d>();
    if (!m_transform) {
        std::cerr << "[SpriteSheetRenderer] Warning: No Transform2d found!" << std::endl;
    }
}

void SpriteSheetRenderer::SetFrame(int frameIndex) {
    // Safety check to prevent drawing out of bounds
    int maxFrames = m_columns * m_rows;
    if (frameIndex >= 0 && frameIndex < maxFrames) {
        m_currentFrame = frameIndex;
    }
}

Rectangle SpriteSheetRenderer::GetSourceRec() const {
    // Calculate the physical pixel size of a single frame
    float frameWidth = static_cast<float>(m_texture.width) / m_columns;
    float frameHeight = static_cast<float>(m_texture.height) / m_rows;

    // Math magic: convert a 1D index into 2D grid coordinates
    int gridX = m_currentFrame % m_columns;
    int gridY = m_currentFrame / m_columns;

    // Return the subset rectangle of the texture to draw
    return Rectangle{
        gridX * frameWidth,
        gridY * frameHeight,
        frameWidth,
        frameHeight
    };
}

void SpriteSheetRenderer::Render() {
    if (m_transform) {
        // Source Rectangle: The specific frame from the spritesheet
        Rectangle sourceRec = GetSourceRec();

        // Destination Rectangle: Position and scaled size of that single frame
        Rectangle destRec = {
            m_transform->position.x,
            m_transform->position.y,
            sourceRec.width * m_transform->scale.x,
            sourceRec.height * m_transform->scale.y
        };

        // Origin: Center of the scaled frame
        Vector2 origin = { destRec.width / 2.0f, destRec.height / 2.0f };

        // Draw with full transform support
        DrawTexturePro(m_texture, sourceRec, destRec, origin, m_transform->rotation, WHITE);
    }
}
