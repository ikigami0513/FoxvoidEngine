#pragma once

#include "../world/Component.hpp"
#include <string>
#include <unordered_map>
#include <vector>

// Forward declarations to avoid cyclic includes and speed up compilation
class SpriteSheetRenderer;

// Struct to hold individual animation data
struct AnimationData {
    std::vector<int> frames;
    float frameDuration;
    bool loop;
};

class Animator2d : public Component {
    public:
        Animator2d();
        ~Animator2d() override = default;

        // Called every frame by the Engine/Scene
        void Update(float deltaTime) override;

        // Registers a new animation state
        void AddAnimation(const std::string& name, const std::vector<int>& frames, float frameDuration, bool loop);

        // Switches to a new animation if it exists and isn't already playing
        void Play(const std::string& name);

        std::string GetName() const override;
        void OnInspector() override;

        nlohmann::json Serialize() const override;
        void Deserialize(const nlohmann::json& j) override;

    private:
        // Updates the visual representation on the SpriteSheetRenderer
        void UpdateSprite();

        std::unordered_map<std::string, AnimationData> m_animations;
        std::string m_currentAnimation;
        
        int m_currentFrameIndex;
        float m_timer;
        
        // Cached pointer to the renderer to avoid querying it every frame
        SpriteSheetRenderer* m_spriteRenderer;
};
