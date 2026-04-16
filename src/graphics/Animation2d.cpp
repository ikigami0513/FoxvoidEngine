#include "Animation2d.hpp"
#include "SpriteSheetRenderer.hpp"
#include "../world/GameObject.hpp"
#include <iostream>
#include <cstring>
#include <editor/commands/CommandHistory.hpp>
#include <editor/commands/ModifyComponentCommand.hpp>
#include <editor/EditorUI.hpp>

Animation2d::Animation2d(const std::vector<int>& frames, float speed, bool loop, bool flipX, bool flipY)
    : m_frames(frames), m_speed(speed), m_loop(loop),
      m_flipX(flipX), m_flipY(flipY),
      m_currentIndex(0), m_timer(0.0f), m_sprite(nullptr) {}

void Animation2d::Start() {
    // Fetch the renderer attached to the same GameObject
    m_sprite = owner->GetComponent<SpriteSheetRenderer>();
    
    if (!m_sprite) {
        std::cerr << "[Animation2d] Warning: No SpriteSheetRenderer found!" << std::endl;
    } else if (!m_frames.empty()) {
        // Apply the very first frame immediately
        m_sprite->SetFrame(m_frames[0]);

        // Apply flip states immediately
        m_sprite->flipX = m_flipX;
        m_sprite->flipY = m_flipY;
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

        // Ensure flip states are continuously applied
        m_sprite->flipX = m_flipX;
        m_sprite->flipY = m_flipY;
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

#ifndef STANDALONE_MODE
void Animation2d::OnInspector() {
    // Use EditorUI for standard properties
    EditorUI::DragFloat("Speed (s)", &m_speed, 0.01f, this, 0.01f, 5.0f);
    EditorUI::Checkbox("Loop", &m_loop, this);

    // UI Checkboxes for flipping (EditorUI handles Undo/Redo natively)
    EditorUI::Checkbox("Flip X", &m_flipX, this);
    ImGui::SameLine();
    EditorUI::Checkbox("Flip Y", &m_flipY, this);

    // Frame sequence editor (Custom UI)
    std::string framesStr = GetFramesAsString();
    char buffer[256];
    strncpy(buffer, framesStr.c_str(), sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';

    // Static variable to track the state before the user starts typing
    static nlohmann::json initialFramesState;

    // We no longer use ImGuiInputTextFlags_EnterReturnsTrue because we rely on the Deactivated state
    ImGui::InputText("Frames", buffer, sizeof(buffer));

    // UI Lifecycle for Text Input
    if (ImGui::IsItemActivated()) {
        // User clicked inside the text box. Save the current state!
        initialFramesState = Serialize();
    }

    if (ImGui::IsItemDeactivatedAfterEdit()) {
        // User pressed Enter or clicked away. 
        // Apply the new text to the component's internal vector
        ParseFramesFromString(buffer);
        
        // Push the modification to the Undo/Redo stack
        CommandHistory::AddCommand(std::make_unique<ModifyComponentCommand>(this, initialFramesState, Serialize()));
    }
    // Note: If the user presses Escape, ImGui::IsItemDeactivatedAfterEdit() is false.
    // The next frame, 'buffer' will simply be repopulated with the old valid data from GetFramesAsString(). Magic!

    ImGui::TextDisabled("Ex: 0, 1, 2, 3");
    
    // Read-only debug info
    if (m_frames.size() > 0 && m_currentIndex < m_frames.size()) {
        ImGui::Text("Current Frame: %d", m_frames[m_currentIndex]);
    }
}
#endif

nlohmann::json Animation2d::Serialize() const {
    return {
        {"type", "Animation2d"},
        {"frames", m_frames}, // nlohmann::json handles std::vector automatically!
        {"speed", m_speed},
        {"loop", m_loop},
        {"flipX", m_flipX},
        {"flipY", m_flipY}
    };
}

void Animation2d::Deserialize(const nlohmann::json& j) {
    m_speed = j.value("speed", 0.15f);
    m_loop = j.value("loop", true);

    m_flipX = j.value("flipX", false);
    m_flipY = j.value("flipY", false);
    
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
