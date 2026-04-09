#pragma once

#include "../world/Component.hpp"
#include <vector>

class SpriteSheetRenderer; // Forward declaration

class Animation2d : public Component {
public:
    // Initializes the animation with a frame sequence, speed, and looping option
    Animation2d(const std::vector<int>& frames, float speed, bool loop = true);
    ~Animation2d() override = default;

    void Start() override;
    void Update(float deltaTime) override;

private:
    std::vector<int> m_frames; // The sequence of frames to play
    float m_speed;             // Time in seconds per frame
    bool m_loop;               // Should the animation restart at the end?
    
    int m_currentIndex;        // Current position in the m_frames array
    float m_timer;             // Time accumulator
    
    SpriteSheetRenderer* m_sprite; // Pointer to the renderer we want to control
};
