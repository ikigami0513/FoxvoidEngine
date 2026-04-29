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
#include <gui/Mask.hpp>

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

        // Triggers the render loop for all GameObjects in the scene based on Z-Index
        void Render() {
            // Create a temporary list of raw pointers to sort
            std::vector<GameObject*> renderList;
            renderList.reserve(m_gameObjects.size());
            for (auto& go : m_gameObjects) {
                renderList.push_back(go.get());
            }

            // Sort the list by Z-Index (Ascending: lower Z is drawn first, higher Z is drawn on top)
            // std::stable_sort is crucial here: it preserves the creation order for objects that have the SAME Z-Index
            std::stable_sort(renderList.begin(), renderList.end(), [](GameObject* a, GameObject* b) {
                int zA = 0;
                int zB = 0;
                
                auto tA = a->GetComponent<Transform2d>();
                auto tB = b->GetComponent<Transform2d>();
                
                if (tA) zA = tA->zIndex;
                if (tB) zB = tB->zIndex;
                
                return zA < zB;
            });

            // Render the sorted list
            for (auto* go : renderList) {
                go->Render();
            }
        }

        void RenderHUD() {
            for (auto& go : m_gameObjects) {
                if (go->GetParent() == nullptr) {
                    RenderHUDHierarchical(go.get());
                }
            }
        }

        void Clear(bool keepPersistent = true) {
            if (!keepPersistent) {
                // Editor stop mode: Destroy absolutely everything to restore the backup
                m_gameObjects.clear();
            }
            else {
                // Gameplay Mode: Keep objects that have the PersistentComponent
                // Use std::erase_if (C++20) to remove only non-persistent objects
                std::erase_if(m_gameObjects, [](const std::unique_ptr<GameObject>& go) {
                    // If the object has a PersistentComponent, we return false (do NOT erase)
                    if (go->GetComponent<PersistentComponent>()) {
                        return false;
                    }
                    return true; // Otherwise, erase it
                });
            }

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
        void LoadFromFile(const std::string& filepath, bool keepPersistent = true) {
            std::ifstream file(filepath);
            if (file.is_open()) {
                nlohmann::json j;
                try {
                    file >> j;
                    Deserialize(j, keepPersistent);
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

        void Deserialize(const nlohmann::json& j, bool keepPersistent = true) {
            Clear(keepPersistent); // Ensure the scene is totally empty before loading

            if (!j.contains("gameObjects")) return;

            for (const auto& goJson : j["gameObjects"]) {
                std::string name = goJson.value("name", "Entity");
                
                // Prevent duplicate Persistent Objects
                // Check if an object with the exact same name survived the Clear() process
                bool isDuplicate = false;
                for (const auto& existingGo : m_gameObjects) {
                    if (existingGo->name == name && existingGo->GetComponent<PersistentComponent>()) {
                        isDuplicate = true;
                        std::cout << "[Scene] Prevented spawning duplicate persistent object: " << name << std::endl;
                        break;
                    }
                }

                // If it already exists, skip loading this JSON object entirely
                if (isDuplicate) {
                    continue;
                }

                GameObject* go = CreateGameObject(name);

                go->Deserialize(goJson);
            }

            // Now that all objects exist in memory, we can safely link children to parents
            for (auto& go : m_pendingObjects) {
                if (go->pendingParentId != 0) { // If it's supposed to have a parent
                    
                    // Search for the parent in the pending list
                    GameObject* foundParent = nullptr;
                    for (auto& potentialParent : m_pendingObjects) {
                        if (potentialParent->id == go->pendingParentId) {
                            foundParent = potentialParent.get();
                            break;
                        }
                    }

                    // If not found in pending, check the persistent active objects
                    if (!foundParent) {
                        for (auto& potentialParent : m_gameObjects) {
                            if (potentialParent->id == go->pendingParentId) {
                                foundParent = potentialParent.get();
                                break;
                            }
                        }
                    }

                    // Link them!
                    if (foundParent) {
                        go->SetParent(foundParent);
                    } else {
                        std::cerr << "[Scene] Warning: Parent ID " << go->pendingParentId 
                                  << " not found for child " << go->name << std::endl;
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
            // Warning: This will overwrite the fresh ID with the one from the JSON
            newObj->Deserialize(j);

            // Regenerate a unique ID so this clone doesn't share the same ID as the file
            newObj->RegenerateID();
            
            // Break any old parent links. 
            // A freshly instantiated prefab should spawn at the root of the scene.
            newObj->pendingParentId = 0;

            std::cout << "[Scene] Successfully instantiated prefab: " << prefabPath << std::endl;
            
            // Return the pointer so the caller can manipulate it immediately
            return newObj;
        }

        // Returns the top-most GameObject at the given world position, or nullptr if empty
        GameObject* PickObject(Vector2 worldPos) {
            // Gather all objects
            std::vector<GameObject*> pickList;
            pickList.reserve(m_gameObjects.size());
            for (auto& go : m_gameObjects) {
                pickList.push_back(go.get());
            }

            // Sort the list by Z-Index (Descending: Highest Z is checked first!)
            std::stable_sort(pickList.begin(), pickList.end(), [](GameObject* a, GameObject* b) {
                int zA = 0, zB = 0;
                auto tA = a->GetComponent<Transform2d>();
                auto tB = b->GetComponent<Transform2d>();
                if (tA) zA = tA->zIndex;
                if (tB) zB = tB->zIndex;
                return zA > zB; // Note the > sign for descending order
            });

            // Check for collisions in sorted order
            for (auto* go : pickList) {
                auto transform = go->GetComponent<Transform2d>();
                if (!transform) continue;

                auto position = transform->GetGlobalPosition();

                // Default bounding box
                Rectangle bounds = { position.x - 25.0f, position.y - 25.0f, 50.0f, 50.0f };

                // Try to get precise bounds from a SpriteRenderer
                auto sprite = go->GetComponent<SpriteRenderer>();
                if (sprite && sprite->GetTexture().id != 0) {
                    float width = sprite->GetTexture().width * transform->scale.x;
                    float height = sprite->GetTexture().height * transform->scale.y;
                    bounds = { position.x - (width / 2.0f), position.y - (height / 2.0f), width, height };
                }

                // Try to get precise bounds from a SpriteSheetRenderer
                auto spriteSheet = go->GetComponent<SpriteSheetRenderer>();
                if (spriteSheet && spriteSheet->GetTexture().id != 0) {
                    Rectangle sourceRec = spriteSheet->GetSourceRec();
                    float width = sourceRec.width * std::abs(transform->scale.x);
                    float height = sourceRec.height * std::abs(transform->scale.y);
                    bounds = { position.x - (width / 2.0f), position.y - (height / 2.0f), width, height };
                }

                auto shape = go->GetComponent<ShapeRenderer>();
                if (shape) {
                    float width = shape->width * std::abs(transform->scale.x);
                    float height = shape->height * std::abs(transform->scale.y);
                    bounds = { position.x - (width / 2.0f), position.y - (height / 2.0f), width, height };
                }

                // If the click is inside the bounds, return this object immediately
                if (CheckCollisionPointRec(worldPos, bounds)) {
                    return go;
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

        // Retrieves the background color defined by the main camera
        Color GetMainCameraBackgroundColor() const {
            for (const auto& go : m_gameObjects) {
                auto cam = go->GetComponent<Camera2d>();
                if (cam && cam->isMain) {
                    return cam->backgroundColor;
                }
            }
            // Fallback color if no main camera is found in the scene
            return RAYWHITE; 
        }

    private:
        // Recursive function to draw the HUD hierarchy
        void RenderHUDHierarchical(GameObject* node) {
            bool isMasking = false;
            
            // 1. Check if this node is a Mask
            Mask* mask = node->GetComponent<Mask>();
            if (mask && mask->isActive) {
                // We need the RectTransform to know WHERE to cut
                if (RectTransform* rt = node->GetComponent<RectTransform>()) {
                    Rectangle rect = rt->GetScreenRect();
                    
                    // Raylib requires integers for Scissor Mode
                    BeginScissorMode((int)rect.x, (int)rect.y, (int)rect.width, (int)rect.height);
                    isMasking = true;
                }
            }

            // 2. Render this specific node
            node->RenderHUD();

            // 3. Render all children recursively (Painter's Algorithm: children are drawn on top)
            for (GameObject* child : node->GetChildren()) {
                RenderHUDHierarchical(child);
            }

            // 4. Close the mask if we opened one, so it doesn't affect siblings
            if (isMasking) {
                EndScissorMode();
            }
        }

        // The scene owns the GameObjects
        std::vector<std::unique_ptr<GameObject>> m_gameObjects;

        // The waiting room for newly instantiated objects
        std::vector<std::unique_ptr<GameObject>> m_pendingObjects;
};
