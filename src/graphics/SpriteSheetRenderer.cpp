#include "SpriteSheetRenderer.hpp"
#include "../world/GameObject.hpp"
#include "../physics/Transform2d.hpp"
#include "Graphics.hpp"
#include <iostream>
#include <cstring>
#include <core/AssetManager.hpp>

SpriteSheetRenderer::SpriteSheetRenderer(const std::string& texturePath, int columns, int rows)
    : m_columns(columns), m_rows(rows), m_currentFrame(0), m_transform(nullptr) 
{
    m_texture.id = 0;

    if (!texturePath.empty()) {
        SetTexture(texturePath);
    }
}

SpriteSheetRenderer::~SpriteSheetRenderer() {}

void SpriteSheetRenderer::SetTexture(const std::string& path) {
    if (m_texture.id != 0) {
        UnloadTexture(m_texture);
    }
    
    m_texturePath = path;
    
    if (!m_texturePath.empty()) {
        m_texture = AssetManager::GetTexture(path);
    }
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
    // Prevent division by zero just in case
    int safeCols = (m_columns > 0) ? m_columns : 1;
    int safeRows = (m_rows > 0) ? m_rows : 1;

    // Calculate the physical pixel size of a single frame
    float frameWidth = static_cast<float>(m_texture.width) / safeCols;
    float frameHeight = static_cast<float>(m_texture.height) / safeRows;

    // Math magic: convert a 1D index into 2D grid coordinates
    int gridX = m_currentFrame % safeCols;
    int gridY = m_currentFrame / safeCols;

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

std::string SpriteSheetRenderer::GetName() const {
    return "SpriteSheet Renderer";
}

void SpriteSheetRenderer::OnInspector() {
    // 1. Texture Path Input
    char buffer[256];
    strncpy(buffer, m_texturePath.c_str(), sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';

    if (ImGui::InputText("Texture Path", buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
        std::string newPath(buffer);
        if (newPath != m_texturePath) {
            SetTexture(newPath);
        }
    }

    // 2. Grid Dimensions (Columns and Rows)
    if (ImGui::DragInt("Columns", &m_columns, 0.1f, 1, 64)) {
        // Reset frame if grid size changes to avoid out-of-bounds rendering
        m_currentFrame = 0; 
    }
    if (ImGui::DragInt("Rows", &m_rows, 0.1f, 1, 64)) {
        m_currentFrame = 0;
    }

    // 3. Debug frame viewer (Read Only in Editor)
    ImGui::Text("Current Frame: %d / %d", m_currentFrame, (m_columns * m_rows) - 1);
}

nlohmann::json SpriteSheetRenderer::Serialize() const {
    return {
        {"type", "SpriteSheetRenderer"},
        {"texturePath", m_texturePath},
        {"columns", m_columns},
        {"rows", m_rows}
    };
}

void SpriteSheetRenderer::Deserialize(const nlohmann::json& j) {
    m_columns = j.value("columns", 1);
    m_rows = j.value("rows", 1);
    m_currentFrame = 0; // Always reset frame on load

    std::string path = j.value("texturePath", "");
    if (!path.empty()) {
        SetTexture(path);
    }
}
