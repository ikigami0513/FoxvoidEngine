#pragma once

#include <vector>
#include <memory>
#include <string>
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

    private:
        // We use unique_ptr so components are automatically destroyed when the GameObject dies
        std::vector<std::unique_ptr<Component>> components;
};
