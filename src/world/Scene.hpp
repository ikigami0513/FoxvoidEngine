#pragma once

#include <vector>
#include <memory>
#include <fstream>
#include <iostream>
#include <algorithm>
#include "GameObject.hpp"
#include "../physics/Transform2d.hpp"
#include <graphics/ShapeRenderer.hpp>
#include <graphics/SpriteRenderer.hpp>
#include <graphics/SpriteSheetRenderer.hpp>
#include <graphics/Animation2d.hpp>
#include "../scripting/ScriptComponent.hpp"
#include "../graphics/Animator2d.hpp"
#include <scripting/ScriptBindings.hpp>
#include "ComponentRegistry.hpp"

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

        void Start() {
            for (auto& go : gameObjects) {
                go->Start();
            }
            
            for (auto& go : m_pendingObjects) {
                go->Start();
            }
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

        // Saves the serialized scene to a JSON file on the disk
        void SaveToFile(const std::string& filepath) const {
            nlohmann::json j = Serialize();

            std::ofstream file(filepath);
            if (file.is_open()) {
                // dump(4) adds an indentation of 4 spaces for readability
                file << j.dump(4);
                file.close();
                std::cout << "[Scene] Successfully saved to: " << filepath << std::endl;
            } else {
                std::cerr << "[Scene] Failed to open file for saving: " << filepath << std::endl;
            }
        }

        // Loads a scene from a JSON file
        void LoadFromFile(const std::string& filepath) {
            std::ifstream file(filepath);
            if (file.is_open()) {
                nlohmann::json j;
                try {
                    file >> j;
                    Deserialize(j);
                    std::cout << "[Scene] Successfully loaded from: " << filepath << std::endl;
                } catch (const nlohmann::json::parse_error& e) {
                    std::cerr << "[Scene] JSON parsing error in " << filepath << ":\n" << e.what() << std::endl;
                }
                file.close();
            } else {
                std::cerr << "[Scene] Failed to open file for loading: " << filepath << std::endl;
            }
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
                        
                        // Search for the type in our C++ factory registry
                        auto it = ComponentRegistry::factories.find(type);
                        
                        if (it != ComponentRegistry::factories.end()) {
                            // Execute the lambda to create the specific component
                            Component* newComp = it->second(*go);
                            
                            // Polymorphic deserialization: 
                            // The virtual method ensures the correct derived logic is called
                            newComp->Deserialize(compJson);
                        } else {
                            std::cerr << "[Scene] Warning: Unknown component type in JSON: " << type << std::endl;
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
