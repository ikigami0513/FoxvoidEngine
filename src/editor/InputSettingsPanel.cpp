#include "InputSettingsPanel.hpp"
#include <imgui.h>
#include <raylib.h>
#include "core/InputManager.hpp"
#include "extras/IconsFontAwesome6.h"

void InputSettingsPanel::Draw() {
    // Trigger the opening of the popup
    if (m_openRequested) {
        ImGui::OpenPopup("Input Settings");
        m_isOpen = true; // Set internal state
        m_openRequested = false; // Reset trigger
    }

    // Automatically center the modal on the screen when it appears
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    // Set a default size (Width: 800px, Height: 600px)
    // Using ImGuiCond_Appearing ensures it only forces this size the first time it opens
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_Appearing);

    // ImGuiWindowFlags_NoSavedSettings prevents ImGui from writing this window's size to imgui.ini
    if (ImGui::BeginPopupModal("Input Settings", &m_isOpen, ImGuiWindowFlags_NoSavedSettings)) {
        auto& bindings = InputManager::GetBindings();

        // Top bar: Save and Load buttons
        if (ImGui::Button(ICON_FA_FLOPPY_DISK " Save")) {
            InputManager::Save("assets/settings/inputs.json");
        }
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_FOLDER_OPEN " Load")) {
            InputManager::Load("assets/settings/inputs.json");
        }

        ImGui::Separator();
        ImGui::Spacing();

        // Add New Action row
        ImGui::InputText("##NewAction", m_newActionBuffer, IM_ARRAYSIZE(m_newActionBuffer));
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_PLUS " Add Action")) {
            std::string newAction(m_newActionBuffer);
            if (!newAction.empty() && bindings.find(newAction) == bindings.end()) {
                // Add an empty list of keys for this new action
                bindings[newAction] = std::vector<int>();
                m_newActionBuffer[0] = '\0'; // Clear the buffer

                // Auto-save on structural change
                InputManager::Save("assets/settings/inputs.json");
            }
        }

        ImGui::Spacing();
        ImGui::Separator();

        // Display existing actions
        for (auto it = bindings.begin(); it != bindings.end(); ) {
            const std::string& action = it->first;
            auto& keys = it->second;

            // Calculate dynamic height based on content
            // Base height (header + add button + padding) is roughly 85 pixels
            // Each bound key adds about 50 pixels of height
            float dynamicHeight = 85.0f + (keys.size() * 50.0f);

            // Draw a slightly darker background for each action block
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
            ImGui::BeginChild(action.c_str(), ImVec2(0, dynamicHeight), true);

            // Action Header (Name + Delete Action Button)
            ImGui::TextDisabled("%s", ICON_FA_GAMEPAD);
            ImGui::SameLine();
            ImGui::TextUnformatted(action.c_str());
            ImGui::SameLine(ImGui::GetWindowWidth() - 40);
            
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f)); // Red button
            if (ImGui::Button((std::string(ICON_FA_TRASH) + "##" + action).c_str())) {
                // Ask the iterator to erase and move to the next item
                it = bindings.erase(it);
                ImGui::PopStyleColor();
                ImGui::EndChild();
                ImGui::PopStyleColor();
                continue; // Skip the rest of the drawing for this deleted item
            }
            ImGui::PopStyleColor();

            // Display bound keys
            ImGui::Indent();
            for (size_t i = 0; i < keys.size(); ++i) {
                int key = keys[i];
                ImGui::Text("- %s", GetKeyName(key).c_str());
                ImGui::SameLine(ImGui::GetWindowWidth() - 40);
                
                // Small 'x' button to unbind a specific key
                if (ImGui::Button((std::string(ICON_FA_XMARK) + "##" + action + std::to_string(key)).c_str())) {
                    keys.erase(keys.begin() + i);
                    break; // Break to avoid iterator issues, it will refresh next frame
                }
            }
            ImGui::Unindent();

            ImGui::Spacing();

            // Add Key Mechanism (The "Listening" state)
            if (m_isListening && m_listeningAction == action) {
                ImGui::Button("Press any key...", ImVec2(150, 0));
                
                // Raylib's GetKeyPressed returns 0 if nothing is pressed
                int pressedKey = GetKeyPressed();
                if (pressedKey > 0) {
                    InputManager::BindKey(action, pressedKey);
                    m_isListening = false;
                }
            } else {
                if (ImGui::Button((std::string(ICON_FA_PLUS) + " Add Key##" + action).c_str())) {
                    m_isListening = true;
                    m_listeningAction = action;
                }
            }

            ImGui::EndChild();
            ImGui::PopStyleColor();

            ++it; // Move to next action
        }

        ImGui::EndPopup();
    }
}

std::string InputSettingsPanel::GetKeyName(int key) const {
    if (key >= KEY_A && key <= KEY_Z) return std::string(1, (char)key);

    if (key == KEY_SPACE) return "Space";
    if (key == KEY_ENTER) return "Enter";
    if (key == KEY_LEFT) return "Left Arrow";
    if (key == KEY_RIGHT) return "Right Arrow";
    if (key == KEY_UP) return "Up Arrow";
    if (key == KEY_DOWN) return "Down Arrow";
    if (key == KEY_ESCAPE) return "Escape";
    if (key == KEY_LEFT_SHIFT || key == KEY_RIGHT_SHIFT) return "Shift";
    if (key == KEY_LEFT_CONTROL || key == KEY_RIGHT_CONTROL) return "Control";

    return "Keycode: " + std::to_string(key);
}
