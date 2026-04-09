#pragma once

#include <vector>
#include <memory>
#include "GameObject.hpp"

class Scene {
    public:
        // Factory method to create a new GameObject inside this scene
        GameObject* CreateGameObject(const std::string& name) {
            auto go = std::make_unique<GameObject>(name);
            GameObject* ptr = go.get();
            gameObjects.push_back(std::move(go));
            return ptr;
        }

        // Triggers the update loop for all GameObjects in the scene
        void Update(float deltaTime) {
            for (auto& go : gameObjects) {
                go->Update(deltaTime);
            }
        }

        // Triggers the render loop for all GameObjects in the scene
        void Render() {
            for (auto& go : gameObjects) {
                go->Render();
            }
        }

        void Clear() {
            gameObjects.clear();
        }

    private:
        // The scene owns the GameObjects
        std::vector<std::unique_ptr<GameObject>> gameObjects;
};
