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
#include "physics/PhysicsEngine.hpp"
#include "graphics/Camera2d.hpp"
#include "PersistentComponent.hpp"

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
            return m_gameObjects;
        }

        void Start() {
            for (auto& go : m_gameObjects) {
                go->Start();
            }
            
            for (auto& go : m_pendingObjects) {
                go->Start();
            }
        }

        // Runs game logic
        void Update(float deltaTime) {
            // Update all objects
            for (auto& go : m_gameObjects) {
                go->Update(deltaTime);
            }

            // Run the physics engine
            PhysicsEngine::Update(*this, deltaTime);
        }

        // Memory management separated from game logic
        // Safely adds pending objects and removes destroyed ones
        void Flush() {
            // GARBAGE COLLECTION (Deferred Destruction)
            // Safely remove and destroy any GameObject marked as pending destroy.
            // Doing this AFTER the update loop prevents iteration crashes (iterator invalidation).
            std::erase_if(m_gameObjects, [](const std::unique_ptr<GameObject>& go) {
                return go->IsPendingDestroy();
            });

            // Now that the loop is over, it is safe to add our new objects into the main scene.
            for (auto& newGo : m_pendingObjects) {
                m_gameObjects.push_back(std::move(newGo));
            }
            // Clear the waiting room for the next frame
            m_pendingObjects.clear();
        }

        // Triggers the render loop for all GameObjects in the scene
        void Render() {
            for (auto& go : m_gameObjects) {
                go->Render();
            }
        }

        void RenderHUD() {
            for (auto& go : m_gameObjects) {
                go->RenderHUD();
            }
        }

        void Clear() {
            // Use std::erase_if (C++20) to remove only non-persistent objects
            std::erase_if(m_gameObjects, [](const std::unique_ptr<GameObject>& go) {
                // If the object has a PersistentComponent, we return false (do NOT erase)
                if (go->GetComponent<PersistentComponent>()) {
                    return false;
                }
                return true; // Otherwise, erase it
            });

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
            for (const auto& go : m_gameObjects) {
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

        // Instantiates a new GameObject from a JSON prefab file
        GameObject* Instantiate(const std::string& prefabPath) {
            // Open the prefab JSON file
            std::ifstream file(prefabPath);
            if (!file.is_open()) {
                std::cerr << "[Scene] Error: Could not open prefab file: " << prefabPath << std::endl;
                return nullptr;
            }

            // Parse the JSON data
            nlohmann::json j;
            file >> j;
            file.close();

            // Extract the name, adding a suffix to indicate it's a clone
            std::string name = j.value("name", "New Prefab") + " (Clone)";

            // Create a fresh GameObject in the current scene
            GameObject* newObj = CreateGameObject(name);

            //  Inject the saved data into the new object
            // This will reconstruct all the components (Transform, Sprite, Scripts...)
            newObj->Deserialize(j);

            std::cout << "[Scene] Successfully instantiated prefab: " << prefabPath << std::endl;
            
            // Return the pointer so the caller can manipulate it immediately
            return newObj;
        }

        // Returns the top-most GameObject at the given world position, or nullptr if empty
        GameObject* PickObject(Vector2 worldPos) {
            // We iterate backwards (rbegin to rend) to pick the object drawn ON TOP first
            for (auto it = m_gameObjects.rbegin(); it != m_gameObjects.rend(); ++it) {
                auto& go = *it;
                auto transform = go->GetComponent<Transform2d>();

                if (!transform) continue; // If it has no transform, it can't be clicked

                // Default bounding box (fallback if the object has no renderer)
                Rectangle bounds = { transform->position.x - 25.0f, transform->position.y - 25.0f, 50.0f, 50.0f };

                // Try to get precise bounds from a SpriteRenderer
                auto sprite = go->GetComponent<SpriteRenderer>();
                if (sprite && sprite->GetTexture().id != 0) {
                    float width = sprite->GetTexture().width * transform->scale.x;
                    float height = sprite->GetTexture().height * transform->scale.y;
                    // Subtract half width/height because our origin is at the center
                    bounds = { transform->position.x - (width / 2.0f), transform->position.y - (height / 2.0f), width, height };
                }

                // Try to get precise bounds from a SpriteSheetRenderer
                auto spriteSheet = go->GetComponent<SpriteSheetRenderer>();
                if (spriteSheet && spriteSheet->GetTexture().id != 0) {
                    Rectangle sourceRec = spriteSheet->GetSourceRec();
                    float width = sourceRec.width * std::abs(transform->scale.x);
                    float height = sourceRec.height * std::abs(transform->scale.y);
                    bounds = { transform->position.x - (width / 2.0f), transform->position.y - (height / 2.0f), width, height };
                }

                auto shape = go->GetComponent<ShapeRenderer>();
                if (shape) {
                    float width = shape->width * std::abs(transform->scale.x);
                    float height = shape->height * std::abs(transform->scale.y);
                    bounds = { transform->position.x - (width / 2.0f), transform->position.y - (height / 2.0f), width, height };
                }

                // Check if the world cursor intersects with the calculated bounding box
                if (CheckCollisionPointRec(worldPos, bounds)) {
                    return go.get();
                }
            }

            return nullptr;
        }

        // Remove the object from the scene but returns its ownership without destroying it
        std::unique_ptr<GameObject> ExtractGameObject(GameObject* target) {
            // Find the object in our container
            auto it = std::find_if(m_gameObjects.begin(), m_gameObjects.end(),
                [target](const std::unique_ptr<GameObject>& go) { return go.get() == target; });

            if (it != m_gameObjects.end()) {
                // Transfer ownership out of the vector
                std::unique_ptr<GameObject> extractedObject = std::move(*it);

                // Remove the empty pointer from the vector
                m_gameObjects.erase(it);

                return extractedObject;
            }

            return nullptr;
        }

        // Puts an existing object back into the scene
        void InjectGameObject(std::unique_ptr<GameObject> object) {
            if (object) {
                // Put the object back into the scene
                m_gameObjects.push_back(std::move(object));
            }
        }

        // Finds the primary Camera2d in the scene and returns its Raylib equivalent
        Camera2D GetMainCamera(float screenWidth, float screenHeight) const {
            for (const auto& go : m_gameObjects) {
                auto cam = go->GetComponent<Camera2d>();
                if (cam and cam->isMain) {
                    return cam->GetCamera(screenWidth, screenHeight);
                }
            }

            // Fallback: If no camera exists in the scene, return a static default camera
            // so the game doesn't crash or render as a black screen.
            Camera2D defaultCam = { 0 };
            defaultCam.zoom = 1.0f;
            defaultCam.offset.x = screenWidth / 2.0f;
            defaultCam.offset.y = screenHeight / 2.0f;
            return defaultCam;
        }

    private:
        // The scene owns the GameObjects
        std::vector<std::unique_ptr<GameObject>> m_gameObjects;

        // The waiting room for newly instantiated objects
        std::vector<std::unique_ptr<GameObject>> m_pendingObjects;
};
