#include "SpriteRenderer.hpp"
#include "Graphics.hpp"
#include "../world/GameObject.hpp"
#include "../physics/Transform2d.hpp"
#include <iostream>
#include <core/AssetManager.hpp>

SpriteRenderer::SpriteRenderer(const std::string& texturePath) {
    // Load the image into GPU memory
    m_transform = nullptr;
    m_texture.id = 0;

    if (!texturePath.empty()) {
        SetTexture(texturePath);
    }
}

SpriteRenderer::~SpriteRenderer() {}

void SpriteRenderer::SetTexture(const std::string& path) {
    // If a texture is already loaded, free its GPU memory first
    if (m_texture.id != 0) {
        UnloadTexture(m_texture);
    }

    m_texturePath = path;

    // Only try to load if the path isn't empty
    if (!m_texturePath.empty()) {
        m_texture = AssetManager::GetTexture(path);
    }
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

std::string SpriteRenderer::GetName() const {
    return "Sprite Renderer";
}

void SpriteRenderer::OnInspector() {
    // 1. Prepare a C-string buffer for ImGui text input
    char buffer[256];
    strncpy(buffer, m_texturePath.c_str(), sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';

    // 2. Draw the InputText. 
    // We use ImGuiInputTextFlags_EnterReturnsTrue so it only loads when the user presses Enter 
    // (otherwise it would try to load incomplete paths on every keystroke!)
    if (ImGui::InputText("Texture Path", buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
        std::string newPath(buffer);
        if (newPath != m_texturePath) {
            SetTexture(newPath); // This safely unloads the old and loads the new
        }
    }
    
    ImGui::TextDisabled("Press ENTER to load new texture");

    // 3. Show useful debug info
    if (m_texture.id != 0) {
        ImGui::Text("Resolution: %d x %d", m_texture.width, m_texture.height);
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "No texture loaded!");
    }
}

nlohmann::json SpriteRenderer::Serialize() const {
    return {
        {"type", "SpriteRenderer"},
        {"texturePath", m_texturePath}
    };
}

void SpriteRenderer::Deserialize(const nlohmann::json& j) {
    std::string path = j.value("texturePath", "");
    if (!path.empty()) {
        SetTexture(path);
    }
}
