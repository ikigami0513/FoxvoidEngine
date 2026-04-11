#pragma once

class ICommand {
    public:
        virtual ~ICommand() = default;

        // Applies the action
        virtual void Execute() = 0;

        // Reverts the action
        virtual void Undo() = 0;
};
