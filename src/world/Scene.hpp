#pragma once

#include <vector>
#include <memory>
#include <algorithm>
#include "GameObject.hpp"
#include "../physics/Transform2d.hpp"
#include <graphics/ShapeRenderer.hpp>
#include <graphics/SpriteRenderer.hpp>
#include <graphics/SpriteSheetRenderer.hpp>
#include <graphics/Animation2d.hpp>

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

        // Runs game logic
        void Update(float deltaTime) {
            // Update all objects
            for (auto& go : gameObjects) {
                go->Update(deltaTime);
            }
        }

        // Memory management separated from game logic
        // Safely adds pending objects and removes destroyed ones
        void Flush() {
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

        nlohmann::json Serialize() const {
            nlohmann::json j;
            j["gameObjects"] = nlohmann::json::array();
            for (const auto& go : gameObjects) {
                j["gameObjects"].push_back(go->Serialize());
            }
            return j;
        }

        void Deserialize(const nlohmann::json& j) {
            Clear(); // Ensure the scene is totally empty before loading

            if (!j.contains("gameObjects")) return;

            for (const auto& goJson : j["gameObjects"]) {
                std::string name = goJson.value("name", "Entity");
                GameObject* go = CreateGameObject(name);

                // Reconstruct components (Simple Factory approach for now)
                if (goJson.contains("components")) {
                    for (const auto& compJson : goJson["components"]) {
                        std::string type = compJson.value("type", "");
                        
                        if (type == "Transform2d") {
                            auto* t = go->AddComponent<Transform2d>();
                            t->Deserialize(compJson);
                        }
                        else if (type == "ShapeRenderer") {
                            auto* sr = go->AddComponent<ShapeRenderer>();
                            sr->Deserialize(compJson);
                        }
                        else if (type == "SpriteRenderer") {
                            auto* spr = go->AddComponent<SpriteRenderer>();
                            spr->Deserialize(compJson);
                        }
                        else if (type == "SpriteSheetRenderer") {
                            auto* ssr = go->AddComponent<SpriteSheetRenderer>();
                            ssr->Deserialize(compJson);
                        }
                        else if (type == "Animation2d") {
                            auto* anim = go->AddComponent<Animation2d>();
                            anim->Deserialize(compJson);
                        }
                    }
                }
            }
            Flush(); // Crucial: move them from pending to active!
        }

    private:
        // The scene owns the GameObjects
        std::vector<std::unique_ptr<GameObject>> gameObjects;

        // The waiting room for newly instantiated objects
        std::vector<std::unique_ptr<GameObject>> m_pendingObjects;
};
