#pragma once

#include <memory>
#include "ICommand.hpp"
#include "world/Scene.hpp"
#include "world/GameObject.hpp"

class CreateObjectCommand : public ICommand {
    public:
        CreateObjectCommand(Scene& scene, GameObject* newObject);

        void Execute() override;
        void Undo() override;

    private:
        Scene& m_scene;
        GameObject* m_targetObject;
        std::unique_ptr<GameObject> m_savedObject;
        bool m_isFirstExecution;
};
