#include "Animator2d.hpp"
#include "../world/GameObject.hpp"
#include "SpriteSheetRenderer.hpp"
#include <iostream>
#include <sstream>

#ifndef STANDALONE_MODE
#include <editor/commands/CommandHistory.hpp>
#include <editor/commands/ModifyComponentCommand.hpp>
#include <extras/IconsFontAwesome6.h>
#endif

Animator2d::Animator2d() 
    : m_currentFrameIndex(0), 
      m_timer(0.0f), 
      m_currentAnimation(""), 
      m_spriteRenderer(nullptr) 
{
}

void Animator2d::AddAnimation(const std::string& name, const std::vector<int>& frames, float frameDuration, bool loop, bool flipX, bool flipY) {
    m_animations[name] = { frames, frameDuration, loop, flipX, flipY };
}

void Animator2d::Play(const std::string& name) {
    // Prevent restarting the same animation if it's already playing
    if (m_currentAnimation == name) {
        return;
    }

    // Check if the animation exists in our map
    if (m_animations.find(name) != m_animations.end()) {
        m_currentAnimation = name;
        m_currentFrameIndex = 0;
        m_timer = 0.0f;
        
        // Immediately update the sprite to show the first frame of the new animation
        UpdateSprite();
    } else {
        std::cerr << "[Animator] Warning: Animation '" << name << "' not found!" << std::endl;
    }
}

void Animator2d::Update(float deltaTime) {
    // Do nothing if no animation is currently set
    if (m_currentAnimation.empty()) {
        return;
    }

    // Lazily fetch the SpriteSheetRenderer if we don't have it yet
    if (!m_spriteRenderer) {
        // We use the owner (GameObject) to find the sibling component
        m_spriteRenderer = owner->GetComponent<SpriteSheetRenderer>();
        
        // If the GameObject doesn't have a SpriteSheetRenderer, we can't animate
        if (!m_spriteRenderer) {
            return; 
        }
    }

    AnimationData& anim = m_animations[m_currentAnimation];
    
    // Safety check: ensure the animation actually has frames
    if (anim.frames.empty()) {
        return;
    }

    m_timer += deltaTime;

    // If enough time has passed, move to the next frame
    if (m_timer >= anim.frameDuration) {
        // Subtract the duration to keep the remainder for precise timing (smooth animations)
        m_timer -= anim.frameDuration; 
        m_currentFrameIndex++;

        // Check if we reached the end of the animation sequence
        if (m_currentFrameIndex >= anim.frames.size()) {
            if (anim.loop) {
                // Loop back to the first frame
                m_currentFrameIndex = 0; 
            } else {
                // Stay clamped on the last frame
                m_currentFrameIndex = anim.frames.size() - 1; 
            }
        }

        UpdateSprite();
    }
}

void Animator2d::UpdateSprite() {
    // Ensure we have a valid renderer and a valid animation playing
    if (m_spriteRenderer && !m_currentAnimation.empty()) {
        AnimationData& anim = m_animations[m_currentAnimation];
        
        if (!anim.frames.empty()) {
            // Apply the actual frame index to the SpriteSheetRenderer
            m_spriteRenderer->SetFrame(anim.frames[m_currentFrameIndex]);
        
            m_spriteRenderer->flipX = anim.flipX;
            m_spriteRenderer->flipY = anim.flipY;
        }
    }
}

// Returns the name of the component for the engine's reflection/UI systems
std::string Animator2d::GetName() const {
    return "Animator 2d";
}

