#pragma once

#include "../world/Component.hpp"
#include <string>
#include <raylib.h>

class Transform2d; // Forward declaration to avoid circular includes

class SpriteRenderer : public Component {
    public:
        // Loads the texture from the given file path
        SpriteRenderer(const std::string& texturePath = "");

        // Virtual destructor to ensure the texture is unloaded safely
        ~SpriteRenderer() override;

        void Start() override;
        void Render() override;

        // A helper method to safely change the texture during runtime or editor mode
        void SetTexture(const std::string& path);

        Texture2D GetTexture() const { return m_texture; }

        // Editor UI and Serialization
        std::string GetName() const override;

#ifndef STANDALONE_MODE
        void OnInspector() override;
#endif

        nlohmann::json Serialize() const override;
        void Deserialize(const nlohmann::json& j) override;

    private:
        std::string m_texturePath;

        // Raylib's native texture structure
        Texture2D m_texture;

        // Cached pointer to the transform for fast position reading every frame
        Transform2d* m_transform;
};
