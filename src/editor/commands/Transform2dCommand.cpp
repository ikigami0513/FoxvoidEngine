#include "Transform2dCommand.hpp"

Transform2dCommand::Transform2dCommand(GameObject* object, const Transform2dState& oldState, const Transform2dState& newState)
    : m_object(object), m_oldState(oldState), m_newState(newState) 
{
}

void Transform2dCommand::Execute() {
    if (!m_object) return;
    
    // Apply the new state
    auto transform = m_object->GetComponent<Transform2d>();
    if (transform) {
        transform->position = m_newState.position;
        transform->rotation = m_newState.rotation;
        transform->scale = m_newState.scale;
    }
}

void Transform2dCommand::Undo() {
    if (!m_object) return;
    
    // Revert to the old state
    auto transform = m_object->GetComponent<Transform2d>();
    if (transform) {
        transform->position = m_oldState.position;
        transform->rotation = m_oldState.rotation;
        transform->scale = m_oldState.scale;
    }
}
