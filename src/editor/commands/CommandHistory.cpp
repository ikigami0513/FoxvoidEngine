#include "CommandHistory.hpp"

// Initialize the static stacks
std::vector<std::unique_ptr<ICommand>> CommandHistory::m_undoStack;
std::vector<std::unique_ptr<ICommand>> CommandHistory::m_redoStack;

void CommandHistory::AddCommand(std::unique_ptr<ICommand> command) {
    // When a new command is added, it executes immediately
    command->Execute();

    // We add it to the undo stack
    m_undoStack.push_back(std::move(command));

    // If we make a new action, the redo history is lost
    m_redoStack.clear();

    // Prevent memory overflow
    if (m_undoStack.size() > MAX_HISTORY) {
        m_undoStack.erase(m_undoStack.begin());
    }
}

void CommandHistory::Undo() {
    if (!CanUndo()) return;

    // Extract the last command
    auto command = std::move(m_undoStack.back());
    m_undoStack.pop_back();

    // Revert it
    command->Undo();

    // Push it to the redo stack
    m_redoStack.push_back(std::move(command));
}

void CommandHistory::Redo() {
    if (!CanRedo()) return;

    // Extract the last undone command
    auto command = std::move(m_redoStack.back());
    m_redoStack.pop_back();

    // Re-execute it
    command->Execute();

    // Push it back to the undo stack
    m_undoStack.push_back(std::move(command));
}

void CommandHistory::Clear() {
    m_undoStack.clear();
    m_redoStack.clear();
}

bool CommandHistory::CanUndo() {
    return !m_undoStack.empty();
}

bool CommandHistory::CanRedo() {
    return !m_redoStack.empty();
}
