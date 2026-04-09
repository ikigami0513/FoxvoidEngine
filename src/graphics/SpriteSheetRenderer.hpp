#pragma once

#include "../world/Component.hpp"
#include <string>
#include <raylib.h>

class Transform2d; // Forward declaration

class SpriteSheetRenderer : public Component {
public:
    // Takes the texture path, and the grid dimensions (columns and rows)
    SpriteSheetRenderer(const std::string& texturePath, int columns, int rows);
    ~SpriteSheetRenderer() override;

    void Start() override;
    void Render() override;

    // Changes the current frame to display
    void SetFrame(int frameIndex);
    int GetFrame() const { return m_currentFrame; }

    // Returns the total number of frames in the spritesheet
    int GetFrameCount() const { return m_columns * m_rows; }

private:
    Texture2D m_texture;
    Transform2d* m_transform;

    int m_columns;
    int m_rows;
    int m_currentFrame;
    
    // Calculates the specific source rectangle for the current frame
    Rectangle GetSourceRec() const;
};
