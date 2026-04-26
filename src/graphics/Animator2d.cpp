#include "Animator2d.hpp"
#include "../world/GameObject.hpp"
#include "SpriteSheetRenderer.hpp"
#include "core/Engine.hpp"
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

void Animator2d::Render() {
    // We only want to force the frame synchronization in the Editor (when the game is stopped).
    // During Play mode, the Update() method naturally handles the animation timing.
    bool isPlaying = true;
    if (Engine::Get()) {
        isPlaying = Engine::Get()->IsPlaying();
    }

    if (!isPlaying && owner) {
        // If an animation is currently selected...
        if (!m_currentAnimation.empty() && m_animations.find(m_currentAnimation) != m_animations.end()) {
            
            if (auto sprite = owner->GetComponent<SpriteSheetRenderer>()) {
                const AnimationData& anim = m_animations[m_currentAnimation];
                
                // Ensure the frame index is valid before applying it
                if (!anim.frames.empty() && m_currentFrameIndex >= 0 && m_currentFrameIndex < anim.frames.size()) {
                    
                    // Instantly sync the visual state so the Editor Game View is perfectly accurate
                    sprite->SetFrame(anim.frames[m_currentFrameIndex]);
                    sprite->flipX = anim.flipX;
                    sprite->flipY = anim.flipY;
                }
            }
        }
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

    // Static variables hold the temporary state of the form inputs.
    // We declare them here so the "Edit" button below can populate them!
    static char nameBuffer[64] = "";
    static char framesBuffer[256] = "";
    static float duration = 0.1f;
    static bool loop = true;
    static bool flipX = false;
    static bool flipY = false;
    static std::vector<int> pickerFrames; // Holds the visual picker's temporary sequence

    bool openPickerModal = false;

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
            if (ImGui::Selectable(animName.c_str(), isCurrent, 0, ImVec2(ImGui::GetContentRegionAvail().x - 65, 0))) {
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

            // Edit Button
            // Loads the selected animation's data into the form at the bottom
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_PEN)) {
                strncpy(nameBuffer, animName.c_str(), sizeof(nameBuffer));
                
                // Convert the frame array back to a comma-separated string
                std::string fStr;
                for (size_t i = 0; i < it->second.frames.size(); ++i) {
                    fStr += std::to_string(it->second.frames[i]);
                    if (i < it->second.frames.size() - 1) fStr += ", ";
                }
                strncpy(framesBuffer, fStr.c_str(), sizeof(framesBuffer));
                
                duration = it->second.frameDuration;
                loop = it->second.loop;
                flipX = it->second.flipX;
                flipY = it->second.flipY;
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Edit this animation");

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

    // Form to add or edit an animation
    // We use TreeNodeEx to keep it open by default
    if (ImGui::TreeNodeEx("Add / Edit Animation", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::InputText("Name", nameBuffer, IM_ARRAYSIZE(nameBuffer));
        
        // Tooltip to explain the expected format
        ImGui::InputText("Frames", framesBuffer, IM_ARRAYSIZE(framesBuffer));
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Enter frame indices separated by commas (e.g., 0, 1, 2, 3)");
        }

        // Visual Picker Button
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_IMAGE " Pick")) {
            // Parse the current buffer text into the visual picker's array
            pickerFrames.clear();
            std::stringstream ss(framesBuffer);
            std::string item;
            while (std::getline(ss, item, ',')) {
                try { pickerFrames.push_back(std::stoi(item)); } catch (...) {}
            }
            openPickerModal = true;
        }

        ImGui::DragFloat("Duration", &duration, 0.01f, 0.01f, 5.0f, "%.2f sec");
        ImGui::Checkbox("Loop", &loop);

        ImGui::Checkbox("Flip X", &flipX);
        ImGui::SameLine();
        ImGui::Checkbox("Flip Y", &flipY);

        if (ImGui::Button("Save Animation", ImVec2(-1, 0))) {
            std::string animName(nameBuffer);
            
            if (!animName.empty()) {
                std::vector<int> parsedFrames;
                std::stringstream ss(framesBuffer);
                std::string item;

                // Parse the comma-separated string into integers
                while (std::getline(ss, item, ',')) {
                    try {
                        parsedFrames.push_back(std::stoi(item));
                    } catch (...) {
                        std::cerr << "[Animator] Invalid frame skipped: " << item << std::endl;
                    }
                }

                if (!parsedFrames.empty()) {
                    nlohmann::json initialState = Serialize();
                    
                    // AddAnimation overwrites existing keys, making it perfect for editing too!
                    AddAnimation(animName, parsedFrames, duration, loop, flipX, flipY);
                    
                    CommandHistory::AddCommand(std::make_unique<ModifyComponentCommand>(this, initialState, Serialize()));

                    // Reset the form buffers for the next entry
                    nameBuffer[0] = '\0';
                    framesBuffer[0] = '\0';
                    duration = 0.1f;
                    loop = true;
                    flipX = false;
                    flipY = false;
                } else {
                    std::cerr << "[Animator] Cannot save animation without frames." << std::endl;
                }
            }
        }
        ImGui::TreePop();
    }

    if (openPickerModal) {
        ImGui::OpenPopup("AnimatorFramePickerModal");
    }

    // Visual frame picker modal
    if (ImGui::BeginPopupModal("AnimatorFramePickerModal", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        SpriteSheetRenderer* sheet = owner->GetComponent<SpriteSheetRenderer>();
        
        if (!sheet || sheet->GetTexture().id == 0) {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), ICON_FA_TRIANGLE_EXCLAMATION " No valid SpriteSheetRenderer found!");
            ImGui::Spacing();
            if (ImGui::Button("Close", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
        } else {
            ImGui::Text("Click frames in order to build your sequence.");
            
            // Build the sequence string for display
            std::string seqStr;
            for(size_t i = 0; i < pickerFrames.size(); ++i) {
                seqStr += std::to_string(pickerFrames[i]) + (i < pickerFrames.size() - 1 ? ", " : "");
            }
            ImGui::TextColored(ImVec4(0.9f, 0.45f, 0.1f, 1.0f), "Sequence: %s", seqStr.empty() ? "Empty" : seqStr.c_str());
            
            if (ImGui::Button(ICON_FA_TRASH " Clear Sequence")) {
                pickerFrames.clear();
            }

            ImGui::Separator();
            ImGui::Spacing();

            Texture2D tex = sheet->GetTexture();
            int cols = sheet->GetColumns();
            int rows = sheet->GetRows();

            float zoom = 2.0f; 
            if (tex.width * zoom > 600) zoom = 600.0f / tex.width; 
            if (zoom < 1.0f) zoom = 1.0f;

            ImVec2 imageSize(tex.width * zoom, tex.height * zoom);
            ImVec2 p = ImGui::GetCursorScreenPos();
            
            ImGui::Image((ImTextureID)(uintptr_t)tex.id, imageSize);
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            
            float cellWidth = imageSize.x / cols;
            float cellHeight = imageSize.y / rows;

            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                ImVec2 mousePos = ImGui::GetMousePos();
                int col = (mousePos.x - p.x) / cellWidth;
                int row = (mousePos.y - p.y) / cellHeight;
                
                if (col >= 0 && col < cols && row >= 0 && row < rows) {
                    int clickedFrame = row * cols + col;
                    pickerFrames.push_back(clickedFrame);
                }
            }

            for (int y = 0; y < rows; ++y) {
                for (int x = 0; x < cols; ++x) {
                    int frameIdx = y * cols + x;
                    ImVec2 cellMin(p.x + x * cellWidth, p.y + y * cellHeight);
                    ImVec2 cellMax(cellMin.x + cellWidth, cellMin.y + cellHeight);

                    drawList->AddRect(cellMin, cellMax, IM_COL32(255, 255, 255, 50));

                    for (size_t i = 0; i < pickerFrames.size(); ++i) {
                        if (pickerFrames[i] == frameIdx) {
                            drawList->AddRectFilled(cellMin, cellMax, IM_COL32(230, 115, 25, 120));
                            std::string stepStr = std::to_string(i + 1);
                            ImVec2 textSize = ImGui::CalcTextSize(stepStr.c_str());
                            ImVec2 textPos(
                                cellMin.x + (cellWidth - textSize.x) * 0.5f,
                                cellMin.y + (cellHeight - textSize.y) * 0.5f
                            );
                            drawList->AddText(ImVec2(textPos.x + 1, textPos.y + 1), IM_COL32(0, 0, 0, 255), stepStr.c_str());
                            drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), stepStr.c_str());
                        }
                    }
                }
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Apply", ImVec2(120, 0))) {
            if (pickerFrames.empty()) pickerFrames.push_back(0); 
            
            // Rebuild the text buffer from our visual selection
            std::string res;
            for(size_t i = 0; i < pickerFrames.size(); ++i) {
                res += std::to_string(pickerFrames[i]) + (i < pickerFrames.size() - 1 ? ", " : "");
            }
            strncpy(framesBuffer, res.c_str(), sizeof(framesBuffer));
            
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
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
