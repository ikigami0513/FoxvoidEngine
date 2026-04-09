#pragma once

// Forward declaration to avoid circular dependencies
class GameObject; 

class Component {
    public:
        // Pointer to the GameObject that owns this component
        GameObject* owner = nullptr; 

        Component() = default;

        // Virtual destructor is mandatory for proper cleanup of derived classes
        virtual ~Component() = default;

        Component(const Component&) = delete;
        Component& operator=(const Component&) = delete;

        // Called once when the component is initialized
        virtual void Start() {}

        // Called every frame for logic and physics
        virtual void Update(float deltaTime) {}

        // Called every frame after update, specifically for drawing via Raylib
        virtual void Render() {}
};
