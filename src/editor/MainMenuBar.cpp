#include "MainMenuBar.hpp"
#include <imgui.h>
#include <iostream>
#include <filesystem>
#include <cctype>
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
                
                std::filesystem::path projectRoot = ProjectSettings::GetProjectRoot();
                std::filesystem::path buildDir = projectRoot / outputDirStr;
                std::filesystem::create_directories(buildDir);
                std::string engineRoot = ProjectSettings::GetEngineRoot().string();

                // Setup initial state for the progress UI
                m_isBuilding = true;
                m_buildProgress = 0;
                m_buildStatusMsg = "Initializing Build...";
                m_openBuildProgressPopup = true; // Trigger the new progress modal

                // Spawn the thread and detach it so it runs independently in the background
                std::thread(&MainMenuBar::RunBuildThread, this, startSceneStr, outputDirStr, projectRoot, engineRoot).detach();
            } 
            else {
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

    if (m_openBuildProgressPopup) {
        ImGui::OpenPopup("Build Progress");
        m_openBuildProgressPopup = false;
    }

    // Modal size is fixed to make sure the console has enough room
    ImGui::SetNextWindowSize(ImVec2(700, 450), ImGuiCond_Appearing);
    if (ImGui::BeginPopupModal("Build Progress", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        // Safely read the current status from the thread
        std::string currentStatus;
        {
            std::lock_guard<std::mutex> lock(m_buildMutex);
            currentStatus = m_buildStatusMsg;
        }
        int progress = m_buildProgress.load();
        bool isBuilding = m_isBuilding.load();

        //  Display the text and the progress bar
        ImGui::TextUnformatted(currentStatus.c_str());
        ImGui::Spacing();

        // Console Output
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.05f, 0.05f, 0.05f, 1.0f)); // Dark background for console
        ImGui::BeginChild("ConsoleLog", ImVec2(0, 280), true, ImGuiWindowFlags_HorizontalScrollbar);

        {
            std::lock_guard<std::mutex> lock(m_buildMutex);
            for (const auto& log : m_buildLogs) {
                // Check if the log contains the word "error" or "warning" to color it
                std::string lowerLog = log;
                std::transform(lowerLog.begin(), lowerLog.end(), lowerLog.begin(), ::tolower);

                if (lowerLog.find("error") != std::string::npos || lowerLog.find("failed") != std::string::npos) {
                    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", log.c_str());
                } else if (lowerLog.find("warning") != std::string::npos) {
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s", log.c_str());
                } else {
                    ImGui::TextUnformatted(log.c_str());
                }
            }

            // Auto-scroll to the bottom if we are actively building
            if (isBuilding && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
                ImGui::SetScrollHereY(1.0f);
            }
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();

        ImGui::Spacing();

        if (progress >= 0) {
            float fraction = progress / 100.0f;
            // Negative width (-1) means "fill the entire available width"
            ImGui::ProgressBar(fraction, ImVec2(-1, 24)); 
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), ICON_FA_TRIANGLE_EXCLAMATION " Build Process Terminated with Errors.");
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Dynamic Close Button
        if (isBuilding) {
            ImGui::BeginDisabled();
            ImGui::Button(ICON_FA_SPINNER " Building...", ImVec2(120, 0));
            ImGui::EndDisabled();
        } else {
            if (ImGui::Button("Close", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
        }
        
        ImGui::EndPopup();
    }
}

void MainMenuBar::RunBuildThread(std::string startSceneStr, std::string outputDirStr, std::filesystem::path projectRoot, std::string engineRoot) {
    std::filesystem::path buildDir = projectRoot / outputDirStr;
    
    {
        std::lock_guard<std::mutex> lock(m_buildMutex);
        m_buildLogs.clear(); // Clear logs from previous builds
        m_buildStatusMsg = "Step 1/3: Configuring CMake...";
    }
    m_buildProgress = 0;
    
    std::string cmakeConfigCmd = "cmake -S \"" + engineRoot + "\" -B \"" + buildDir.string() + "\" -DCMAKE_BUILD_TYPE=Release";
    int configResult = ExecuteCommandWithOutput(cmakeConfigCmd, 0, 10);

    if (configResult == 0) {
        {
            std::lock_guard<std::mutex> lock(m_buildMutex);
            m_buildStatusMsg = "Step 2/3: Compiling Game (This will take a moment)...";
        }
        
        std::string cmakeBuildCmd = "cmake --build \"" + buildDir.string() + "\" --target FoxvoidStandalone --config Release";
        int buildResult = ExecuteCommandWithOutput(cmakeBuildCmd, 10, 90);

        if (buildResult == 0) {
            {
                std::lock_guard<std::mutex> lock(m_buildMutex);
                m_buildStatusMsg = "Step 3/3: Copying Assets...";
            }
            m_buildProgress = 95;

            try {
                std::filesystem::copy(projectRoot / "assets", buildDir / "assets", std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
                std::filesystem::copy_file(projectRoot / "project.json", buildDir / "project.json", std::filesystem::copy_options::overwrite_existing);
                
                {
                    std::lock_guard<std::mutex> lock(m_buildMutex);
                    m_buildStatusMsg = "SUCCESS! Game exported to: " + outputDirStr;
                    m_buildLogs.push_back("--- BUILD FINISHED SUCCESSFULLY ---");
                }
                m_buildProgress = 100;
            } catch(std::filesystem::filesystem_error& e) {
                {
                    std::lock_guard<std::mutex> lock(m_buildMutex);
                    m_buildStatusMsg = "Error copying assets.";
                    m_buildLogs.push_back(std::string("Filesystem Error: ") + e.what());
                }
                m_buildProgress = -1; 
            }
        } else {
            {
                std::lock_guard<std::mutex> lock(m_buildMutex);
                m_buildStatusMsg = "FAILED during compilation.";
            }
            m_buildProgress = -1;
        }
    } else {
        {
            std::lock_guard<std::mutex> lock(m_buildMutex);
            m_buildStatusMsg = "FAILED during CMake configuration.";
        }
        m_buildProgress = -1;
    }

    m_isBuilding = false;
}

int MainMenuBar::ExecuteCommandWithOutput(const std::string& cmd, int baseProgress, int maxProgress) {
    // Append " 2>&1" to redirect standard errors to standard output so we catch everything
    std::string fullCmd = cmd + " 2>&1";
    
    // Open the pipe
    FILE* pipe = popen(fullCmd.c_str(), "r");
    if (!pipe) {
        std::lock_guard<std::mutex> lock(m_buildMutex);
        m_buildLogs.push_back("[Error] Failed to open process pipe.");
        return -1;
    }

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        std::string line(buffer);
        
        // Remove the trailing newline character
        if (!line.empty() && line.back() == '\n') {
            line.pop_back();
        }

        std::lock_guard<std::mutex> lock(m_buildMutex);
        m_buildLogs.push_back(line);

        // Progress Parser
        // Look for the '%' character to extract Ninja/CMake progress
        size_t pctPos = line.find("%");
        if (pctPos != std::string::npos && pctPos > 0) {
            // Backtrack to find the number before the '%'
            size_t start = pctPos - 1;
            while (start > 0 && std::isdigit(line[start])) {
                start--;
            }
            if (!std::isdigit(line[start])) start++; // Move forward to the first digit

            if (start < pctPos) {
                try {
                    int extractedPct = std::stoi(line.substr(start, pctPos - start));
                    // Map the 0-100% of this specific command to our global progress bar
                    float fraction = extractedPct / 100.0f;
                    m_buildProgress = baseProgress + static_cast<int>(fraction * (maxProgress - baseProgress));
                } catch(...) {
                    // Ignore parsing errors
                }
            }
        }
    }

    return pclose(pipe);
}
