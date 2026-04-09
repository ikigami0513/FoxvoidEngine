#include "AssetManager.hpp"
#include "../graphics/Graphics.hpp"
#include <iostream>

std::unordered_map<std::string, Texture2D> AssetManager::s_textures;

Texture2D AssetManager::GetTexture(const std::string& filepath) {
    auto it = s_textures.find(filepath);

    if (it != s_textures.end()) {
        // Texture found, return the cached version
        return it->second;
    }

    // Texture not found, load it from the disk
    Texture2D texture = Graphics::LoadTextureFiltered(filepath);

    // Validate that the texture loaded correctly (Raylib sets id to 0 on failure)
    if (texture.id == 0) {
        std::cerr << "[AssetManager] Failed to load texture: " << filepath << std::endl;
    } else {
        std::cout << "[AssetManager] Loaded new texture into VRAM: " << filepath << std::endl;
        // Store it in the cache for future requests
        s_textures[filepath] = texture;
    }

    return texture;
}

void AssetManager::Clear() {
    // Safely unload all stored textures from the GPU
    for (auto& pair : s_textures) {
        UnloadTexture(pair.second);
    }

    // Clear the map to reset the state
    s_textures.clear();
    std::cout << "[AssetManager] Cleared all textures from VRAM." << std::endl;
}
