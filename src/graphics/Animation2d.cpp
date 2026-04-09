#include "Animation2d.hpp"
#include "SpriteSheetRenderer.hpp"
#include "../world/GameObject.hpp"
#include <iostream>

Animation2d::Animation2d(const std::vector<int>& frames, float speed, bool loop)
    : m_frames(frames), m_speed(speed), m_loop(loop), 
      m_currentIndex(0), m_timer(0.0f), m_sprite(nullptr) {}

void Animation2d::Start() {
    // Fetch the renderer attached to the same GameObject
    m_sprite = owner->GetComponent<SpriteSheetRenderer>();
    
    if (!m_sprite) {
        std::cerr << "[Animation2d] Warning: No SpriteSheetRenderer found!" << std::endl;
    } else if (!m_frames.empty()) {
        // Apply the very first frame immediately
        m_sprite->SetFrame(m_frames[0]);
    }
}

void Animation2d::Update(float deltaTime) {
    // Do nothing if we lack a sprite or frames
    if (!m_sprite || m_frames.empty()) return;

    // Accumulate the time passed since the last frame
    m_timer += deltaTime;

    // Check if it's time to advance the animation
    if (m_timer >= m_speed) {
        m_timer -= m_speed; // Keep the remainder for perfect timing precision
        m_currentIndex++;

        // Handle the end of the animation sequence
        if (m_currentIndex >= m_frames.size()) {
            if (m_loop) {
                m_currentIndex = 0; // Back to the beginning
            } else {
                m_currentIndex = m_frames.size() - 1; // Stay on the last frame
            }
        }
        
        // Tell the renderer to display the new frame
        m_sprite->SetFrame(m_frames[m_currentIndex]);
    }
}