#ifndef STANDALONE_MODE
void Animator2d::OnInspector() {
    // Read-only properties do not need Undo/Redo tracking
    // Display current state
    ImGui::Text("Current Animation: %s", m_currentAnimation.empty() ? "None" : m_currentAnimation.c_str());
    ImGui::Text("Frame Index: %d", m_currentFrameIndex);
    ImGui::Text("Timer: %.3f", m_timer);

    ImGui::Separator();

    // Display a list of all registered animations
    if (ImGui::TreeNode("Registered Animations")) {
        // We use an iterator to safely erase elements while looping through the map
        for (auto it = m_animations.begin(); it != m_animations.end(); ) {
            const std::string& animName = it->first;
            bool isCurrent = (m_currentAnimation == animName);
            
            // Push the ID for this specific animation row
            ImGui::PushID(animName.c_str());

            // Create a selectable list item for each animation
            // Clicking it will force the Animator to play this animation
            if (ImGui::Selectable(animName.c_str(), isCurrent, 0, ImVec2(ImGui::GetContentRegionAvail().x - 30, 0))) {
                // Capture the state BEFORE changing the animation
                nlohmann::json initialState = Serialize();
                
                // Apply the change
                Play(animName);
                
                // Push the modification to the Undo/Redo stack immediately
                CommandHistory::AddCommand(std::make_unique<ModifyComponentCommand>(this, initialState, Serialize()));
            }
            
            // Show a small tooltip with animation details when hovering
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Frames: %zu | Duration: %.2fs | Loop: %s | Flip: %s%s", 
                                  it->second.frames.size(), 
                                  it->second.frameDuration, 
                                  it->second.loop ? "Yes" : "No",
                                  it->second.flipX ? "X " : "",
                                  it->second.flipY ? "Y" : (!it->second.flipX ? "None" : ""));
            }

            // Deletion: Trash icon button at the end of the line
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f)); // Red tint

            if (ImGui::Button(ICON_FA_TRASH)) {
                nlohmann::json initialState = Serialize();

                // If the deleted animation was the one playing, we reset the state
                if (isCurrent) {
                    m_currentAnimation = "";
                    m_currentFrameIndex = 0;
                    m_timer = 0.0f;
                }

                // Erase from map and update iterator
                it = m_animations.erase(it);

                // Commit the change to Undo/Redo history
                CommandHistory::AddCommand(std::make_unique<ModifyComponentCommand>(this, initialState, Serialize()));
                
                ImGui::PopStyleColor();
                ImGui::PopID();
                continue; // Skip the rest and go to next iteration
            }
            ImGui::PopStyleColor();

            ImGui::PopID();
            ++it;
        }
        ImGui::TreePop();
    }

    ImGui::Separator();

    // Form to add a new animation
    if (ImGui::TreeNode("Add New Animation")) {
        // Static variables hold the temporary state of the form inputs
        static char nameBuffer[64] = "";
        static char framesBuffer[256] = "";
        static float duration = 0.1f;
        static bool loop = true;
        static bool flipX = false;
        static bool flipY = false;

        ImGui::InputText("Name", nameBuffer, IM_ARRAYSIZE(nameBuffer));
        
        // Tooltip to explain the expected format
        ImGui::InputText("Frames", framesBuffer, IM_ARRAYSIZE(framesBuffer));
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Enter frame indices separated by commas (e.g., 0, 1, 2, 3)");
        }

        ImGui::DragFloat("Duration", &duration, 0.01f, 0.01f, 5.0f, "%.2f sec");
        ImGui::Checkbox("Loop", &loop);

        ImGui::Checkbox("Flip X", &flipX);
        ImGui::SameLine();
        ImGui::Checkbox("Flip Y", &flipY);

        if (ImGui::Button("Create Animation", ImVec2(-1, 0))) {
            std::string animName(nameBuffer);
            
            if (!animName.empty()) {
                std::vector<int> parsedFrames;
                std::stringstream ss(framesBuffer);
                std::string item;

                // Parse the comma-separated string into integers
                while (std::getline(ss, item, ',')) {
                    try {
                        // std::stoi converts string to integer, ignoring surrounding whitespaces
                        parsedFrames.push_back(std::stoi(item));
                    } catch (...) {
                        // Ignore any invalid inputs (like letters or extra commas)
                        std::cerr << "[Animator] Invalid frame skipped: " << item << std::endl;
                    }
                }

                // Only create the animation if at least one valid frame was provided
                if (!parsedFrames.empty()) {
                    nlohmann::json initialState = Serialize();
                    
                    AddAnimation(animName, parsedFrames, duration, loop, flipX, flipY);
                    
                    CommandHistory::AddCommand(std::make_unique<ModifyComponentCommand>(this, initialState, Serialize()));

                    // Reset the form buffers for the next entry
                    nameBuffer[0] = '\0';
                    framesBuffer[0] = '\0';
                    duration = 0.1f;
                    loop = true;
                } else {
                    std::cerr << "[Animator] Cannot create animation without frames." << std::endl;
                }
            }
        }
        ImGui::TreePop();
    }
}
#endif

// Converts the component's data into a JSON object for saving scenes
nlohmann::json Animator2d::Serialize() const {
    nlohmann::json j;

    j["type"] = "Animator2d";
    
    // Save current playback state
    j["currentAnimation"] = m_currentAnimation;
    j["currentFrameIndex"] = m_currentFrameIndex;
    j["timer"] = m_timer;

    // Save all registered animations
    nlohmann::json animsJson;
    for (const auto& pair : m_animations) {
        nlohmann::json animData;
        animData["frames"] = pair.second.frames;
        animData["frameDuration"] = pair.second.frameDuration;
        animData["loop"] = pair.second.loop;
        animData["flipX"] = pair.second.flipX;
        animData["flipY"] = pair.second.flipY;
        
        // Use the animation name as the key in the JSON object
        animsJson[pair.first] = animData;
    }
    
    j["animations"] = animsJson;

    return j;
}

// Restores the component's data from a JSON object when loading scenes
void Animator2d::Deserialize(const nlohmann::json& j) {
    // Safely load the playback state
    if (j.contains("currentAnimation")) m_currentAnimation = j["currentAnimation"].get<std::string>();
    if (j.contains("currentFrameIndex")) m_currentFrameIndex = j["currentFrameIndex"].get<int>();
    if (j.contains("timer")) m_timer = j["timer"].get<float>();

    // Safely load all registered animations
    if (j.contains("animations")) {
        m_animations.clear();
        
        // Iterate through the JSON object containing the animations
        for (auto it = j["animations"].begin(); it != j["animations"].end(); ++it) {
            AnimationData data;
            data.frames = it.value()["frames"].get<std::vector<int>>();
            data.frameDuration = it.value()["frameDuration"].get<float>();
            data.loop = it.value()["loop"].get<bool>();
            data.flipX = it.value().value("flipX", false);
            data.flipY = it.value().value("flipY", false);

            // Re-insert into our unordered_map using the JSON key as the animation name
            m_animations[it.key()] = data;
        }
    }
}
