#include "Animation2d.hpp"
#include "SpriteSheetRenderer.hpp"
#include "../world/GameObject.hpp"
#include <iostream>
#include <cstring>

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

std::string Animation2d::GetName() const {
    return "Animation 2D";
}

// Converts {0, 1, 2} to "0, 1, 2"
std::string Animation2d::GetFramesAsString() const {
    std::stringstream ss;
    for (size_t i = 0; i < m_frames.size(); ++i) {
        ss << m_frames[i];
        if (i < m_frames.size() - 1) ss << ", ";
    }
    return ss.str();
}

// Converts "0, 1, 2" to {0, 1, 2}
void Animation2d::ParseFramesFromString(const std::string& str) {
    m_frames.clear();
    std::stringstream ss(str);
    std::string item;
    
    // Split by comma
    while (std::getline(ss, item, ',')) {
        try {
            m_frames.push_back(std::stoi(item)); // Convert text to int
        } catch (...) {
            // Ignore invalid text inputs (like letters) safely
        }
    }
    
    // Fallback: Ensure there is always at least one frame to avoid crashes
    if (m_frames.empty()) {
        m_frames.push_back(0);
    }
    m_currentIndex = 0; // Reset animation to start
}

void Animation2d::OnInspector() {
    ImGui::DragFloat("Speed (s)", &m_speed, 0.01f, 0.01f, 5.0f);
    ImGui::Checkbox("Loop", &m_loop);

    // Frame sequence editor
    std::string framesStr = GetFramesAsString();
    char buffer[256];
    strncpy(buffer, framesStr.c_str(), sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';

    if (ImGui::InputText("Frames", buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
        ParseFramesFromString(buffer);
    }
    ImGui::TextDisabled("Ex: 0, 1, 2, 3");
    
    // Read-only debug info
    if (m_frames.size() > 0 && m_currentIndex < m_frames.size()) {
        ImGui::Text("Current Frame: %d", m_frames[m_currentIndex]);
    }
}

nlohmann::json Animation2d::Serialize() const {
    return {
        {"type", "Animation2d"},
        {"frames", m_frames}, // nlohmann::json handles std::vector automatically!
        {"speed", m_speed},
        {"loop", m_loop}
    };
}

void Animation2d::Deserialize(const nlohmann::json& j) {
    m_speed = j.value("speed", 0.15f);
    m_loop = j.value("loop", true);
    
    if (j.contains("frames") && j["frames"].is_array()) {
        m_frames.clear();
        for (const auto& frame : j["frames"]) {
            m_frames.push_back(frame.get<int>());
        }
    }
    
    // Fallback
    if (m_frames.empty()) {
        m_frames.push_back(0);
    }
    
    m_currentIndex = 0;
    m_timer = 0.0f;
}
