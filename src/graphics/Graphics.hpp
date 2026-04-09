#pragma once

#include <raylib.h>
#include <string>

class Graphics {
    public:
        // Global setting: True for crisp pixels (Nearest), False for smooth scaling (Bilinear)
        static bool pixelArtMode;

        // A smart wrapper around Raylib's LoadTexture that automatically applies the global filter
        static Texture2D LoadTextureFiltered(const std::string& path);
};
