#pragma once

#ifndef STANDALONE_MODE
#include <imgui.h>
#endif

#include "../world/Scene.hpp"
#include "../world/GameObject.hpp"

class HierarchyPanel {
    public:
        void Draw(Scene& activeScene, GameObject*& selectedObject);

    private:
        // Resursive helper to draw a GameObject and all its children
        void DrawNode(Scene& activeScene, GameObject* node, GameObject*& selectedObject, GameObject*& objectToDelete);
};
