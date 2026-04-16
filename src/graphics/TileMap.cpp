#include "TileMap.hpp"
#include "physics/Transform2d.hpp"
#include "world/GameObject.hpp"
#include "graphics/Graphics.hpp"
#include <iostream>

#ifndef STANDALONE_MODE
#include "editor/EditorUI.hpp"
#include <imgui.h>
#endif

TileMap::TileMap()
    : tileSize { 32.0f, 32.0f }, tileSpacing(0), gridWidth(10), gridHeight(10), m_tilesetTexture{0}
{
    // By default, create a single base layer
    AddLayer("Background");

    m_layers[0].data[0] = 0; // Put tile ID 1 at Top-Left
    m_layers[0].data[1] = 0; // Put tile ID 2 next to it
}

TileMap::~TileMap() {
    if (m_tilesetTexture.id != 0) {
        UnloadTexture(m_tilesetTexture);
    }
}

std::string TileMap::GetName() const {
    return "Tile Map";
}

void TileMap::LoadTileset(const std::string& path) {
    if (m_tilesetTexture.id != 0) {
        UnloadTexture(m_tilesetTexture);
    }

    m_tilesetTexture = Graphics::LoadTextureFiltered(path);
    if (m_tilesetTexture.id != 0) {
        m_tilesetPath = path;
    }
}

void TileMap::Render() {
    // Safety checks
    if (m_tilesetTexture.id == 0 || m_layers.empty() || !owner) return;

    auto transform = owner->GetComponent<Transform2d>();
    if (!transform) return;

    int stepX = (int)tileSize.x + tileSpacing;
    int stepY = (int)tileSize.y + tileSpacing;

    // Safety check: Prevent division by zero
    if (stepX <= 0 || stepY <= 0) return;

    // Calculate how many columns the tileset texture has
    // +tileSpacing handles the edge case where the right/bottom edge of the image doesn't have a final padding
    int tilesetCols = (m_tilesetTexture.width + tileSpacing) / stepX;
    if (tilesetCols == 0) return; // Prevent division by zero if texture is too small

    // Loop through all layers from bottom to top
    for (const auto& layer : m_layers) {
        if (!layer.isVisible) continue;

        // Loop through the grid
        for (int y = 0; y < gridHeight; ++y) {
            for (int x = 0; x < gridWidth; ++x) {
                int index = y * gridWidth + x;

                if (index >= layer.data.size()) continue;
                
                int tileID = layer.data[index];
                if (tileID < 0) continue; // -1 represents an empty (transparent) tile

                // Calculate where to read the tile from the texture (Source Rectangle) using the step sizes
                float srcX = (float)((tileID % tilesetCols) * stepX);
                float srcY = (float)((tileID / tilesetCols) * stepY);

                Rectangle srcRec = { srcX, srcY, tileSize.x, tileSize.y };

                // Calculate where to draw the tile in the world (Destination Rectangle)
                // We round the coordinates to avoid sub-pixel positioning
                float dstX = std::round(transform->position.x + (x * tileSize.x * transform->scale.x));
                float dstY = std::round(transform->position.y + (y * tileSize.y * transform->scale.y));
                
                // We use ceil to slightly force the width/height to the upper pixel
                // This completely eliminates floating-point hairline cracks between tiles
                float dstWidth = std::ceil(tileSize.x * transform->scale.x);
                float dstHeight = std::ceil(tileSize.y * transform->scale.y);
                
                Rectangle dstRec = { dstX, dstY, dstWidth, dstHeight };

                // We don't want each individual tile to rotate around its own center, 
                // so origin is 0,0. (For full map rotation, logic would be more complex)
                Vector2 origin = { 0.0f, 0.0f };

                DrawTexturePro(m_tilesetTexture, srcRec, dstRec, origin, transform->rotation, WHITE);
            }
        }
    }
}

