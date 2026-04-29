#include "AssetManager.hpp"
#include "../graphics/Graphics.hpp"
#include <iostream>

std::unordered_map<std::string, Texture2D> AssetManager::s_textures;
std::unordered_map<std::string, Font> AssetManager::s_fonts;

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

Font AssetManager::GetFont(const std::string& filepath) {
    // 1. Check if the font is already loaded in memory
    auto it = s_fonts.find(filepath);
    if (it != s_fonts.end()) {
        return it->second;
    }

    // 2. Load the font at a high resolution (128) for crisp scaling
    Font font = LoadFontEx(filepath.c_str(), 128, 0, 250);
    
    // 3. Verify it loaded correctly (Raylib returns texture ID 0 on failure)
    if (font.texture.id != 0) {
        // Use Bilinear filtering for smooth modern fonts
        SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR); 
        
        // Cache it for future use
        s_fonts[filepath] = font;
        std::cout << "[AssetManager] Loaded Font: " << filepath << std::endl;
        return font;
    }

    // Fallback if loading failed
    std::cerr << "[AssetManager] Error: Failed to load font at " << filepath << std::endl;
    return GetFontDefault();
}

void AssetManager::Clear() {
    // Safely unload all stored textures from the GPU
    for (auto& pair : s_textures) {
        UnloadTexture(pair.second);
    }
    s_textures.clear();

    // Unload all Fonts
    for (auto& pair : s_fonts) {
        UnloadFont(pair.second);
    }
    s_fonts.clear();

    std::cout << "[AssetManager] Cleared all textures from VRAM." << std::endl;
}
