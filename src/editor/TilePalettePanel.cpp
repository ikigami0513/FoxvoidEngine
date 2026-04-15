#include "TilePalettePanel.hpp"
#include <imgui.h>
#include <rlImGui.h>
#include <algorithm>
#include "extras/IconsFontAwesome6.h"

TilePalettePanel::TilePalettePanel() : m_zoom(2.0f) {}

void TilePalettePanel::Draw(int& selectedTileID, Texture2D currentTileset, Vector2 tileSize, int tileSpacing) {
    if (!isOpen) return;

    if (ImGui::Begin("Tile Palette", &isOpen)) {
        if (currentTileset.id == 0) {
            ImGui::TextDisabled("No tileset texture loaded.");
            ImGui::End();
            return;
        }

        // Controls
        // Reduce the slider width to make room for the eraser button on the same line
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 200);
        ImGui::SliderFloat("Zoom", &m_zoom, 1.0f, 5.0f);
        ImGui::PopItemWidth();

        ImGui::SameLine();

        // Eraser Button
        bool isEraserActive = (selectedTileID == -1);

        if (isEraserActive) {
            // If the eraser is active, tint the button red to provide clear visual feedback
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f)); 
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
        }

        if (ImGui::Button(ICON_FA_ERASER " Eraser")) {
            selectedTileID = -1; // -1 represents an empty tile (eraser tool)
        }

        if (isEraserActive) {
            // Restore the default button colors
            ImGui::PopStyleColor(3);
        }

        ImGui::Separator();

        // Palette display
        // We use a child window to allow scrolling if the tileset is large
        ImGui::BeginChild("PaletteRegion", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

        float displayWidth = currentTileset.width * m_zoom;
        float displayHeight = currentTileset.height * m_zoom;

        // Save the start position of the image in screen coordinates
        ImVec2 screenPos = ImGui::GetCursorScreenPos();

        // Draw the tileset texture
        rlImGuiImageSize(&currentTileset, (int)displayWidth, (int)displayHeight);

        // Interaction logic
        bool isHovered = ImGui::IsItemHovered();
        
        int stepPxX = (int)tileSize.x + tileSpacing;
        int stepPxY = (int)tileSize.y + tileSpacing;
        int cols = (currentTileset.width + tileSpacing) / stepPxX;
        int rows = (currentTileset.height + tileSpacing) / stepPxY;

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImU32 gridColor = IM_COL32(200, 200, 200, 60);

        // Draw individual tile boxes (skipping the spacing gaps)
        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                ImVec2 p_min = ImVec2(screenPos.x + (x * stepPxX * m_zoom), screenPos.y + (y * stepPxY * m_zoom));
                ImVec2 p_max = ImVec2(p_min.x + (tileSize.x * m_zoom), p_min.y + (tileSize.y * m_zoom));
                drawList->AddRect(p_min, p_max, gridColor);
            }
        }

        // Selection Logic
        if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            ImVec2 mousePos = ImGui::GetMousePos();
            
            // Calculate local position in standard unzoomed pixels
            int localX = (int)((mousePos.x - screenPos.x) / m_zoom);
            int localY = (int)((mousePos.y - screenPos.y) / m_zoom);

            int gridX = localX / stepPxX;
            int gridY = localY / stepPxY;

            // Modulo math to check if the click fell inside the spacing gap (dead zone)
            int modX = localX % stepPxX;
            int modY = localY % stepPxY;

            // Only select the tile if we clicked inside the actual tile area
            if (modX < (int)tileSize.x && modY < (int)tileSize.y && gridX < cols && gridY < rows) {
                selectedTileID = gridY * cols + gridX;
            }
        }

        // Visual feedback: Draw a thick white rectangle around the selected tile
        if (selectedTileID >= 0) {
            int selX = selectedTileID % cols;
            int selY = selectedTileID / cols;

            ImVec2 rectMin = {
                screenPos.x + (selX * stepPxX * m_zoom),
                screenPos.y + (selY * stepPxY * m_zoom)
            };
            ImVec2 rectMax = {
                rectMin.x + (tileSize.x * m_zoom),
                rectMin.y + (tileSize.y * m_zoom)
            };

            drawList->AddRect(rectMin, rectMax, IM_COL32(255, 255, 255, 255), 0.0f, 0, 2.5f);
        }

        ImGui::EndChild();
    }

    ImGui::End();
}
