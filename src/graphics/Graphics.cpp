#include "Graphics.hpp"

// We default to false.
bool Graphics::pixelArtMode = false; 

Texture2D Graphics::LoadTextureFiltered(const std::string& path) {
    // Load the raw texture from disk to GPU
    Texture2D texture = LoadTexture(path.c_str());

    // Prevent Texture Edge Bleeding
    // Forces the GPU to stop sampling pixels outside the actual image dimensions.
    // This removes the dark/transparent artifacts on the edges of scaled sprites.
    SetTextureWrap(texture, TEXTURE_WRAP_CLAMP);

    // Apply the correct filter based on the global engine setting
    if (pixelArtMode) {
        // Nearest-neighbor: Keeps pixels sharp and blocky (Perfect for Pixel Art)
        SetTextureFilter(texture, TEXTURE_FILTER_POINT);
    } else {
        // Bilinear: Smooths out the pixels when scaled
        SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
    }

    return texture;
}
