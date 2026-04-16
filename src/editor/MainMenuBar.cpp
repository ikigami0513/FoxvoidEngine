#include "MainMenuBar.hpp"
#include <imgui.h>
#include <iostream>
#include <filesystem>
#include "commands/CommandHistory.hpp"
#include "core/ProjectSettings.hpp"
#include <portable-file-dialogs.h>
#include <extras/IconsFontAwesome6.h>

void MainMenuBar::Draw(Scene& activeScene, std::string& currentScenePath, bool& isRunning, GameObject*& selectedObject, InputSettingsPanel& inputPanel, GameStatePanel& gameStatePanel, bool& showGlobalGrid) {
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

            if (ImGui::MenuItem("Game State")) {
                gameStatePanel.Open();
            }
            
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Build")) {
            if (ImGui::MenuItem("Build Project...")) {
                m_openBuildPopup = true;
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

    // Trigger the build popup
    if (m_openBuildPopup) {
        ImGui::OpenPopup("Build Project");
        m_openBuildPopup = false;
    }

    // Draw the Build Project modal window
    if (ImGui::BeginPopupModal("Build Project", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted("Configure Standalone Build");
        ImGui::Separator();
        ImGui::Spacing();

        // Select Start Scene
        static char startSceneBuffer[256] = "";
        
        // Pre-fill the buffer with the existing setting if it's currently empty
        if (strlen(startSceneBuffer) == 0) {
            strncpy(startSceneBuffer, ProjectSettings::GetStartScenePath().c_str(), sizeof(startSceneBuffer) - 1);
        }

        ImGui::TextUnformatted("Start Scene (.scene)");
        ImGui::SetNextItemWidth(300.0f);
        ImGui::InputText("##StartScene", startSceneBuffer, sizeof(startSceneBuffer));
        
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_FOLDER " Browse##Scene")) {
            // Open native file dialog starting in the assets folder
            auto file = pfd::open_file("Select starting scene", ProjectSettings::GetAssetsPath().string(), {"Scene Files", "*.scene"}).result();
            if (!file.empty()) {
                // Convert the absolute path to a relative path from the project root
                std::filesystem::path fullPath(file[0]);
                std::filesystem::path relativePath = std::filesystem::relative(fullPath, ProjectSettings::GetProjectRoot());
                strncpy(startSceneBuffer, relativePath.string().c_str(), sizeof(startSceneBuffer) - 1);
            }
        }

        ImGui::Spacing();

        // Select Output Directory
        static char outputDirBuffer[256] = "build_standalone";
        ImGui::TextUnformatted("Output Directory Name");
        ImGui::SetNextItemWidth(300.0f);
        ImGui::InputText("##OutputDir", outputDirBuffer, sizeof(outputDirBuffer));

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Build", ImVec2(120, 0))) {
            std::string startSceneStr(startSceneBuffer);
            std::string outputDirStr(outputDirBuffer);
            
            // Save the chosen start scene into the project configuration
            ProjectSettings::SetStartScenePath(startSceneStr);
            
            if (ProjectSettings::Save()) {
                std::cout << "[Editor] Triggering CMake build process..." << std::endl;
                
                // Define paths for the current project and the output build directory
                std::filesystem::path projectRoot = ProjectSettings::GetProjectRoot();
                std::filesystem::path buildDir = projectRoot / outputDirStr;
                
                // Ensure the build directory exists on the disk
                std::filesystem::create_directories(buildDir);

                // Construct the CMake system commands
                // We retrieve the original Engine path to tell CMake exactly where the source is
                std::string engineRoot = ProjectSettings::GetEngineRoot().string();
                
                // - Config: Sets the source directory to the Engine Root, and outputs to the project's build folder
                std::string cmakeConfigCmd = "cmake -S \"" + engineRoot + "\" -B \"" + buildDir.string() + "\" -DCMAKE_BUILD_TYPE=Release";
                std::string cmakeBuildCmd = "cmake --build \"" + buildDir.string() + "\" --target FoxvoidStandalone --config Release";

                // Execute the commands 
                // Note: std::system is blocking, so the Editor UI will freeze during compilation
                std::cout << "[Build] Configuring: " << cmakeConfigCmd << std::endl;
                int configResult = std::system(cmakeConfigCmd.c_str());

                if (configResult == 0) {
                    std::cout << "[Build] Compiling: " << cmakeBuildCmd << std::endl;
                    int buildResult = std::system(cmakeBuildCmd.c_str());
                    
                    if (buildResult == 0) {
                        std::cout << "[Build] SUCCESS! Game exported to: " << buildDir.string() << std::endl;
                        
                        // Copy the required runtime assets to the build folder
                        // The standalone executable needs the 'assets' folder and 'project.json' to run properly
                        try {
                            std::filesystem::copy(projectRoot / "assets", buildDir / "assets", std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
                            std::filesystem::copy_file(projectRoot / "project.json", buildDir / "project.json", std::filesystem::copy_options::overwrite_existing);
                            std::cout << "[Build] Assets copied successfully." << std::endl;
                        } catch(std::filesystem::filesystem_error& e) {
                            std::cerr << "[Build] Error copying assets: " << e.what() << '\n';
                        }
                    } else {
                        std::cerr << "[Build] FAILED during compilation." << std::endl;
                    }
                } else {
                    std::cerr << "[Build] FAILED during CMake configuration." << std::endl;
                }
            } else {
                std::cerr << "[Editor] Failed to save build configuration." << std::endl;
            }
            
            // Close the modal whether the build succeeded or failed
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
}
