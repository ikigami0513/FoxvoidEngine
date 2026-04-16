#include "ComponentRegistration.hpp"
#include "ComponentRegistry.hpp"

#include "physics/Transform2d.hpp"
#include "graphics/ShapeRenderer.hpp"
#include "graphics/SpriteRenderer.hpp"
#include "graphics/SpriteSheetRenderer.hpp"
#include "graphics/Animation2d.hpp"
#include "graphics/Animator2d.hpp"
#include "scripting/ScriptComponent.hpp"
#include "physics/RectCollider.hpp"
#include "physics/RigidBody2d.hpp"
#include "graphics/Camera2d.hpp"
#include "graphics/TileMap.hpp"
#include "gui/TextRenderer.hpp"

namespace EngineSetup {
    void RegisterNativeComponents() {
        // Register all native components to the C++ Engine.
        ComponentRegistry::RegisterCPP<Transform2d>("Transform2d");
        ComponentRegistry::RegisterCPP<RectCollider>("RectCollider");
        ComponentRegistry::RegisterCPP<RigidBody2d>("RigidBody2d");

        ComponentRegistry::RegisterCPP<ShapeRenderer>("ShapeRenderer");
        ComponentRegistry::RegisterCPP<SpriteRenderer>("SpriteRenderer");
        ComponentRegistry::RegisterCPP<SpriteSheetRenderer>("SpriteSheetRenderer");
        ComponentRegistry::RegisterCPP<Animation2d>("Animation2d");
        ComponentRegistry::RegisterCPP<Animator2d>("Animator2d");
        ComponentRegistry::RegisterCPP<Camera2d>("Camera2d");
        ComponentRegistry::RegisterCPP<TileMap>("TileMap");

        ComponentRegistry::RegisterCPP<TextRenderer>("TextRenderer");

        ComponentRegistry::RegisterCPP<ScriptComponent>("ScriptComponent");
    }
}
