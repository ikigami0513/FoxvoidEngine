#include "physics/SpatialHashGrid.hpp"
#include <algorithm>
#include <cmath>

SpatialHashGrid::SpatialHashGrid(float cellSize) : m_cellSize(cellSize) {}

void SpatialHashGrid::Clear() {
    m_cells.clear();
}

std::pair<int, int> SpatialHashGrid::GetCellCoords(Vector2 pos) const {
    // std::floor ensures that negative coordinates map correctly (e.g., -0.5 becomes cell -1)
    return {
        static_cast<int>(std::floor(pos.x / m_cellSize)),
        static_cast<int>(std::floor(pos.y / m_cellSize))
    };
}

void SpatialHashGrid::GetAABB(const ColliderData& data, Vector2& outMin, Vector2& outMax) const {
    if (data.shapeType == ColliderShape::Polygon) {
        outMin = data.vertices[0];
        outMax = data.vertices[0];
        for (const auto& v : data.vertices) {
            if (v.x < outMin.x) outMin.x = v.x;
            if (v.y < outMin.y) outMin.y = v.y;
            if (v.x > outMax.x) outMax.x = v.x;
            if (v.y > outMax.y) outMax.y = v.y;
        }
    } 
    else if (data.shapeType == ColliderShape::Circle) {
        outMin = { data.center.x - data.radius, data.center.y - data.radius };
        outMax = { data.center.x + data.radius, data.center.y + data.radius };
    } 
    else if (data.shapeType == ColliderShape::Capsule) {
        outMin.x = std::min(data.p1.x, data.p2.x) - data.radius;
        outMin.y = std::min(data.p1.y, data.p2.y) - data.radius;
        outMax.x = std::max(data.p1.x, data.p2.x) + data.radius;
        outMax.y = std::max(data.p1.y, data.p2.y) + data.radius;
    }
}

void SpatialHashGrid::Insert(GameObject* obj, const ColliderData& data) {
    Vector2 min, max;
    GetAABB(data, min, max);

    std::pair<int, int> minCell = GetCellCoords(min);
    std::pair<int, int> maxCell = GetCellCoords(max);

    // Insert the object into every cell its bounding box overlaps
    for (int x = minCell.first; x <= maxCell.first; ++x) {
        for (int y = minCell.second; y <= maxCell.second; ++y) {
            m_cells[{x, y}].push_back(obj);
        }
    }
}

std::vector<GameObject*> SpatialHashGrid::GetNearby(const ColliderData& data) {
    Vector2 min, max;
    GetAABB(data, min, max);

    std::pair<int, int> minCell = GetCellCoords(min);
    std::pair<int, int> maxCell = GetCellCoords(max);

    std::vector<GameObject*> nearbyObjects;
    
    // We use a Set to prevent checking the same object twice if it spans multiple cells
    std::unordered_set<GameObject*> uniqueObjects;

    for (int x = minCell.first; x <= maxCell.first; ++x) {
        for (int y = minCell.second; y <= maxCell.second; ++y) {
            auto it = m_cells.find({x, y});
            if (it != m_cells.end()) {
                for (GameObject* obj : it->second) {
                    if (uniqueObjects.insert(obj).second) { // .second is true if it was newly inserted
                        nearbyObjects.push_back(obj);
                    }
                }
            }
        }
    }

    return nearbyObjects;
}