#ifndef STANDALONE_MODE
void TileMap::OnInspector() {
    // Grid setup
    if (ImGui::CollapsingHeader("Map Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Show Tile Grid", &showGrid);
        
        EditorUI::DragFloat2("Tile Size", &tileSize.x, 1.0f, this, 1.0f, 256.0f);
        EditorUI::DragInt("Tile Spacing", &tileSpacing, 1, this, 0, 64);

        int newWidth = gridWidth;
        int newHeight = gridHeight;
        
        if (ImGui::DragInt("Grid Width", &newWidth, 1, 1, 1000)) ResizeMap(newWidth, gridHeight);
        if (ImGui::DragInt("Grid Height", &newHeight, 1, 1, 1000)) ResizeMap(gridWidth, newHeight);
        
        // Simple file path input for prototype (Asset Browser drag-n-drop would be better later)
        static char pathBuffer[256];
        strncpy(pathBuffer, m_tilesetPath.c_str(), sizeof(pathBuffer));
        pathBuffer[sizeof(pathBuffer) - 1] = '\0';
        
        if (ImGui::InputText("Tileset Path", pathBuffer, sizeof(pathBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
            LoadTileset(pathBuffer);
        }
    }

    // Layers management
    if (ImGui::CollapsingHeader("Layers", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::Button("Add Layer")) {
            AddLayer("Layer " + std::to_string(m_layers.size()));
        }

        for (size_t i = 0; i < m_layers.size(); ++i) {
            ImGui::PushID(i);

            // UI layout for Layer controls
            ImGui::Checkbox("Vis", &m_layers[i].isVisible);
            ImGui::SameLine();

            ImGui::Checkbox("Solid", &m_layers[i].isSolid);
            ImGui::SameLine();
            
            // Allow renaming the layer
            char nameBuffer[64];
            strncpy(nameBuffer, m_layers[i].name.c_str(), sizeof(nameBuffer));
            if (ImGui::InputText("##name", nameBuffer, sizeof(nameBuffer))) {
                m_layers[i].name = nameBuffer;
            }
            ImGui::PopID();
        }
    }
}
#endif

void TileMap::AddLayer(const std::string& name) {
    m_layers.emplace_back(name, gridWidth, gridHeight);
}

bool TileMap::IsInBounds(int x, int y) const {
    return (x >= 0 && x < gridWidth && y >= 0 && y < gridHeight);
}

int TileMap::GetTile(int layerIndex, int x, int y) const {
    if (layerIndex < 0 || layerIndex >= m_layers.size() || !IsInBounds(x, y)) return -1;
    
    return m_layers[layerIndex].data[y * gridWidth + x];
}

void TileMap::SetTile(int layerIndex, int x, int y, int tileID) {
    if (layerIndex < 0 || layerIndex >= m_layers.size() || !IsInBounds(x, y)) return;
    
    m_layers[layerIndex].data[y * gridWidth + x] = tileID;
}

void TileMap::ResizeMap(int newWidth, int newHeight) {
    if (newWidth <= 0 || newHeight <= 0) return;

    // For every layer, we need to create a new buffer and copy the old data over
    // while respecting the 2D grid layout (we can't just standard resize the 1D vector)
    for (auto& layer : m_layers) {
        std::vector<int> newData(newWidth * newHeight, -1);

        for (int y = 0; y < std::min(gridHeight, newHeight); ++y) {
            for (int x = 0; x < std::min(gridWidth, newWidth); ++x) {
                newData[y * newWidth + x] = layer.data[y * gridWidth + x];
            }
        }
        layer.data = std::move(newData);
    }

    gridWidth = newWidth;
    gridHeight = newHeight;
}

nlohmann::json TileMap::Serialize() const {
    nlohmann::json j;
    j["type"] = "TileMap";
    j["tilesetPath"] = m_tilesetPath;
    j["tileSize"] = { tileSize.x, tileSize.y };
    j["tileSpacing"] = tileSpacing;
    j["gridWidth"] = gridWidth;
    j["gridHeight"] = gridHeight;
    j["showGrid"] = showGrid;

    nlohmann::json layersArray = nlohmann::json::array();
    for (const auto& layer : m_layers) {
        layersArray.push_back({
            {"name", layer.name},
            {"isVisible", layer.isVisible},
            {"isSolid", layer.isSolid},
            {"data", layer.data}
        });
    }
    j["layers"] = layersArray;

    return j;
}

void TileMap::Deserialize(const nlohmann::json& j) {
    // Safely extract the tileSize array
    if (j.contains("tileSize") && j["tileSize"].is_array() && j["tileSize"].size() == 2) {
        tileSize.x = j["tileSize"][0];
        tileSize.y = j["tileSize"][1];
    } else {
        // Fallback defaults
        tileSize.x = 32.0f;
        tileSize.y = 32.0f;
    }

    tileSpacing = j.value("tileSpacing", 0);

    // Extract standard values using correct default fallbacks
    gridWidth = j.value("gridWidth", 10);
    gridHeight = j.value("gridHeight", 10);

    showGrid = j.value("showGrid", true);
    
    std::string path = j.value("tilesetPath", "");
    if (!path.empty()) {
        LoadTileset(path);
    }

    m_layers.clear();
    if (j.contains("layers")) {
        for (const auto& layerJson : j["layers"]) {
            TileLayer layer(layerJson.value("name", "Layer"), gridWidth, gridHeight);
            layer.isVisible = layerJson.value("isVisible", true);
            layer.isSolid = layerJson.value("isSolid", false);
            
            // Safely copy the JSON array into the vector
            if (layerJson.contains("data") && layerJson["data"].is_array()) {
                layer.data = layerJson["data"].get<std::vector<int>>();
            }
            m_layers.push_back(layer);
        }
    }
}

std::vector<int> TileMap::GetLayerData(int layerIndex) const {
    if (layerIndex >= 0 && layerIndex < m_layers.size()) {
        return m_layers[layerIndex].data;
    }
    return {};
}

void TileMap::SetLayerData(int layerIndex, const std::vector<int>& data) {
    if (layerIndex >= 0 && layerIndex < m_layers.size()) {
        m_layers[layerIndex].data = data;
    }
}

std::vector<Rectangle> TileMap::GetCollisionRects() const {
    std::vector<Rectangle> rects;

    if (!owner) return rects;
    auto transform = owner->GetComponent<Transform2d>();
    if (!transform) return rects;

    for (const auto& layer : m_layers) {
        // Skip layers that don't have physics enabled
        if (!layer.isSolid) continue;

        for (int y = 0; y < gridHeight; ++y) {
            for (int x = 0; x < gridWidth; ++x) {
                int index = y * gridWidth + x;

                // Safety check: Prevent Buffer Overrun
                if (index >= layer.data.size()) continue;

                int tileID = layer.data[index];
                if (tileID < 0) continue; // Skip empty tiles

                // Calculate the world-space bounding box of this specific tile
                float dstX = transform->position.x + (x * tileSize.x * transform->scale.x);
                float dstY = transform->position.y + (y * tileSize.y * transform->scale.y);
                float dstWidth = tileSize.x * transform->scale.x;
                float dstHeight = tileSize.y * transform->scale.y;

                rects.push_back({ dstX, dstY, dstWidth, dstHeight });
            }
        }
    }

    return rects;
}

void TileMap::RenderGrid() const {
    if (!showGrid || !owner) return;

    auto transform = owner->GetComponent<Transform2d>();
    if (!transform) return;

    // Calculate dimensions taking scale into account
    float scaledWidth = tileSize.x * transform->scale.x;
    float scaledHeight = tileSize.y * transform->scale.y;
    float totalWidth = gridWidth * scaledWidth;
    float totalHeight = gridHeight * scaledHeight;

    // A nice semi-transparent white for the internal grid
    Color gridColor = { 255, 255, 255, 90 };

    // Draw vertical lines
    for (int x = 0; x <= gridWidth; ++x) {
        float posX = transform->position.x + (x * scaledWidth);
        DrawLineV(
            { posX, transform->position.y },
            { posX, transform->position.y + totalHeight },
            gridColor
        );
    }

    // Draw horizontal lines
    for (int y = 0; y <= gridHeight; ++y) {
        float posY = transform->position.y + (y * scaledHeight);
        DrawLineV(
            { transform->position.x, posY },
            { transform->position.x + totalWidth, posY },
            gridColor
        );
    }

    // Draw a thicker white border to clearly show the bounds of the TileMap
    DrawRectangleLinesEx(
        { transform->position.x, transform->position.y, totalWidth, totalHeight },
        2.0f, 
        WHITE
    );
}

TileLayer* TileMap::GetLayer(int index) {
    if (index >= 0 && index < m_layers.size()) {
        return &m_layers[index];
    }
    return nullptr;
}

TileLayer* TileMap::GetLayer(const std::string& name) {
    for (auto& layer : m_layers) {
        if (layer.name == name) {
            return &layer;
        }
    }
    return nullptr;
}
