#pragma once

#include "ICommand.hpp"
#include <memory>
#include <vector>

class CommandHistory {
    public:
        // Add a new command and execute it
        static void AddCommand(std::unique_ptr<ICommand> command);

        static void Undo();
        static void Redo();
        static void Clear();

        static bool CanUndo();
        static bool CanRedo();

    private:
        static std::vector<std::unique_ptr<ICommand>> m_undoStack;
        static std::vector<std::unique_ptr<ICommand>> m_redoStack;

        // List to avoid infinite memory usage
        static const size_t MAX_HISTORY = 50;
};
