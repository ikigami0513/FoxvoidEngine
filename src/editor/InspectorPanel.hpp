#pragma once

#ifndef STANDALONE_MODE
#include <imgui.h>
#endif

#include "../world/GameObject.hpp"
#include "../world/ComponentRegistry.hpp"

class InspectorPanel {
    public:
        void Draw(GameObject*& selectedObject, py::object& selectedAsset, std::string& selectedAssetPath);
};
