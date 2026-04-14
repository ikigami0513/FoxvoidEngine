#include "ModifyComponentCommand.hpp"

ModifyComponentCommand::ModifyComponentCommand(Component* component, const nlohmann::json& oldState, const nlohmann::json& newState)
    : m_component(component), m_oldState(oldState), m_newState(newState)
{
}

void ModifyComponentCommand::Execute() {
    if (m_component) {
        // Apply the new JSON state
        m_component->Deserialize(m_newState);
    }
}

void ModifyComponentCommand::Undo() {
    if (m_component) {
        // Revert to the old JSON state
        m_component->Deserialize(m_oldState);
    }
}
