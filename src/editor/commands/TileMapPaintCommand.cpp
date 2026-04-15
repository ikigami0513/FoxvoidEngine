#include "TileMapPaintCommand.hpp"

TileMapPaintCommand::TileMapPaintCommand(TileMap* tileMap, int layerIndex, const std::vector<int>& oldData, const std::vector<int>& newData)
    : m_tileMap(tileMap), m_layerIndex(layerIndex), m_oldData(oldData), m_newData(newData) {}

void TileMapPaintCommand::Execute() {
    if (m_tileMap) {
        // Overwrite the layer with the new painted data
        m_tileMap->SetLayerData(m_layerIndex, m_newData);
    }
}

void TileMapPaintCommand::Undo() {
    if (m_tileMap) {
        // Restore the layer to how it was before the mouse click
        m_tileMap->SetLayerData(m_layerIndex, m_oldData);
    }
}
