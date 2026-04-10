#include "EditorCamera.hpp"

EditorCamera::EditorCamera(float screenWidth, float screenHeight) {
    m_camera = { 0 };
        
    // The offset is the center of the screen, so zooming happens from the center
    m_camera.offset = { screenWidth / 2.0f, screenHeight / 2.0f };
        
    // The target is the world coordinate the camera is looking at
    m_camera.target = { 0.0f, 0.0f };
        
    // Default rotation and zoom
    m_camera.rotation = 0.0f;
    m_camera.zoom = 1.0f;

    m_isPanning = false;
}

void EditorCamera::Update(bool isWindowHovered) {
    // Zooming
    if (isWindowHovered) {
        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            // Adjust zoom speed
            m_camera.zoom += wheel * 0.1f;

            // Clamp zoom to prevent flipping or zooming too far
            if (m_camera.zoom < 0.1f) m_camera.zoom = 0.1f;
            if (m_camera.zoom > 10.0f) m_camera.zoom = 10.0f;
        }
    }

    // Panning
    // Start panning if we press the middle mouse button while hovering the scene view
    if (isWindowHovered && IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) {
        m_isPanning = true;
    }

    // Stop panning when we release the middle mouse button
    if (IsMouseButtonReleased(MOUSE_BUTTON_MIDDLE)) {
        m_isPanning = false;
    }

    // If currently panning, move the camera target opposite to the mouse movement
    if (m_isPanning) {
        Vector2 mouseDelta = GetMouseDelta();

        // We divide by zoom so panning speed stays consistent regardless of zoom level
        m_camera.target.x -= mouseDelta.x / m_camera.zoom;
        m_camera.target.y -= mouseDelta.y / m_camera.zoom;
    }
}

void EditorCamera::Begin() {
    BeginMode2D(m_camera);
}

void EditorCamera::End() {
    EndMode2D();
}

void EditorCamera::DrawGrid(int slices, float spacing) {
    int halfSlices = slices / 2;

    for (int i = -halfSlices; i <= halfSlices; i++) {
        // Highlight the main X and Y axes (0) with distinct colors, use light gray for the rest
        Color xAxisColor = (i == 0) ? GREEN : LIGHTGRAY;
        Color yAxisColor = (i == 0) ? RED : LIGHTGRAY;

        // Draw vertical lines
        DrawLine(i * spacing, -halfSlices * spacing, i * spacing, halfSlices * spacing, yAxisColor);
            
        // Draw horizontal lines
        DrawLine(-halfSlices * spacing, i * spacing, halfSlices * spacing, i * spacing, xAxisColor);
    }
}
