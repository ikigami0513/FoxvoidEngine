#pragma once

#include <imgui.h>
#include "../world/Scene.hpp"
#include "../world/GameObject.hpp"

class HierarchyPanel {
    public:
        void Draw(Scene& activeScene, GameObject*& selectedObject);
};
