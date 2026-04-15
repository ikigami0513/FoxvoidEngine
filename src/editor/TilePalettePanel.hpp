#pragma once

#include <raylib.h>
#include <string>

class TilePalettePanel {
    public:
        TilePalettePanel();

        // Draws the Tile Palette window
        void Draw(int& selectedTileID, int& selectedLayer, class TileMap* activeTileMap);

        bool isOpen = true;

    private:
        float m_zoom = 1.0f;
};
