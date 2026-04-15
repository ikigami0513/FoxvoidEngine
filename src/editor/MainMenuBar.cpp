#include "MainMenuBar.hpp"
#include <imgui.h>
#include <iostream>
#include <filesystem>
#include "commands/CommandHistory.hpp"

void MainMenuBar::Draw(Scene& activeScene, std::string& currentScenePath, bool& isRunning, GameObject*& selectedObject, InputSettingsPanel& inputPanel, bool& showGlobalGrid) {
    // Global Shortcuts
    ImGuiIO& io = ImGui::GetIO();

    // Check for Ctrl + S (and make sure we aren't currently typing in a text input box!)
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S) && !io.WantTextInput) {
        if (currentScenePath.empty()) {
            m_openSavePopup = true; // Open the Save As window
        } else {
            activeScene.SaveToFile(currentScenePath);
            std::cout << "[Editor] Scene saved via shortcut to " << currentScenePath << std::endl;
        }
    }

    // Check for Ctrl + Z (QWERTY) or Ctrl + W (AZERTY) for Undo
    if (io.KeyCtrl && (ImGui::IsKeyPressed(ImGuiKey_Z) || ImGui::IsKeyPressed(ImGuiKey_W)) && !io.WantTextInput) {
        CommandHistory::Undo();
    }

    // Check for Ctrl + Y (Redo)
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y) && !io.WantTextInput) {
        CommandHistory::Redo();
    }
    
    // Draw the top menu bar
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            // New scene
            if (ImGui::MenuItem("New Scene")) {
                activeScene.Clear();
                currentScenePath = ""; // Reset the path
                selectedObject = nullptr; // Deselect anything
            }

            // Save scene
            if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
                if (currentScenePath.empty()) {
                    // If it's a new scene, open the "Save As" popup
                    m_openSavePopup = true;
                }
                else {
                    // Otherwise, just overwrite the current file
                    activeScene.SaveToFile(currentScenePath);
                    std::cout << "[Editor] Scene saved to " << currentScenePath << std::endl;
                }
            }

            // Save scene as
            if (ImGui::MenuItem("Save Scene as...")) {
                m_openSavePopup = true;
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            // Undo
            ImGui::BeginDisabled(!CommandHistory::CanUndo());
            if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
                CommandHistory::Undo();
            }
            ImGui::EndDisabled();

            // Redo
            ImGui::BeginDisabled(!CommandHistory::CanRedo());
            if (ImGui::MenuItem("Redo", "Ctrl+Y")) {
                CommandHistory::Redo();
            }
            ImGui::EndDisabled();

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Show Global Grid", nullptr, &showGlobalGrid);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Settings")) {
            if (ImGui::MenuItem("Input Settings")) {
                inputPanel.Open();
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    // Trigger the popup
    if (m_openSavePopup) {
        ImGui::OpenPopup("Save Scene As...");
        m_openSavePopup = false; // Reset flag immediately so it doesn't try to open every frame
    }

    // Draw the Save As modal window
    if (ImGui::BeginPopupModal("Save Scene As...", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        static char nameBuffer[128] = "NewScene";
        ImGui::InputText("Scene Name", nameBuffer, IM_ARRAYSIZE(nameBuffer));

        if (ImGui::Button("Save", ImVec2(120, 0))) {
            std::filesystem::create_directories("assets/scenes");
            
            // Construct the path and force the .scene extension
            currentScenePath = std::string("assets/scenes/") + nameBuffer + ".scene";
            activeScene.SaveToFile(currentScenePath);
            
            std::cout << "[Editor] Scene saved as " << currentScenePath << std::endl;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}
