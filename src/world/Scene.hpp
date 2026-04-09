#pragma once

#include <vector>
#include <memory>
#include <algorithm>
#include "GameObject.hpp"

class Scene {
    public:
        // Factory method to create a new GameObject inside this scene
        GameObject* CreateGameObject(const std::string& name) {
            auto go = std::make_unique<GameObject>(name);
            GameObject* ptr = go.get();
            
            // Instead of pushing to the main list directly and crashing the current loop,
            // we put the new object in a waiting room (pending objects).
            m_pendingObjects.push_back(std::move(go));

            return ptr;
        }

        // Read-only access to the game objects for the Editor
        // Returns a constant reference to the vector, preventing external modifications
        const std::vector<std::unique_ptr<GameObject>>& GetGameObjects() const {
            return gameObjects;
        }

        // Triggers the update loop for all GameObjects and cleans up dead ones
        void Update(float deltaTime) {
            // Update all objects
            for (auto& go : gameObjects) {
                go->Update(deltaTime);
            }

            // GARBAGE COLLECTION (Deferred Destruction)
            // Safely remove and destroy any GameObject marked as pending destroy.
            // Doing this AFTER the update loop prevents iteration crashes (iterator invalidation).
            std::erase_if(gameObjects, [](const std::unique_ptr<GameObject>& go) {
                return go->IsPendingDestroy();
            });

            // Now that the loop is over, it is safe to add our new objects into the main scene.
            for (auto& newGo : m_pendingObjects) {
                gameObjects.push_back(std::move(newGo));
            }
            // Clear the waiting room for the next frame
            m_pendingObjects.clear();
        }

        // Triggers the render loop for all GameObjects in the scene
        void Render() {
            for (auto& go : gameObjects) {
                go->Render();
            }
        }

        void Clear() {
            gameObjects.clear();
            m_pendingObjects.clear();
        }

    private:
        // The scene owns the GameObjects
        std::vector<std::unique_ptr<GameObject>> gameObjects;

        // The waiting room for newly instantiated objects
        std::vector<std::unique_ptr<GameObject>> m_pendingObjects;
};
