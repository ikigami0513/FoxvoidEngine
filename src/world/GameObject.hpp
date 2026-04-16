#pragma once

#include <vector>
#include <memory>
#include <string>
#include <iostream>
#include "Component.hpp"

class GameObject {
    public:
        std::string name;
        
        // Basic position (X, Y). Later, you might want to move this into a dedicated 'TransformComponent'
        float x = 0.0f;
        float y = 0.0f;

        GameObject(const std::string& name) : name(name) {}
        ~GameObject() = default;

        GameObject(const GameObject&) = delete;
        GameObject& operator=(const GameObject&) = delete;

        // Marks the GameObject to be destroyed at the end of the frame
        void Destroy() {
            m_pendingDestroy = true;
        }

        // Checks if the GameObject is scheduled for destruction
        bool IsPendingDestroy() const {
            return m_pendingDestroy;
        }

        void Start() {
            for (auto& component : components) {
                component->Start();
            }
        }

        // Iterate through all components and call their update methods
        void Update(float deltaTime) {
            for (auto& component : components) {
                component->Update(deltaTime);
            }
        }

        // Iterate through all components and call their render methods
        void Render() {
            for (auto& component : components) {
                component->Render();
            }
        }

        // Iterate and call HUD render methods
        void RenderHUD() {
            for (auto& component : components) {
                component->RenderHUD();
            }
        }

        // Relays the collision event to all attached components
        void OnCollision(const Collision2D& collision) {
            for (auto& component : components) {
                component->OnCollision(collision);
            }
        }

        // Safely removes a component from the entity
        void RemoveComponent(Component* compToRemove) {
            if (!compToRemove) return;

            // std::erase_if will find the component with this memory address
            // and remove it from the vector (which automatically calls its destructor)
            std::erase_if(components, [compToRemove](const std::unique_ptr<Component>& comp) {
                return comp.get() == compToRemove;
            });
        }

        // Template method to dynamically add any component type to this GameObject
        // Args&& allows passing constructor arguments directly
        template<typename T, typename... Args>
        T* AddComponent(Args&&... args) {
            // Create the component using modern C++ memory management
            auto newComponent = std::make_unique<T>(std::forward<Args>(args)...);
            
            // Link the component back to this GameObject
            newComponent->owner = this;
            
            // Keep a raw pointer to return to the user
            T* rawPointer = newComponent.get();
            
            // Transfer ownership to the vector
            components.push_back(std::move(newComponent));
            
            // Trigger the start lifecycle method
            rawPointer->Start(); 
            
            return rawPointer;
        }

        // Template method to find and retrieve a component by its type
        template<typename T>
        T* GetComponent() {
            for (auto& component : components) {
                // dynamic_cast checks at runtime if the component is of type T
                if (T* target = dynamic_cast<T*>(component.get())) {
                    return target;
                }
            }
            return nullptr; // Return null if the component is not found
        }

        // Access to components for the Editor
        // Returns a constant reference to the components list
        const std::vector<std::unique_ptr<Component>>& GetComponents() const {
            return components;
        }

        nlohmann::json Serialize() const {
            nlohmann::json j;
            j["name"] = name;
            j["components"] = nlohmann::json::array();

            for (const auto& comp : components) {
                j["components"].push_back(comp->Serialize());
            }

            return j;
        }

        // Reconstructs the GameObject and all its components from a JSON object
        void Deserialize(nlohmann::json& j);

    private:
        // We use unique_ptr so components are automatically destroyed when the GameObject dies
        std::vector<std::unique_ptr<Component>> components;

        // The death flag
        bool m_pendingDestroy = false;
};
