#pragma once

#include <raylib.h>

class EditorCamera {
    public:
        // Initialize the camera centered on the screen
        EditorCamera(float screenWidth, float screenHeight);

        // Handles user input for zooming and panning
        // isWindowHovered is passed from ImGui to ensure we only move when over the Scene View
        void Update(bool isWindowHovered);

        // Activates the camera for rendering
        void Begin();

        // Deactivates the camera
        void End();

        // Draws a background grid to help orient the user in the world space
        void DrawGrid(int slices, float spacing);

        Camera2D GetCamera() const { return m_camera; }

    private:
        Camera2D m_camera;
        bool m_isPanning;
};
