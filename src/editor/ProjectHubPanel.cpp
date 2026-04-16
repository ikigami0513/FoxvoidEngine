#include "ProjectHubPanel.hpp"
#include "core/ProjectSettings.hpp"
#include <imgui.h>
#include <extras/IconsFontAwesome6.h>
#include <portable-file-dialogs.h>

bool ProjectHubPanel::Draw() {
    bool projectLoaded = false;

    // Force the window to take up the entire screen (Fullscreen Modal behavior)
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    
    // Remove all decorations (title bar, borders, resize handles)
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | 
                             ImGuiWindowFlags_NoMove | 
                             ImGuiWindowFlags_NoSavedSettings | 
                             ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::Begin("Project Hub", nullptr, flags);

    // Center the content box in the middle of the screen
    ImVec2 windowSize = ImGui::GetWindowSize();
    float boxWidth = 700.0f;
    ImGui::SetCursorPos(ImVec2((windowSize.x - boxWidth) * 0.5f, windowSize.y * 0.2f));

    // Create an inner child window to hold the form
    ImGui::BeginChild("HubContent", ImVec2(boxWidth, 400.0f), true, ImGuiWindowFlags_NoScrollbar);
    
    // Header Title
    ImGui::TextUnformatted(ICON_FA_CUBES " Foxvoid Engine - Project Hub");
    ImGui::Separator();
    ImGui::Spacing();

    // Section 1: Create a new project
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), ICON_FA_PLUS " Create New Project");
    ImGui::Spacing();

    ImGui::InputText("Project Name", m_newProjectName, sizeof(m_newProjectName));
    
    // Input for the path with a button next to it to open the native folder browser
    ImGui::SetNextItemWidth(450.0f);
    ImGui::InputText("Directory", m_newProjectPath, sizeof(m_newProjectPath));
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_FOLDER " Browse...##Create")) {
        // Open the native OS folder selection dialog
        auto dir = pfd::select_folder("Choose where to save your new project").result();
        if (!dir.empty()) {
            // Copy the selected path back into the ImGui buffer
            strncpy(m_newProjectPath, dir.c_str(), sizeof(m_newProjectPath) - 1);
        }
    }

    if (ImGui::Button("Create Project", ImVec2(150, 40))) {
        std::string pathStr(m_newProjectPath);
        std::string nameStr(m_newProjectName);
        
        if (!pathStr.empty() && !nameStr.empty()) {
            if (ProjectSettings::CreateNewProject(pathStr, nameStr)) {
                projectLoaded = true;
            } else {
                m_errorMessage = "Failed to create project. Check path and permissions.";
            }
        } else {
            m_errorMessage = "Path and Name cannot be empty.";
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Section 2: Open an existing project
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.8f, 1.0f), ICON_FA_FOLDER_OPEN " Open Existing Project");
    ImGui::Spacing();

    ImGui::SetNextItemWidth(450.0f);
    ImGui::InputText("Project File", m_existingProjectPath, sizeof(m_existingProjectPath));
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_FOLDER " Browse...##Open")) {
        // Open the native OS file selection dialog (filtering for project.json files)
        auto file = pfd::open_file("Select a project.json", ".", {"Foxvoid Project", "project.json"}).result();
        if (!file.empty()) {
            strncpy(m_existingProjectPath, file[0].c_str(), sizeof(m_existingProjectPath) - 1);
        }
    }

    if (ImGui::Button("Open Project", ImVec2(150, 40))) {
        std::string pathStr(m_existingProjectPath);
        if (!pathStr.empty()) {
            // If the user selected a folder instead of the file, automatically append the filename
            std::filesystem::path p(pathStr);
            if (std::filesystem::is_directory(p)) {
                p /= "project.json";
            }

            if (ProjectSettings::Load(p)) {
                projectLoaded = true;
            } else {
                m_errorMessage = "Could not find or load project.json at this location.";
            }
        }
    }

    // Display error messages if any occurred
    if (!m_errorMessage.empty()) {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.0f), ICON_FA_TRIANGLE_EXCLAMATION " %s", m_errorMessage.c_str());
    }

    ImGui::EndChild();
    ImGui::End();

    return projectLoaded;
}
