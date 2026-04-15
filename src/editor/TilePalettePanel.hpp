#pragma once

#include <raylib.h>
#include <string>

class TilePalettePanel {
    public:
        TilePalettePanel();

        // Draws the Tile Palette window
        // selectedTileID: Reference to the globally selected tile ID in the editor
        // currentTileset: The texture currently used by the selected TileMap
        // tileSize: The size of tiles (usually from the TileMap component)
        void Draw(int& selectedTileID, Texture2D currentTileset, Vector2 tileSize, int tileSpacing);

        bool isOpen = true;

    private:
        float m_zoom = 1.0f;
};
