#pragma once

#include <imgui.h>
#include "../world/GameObject.hpp"
#include "../world/ComponentRegistry.hpp"

class InspectorPanel {
    public:
        void Draw(GameObject*& selectedObject);
};
