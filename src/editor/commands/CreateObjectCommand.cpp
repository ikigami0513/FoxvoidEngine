#include "CreateObjectCommand.hpp"

CreateObjectCommand::CreateObjectCommand(Scene& scene, GameObject* newObject)
    : m_scene(scene), m_targetObject(newObject), m_savedObject(nullptr), m_isFirstExecution(true)
{
}

void CreateObjectCommand::Execute() {
    if (m_isFirstExecution) {
        // The object was already created by the UI before calling this command.
        // We do nothing on the very first execution.
        m_isFirstExecution = false;
    }
    else {
        // Redo: Inject the object back into the scene
        if (m_savedObject) {
            m_scene.InjectGameObject(std::move(m_savedObject));
        }
    }
}

void CreateObjectCommand::Undo() {
    // Undo: Remove the object from the scene and hold it here
    m_savedObject = m_scene.ExtractGameObject(m_targetObject);
}
