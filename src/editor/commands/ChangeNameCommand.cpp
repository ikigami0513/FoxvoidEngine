#include "ChangeNameCommand.hpp"

ChangeNameCommand::ChangeNameCommand(GameObject* object, const std::string& oldName, const std::string& newName)
    : m_object(object), m_oldName(oldName), m_newName(newName)
{
}

void ChangeNameCommand::Execute() {
    if (m_object) {
        m_object->name = m_newName;
    }
}

void ChangeNameCommand::Undo() {
    if (m_object) {
        m_object->name = m_oldName;
    }
}
