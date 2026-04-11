#include "DeleteObjectCommand.hpp"

DeleteObjectCommand::DeleteObjectCommand(Scene& scene, GameObject* object)
    : m_scene(scene), m_targetObject(object), m_savedObject(nullptr)
{}

void DeleteObjectCommand::Execute() {
    // Redo or initial execution
    // Remove the object from the scene and store it safely in this command
    m_savedObject = m_scene.ExtractGameObject(m_targetObject);
}

void DeleteObjectCommand::Undo() {
    if (m_savedObject) {
        // Undo: Give the object back to the scene
        m_scene.InjectGameObject(std::move(m_savedObject));

        // Note: m_savedObject is now nullptr, but m_targetObject
        // still points to the valid memory now managed by the scene again.
    }
}
