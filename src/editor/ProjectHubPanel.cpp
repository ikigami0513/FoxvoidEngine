#include "ProjectHubPanel.hpp"
#include "core/ProjectSettings.hpp"

#ifndef STANDALONE_MODE
#include <imgui.h>
#include <extras/IconsFontAwesome6.h>
#include <portable-file-dialogs.h>
#endif

bool ProjectHubPanel::Draw() {
    bool projectLoaded = false;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    // Use WorkPos/WorkSize to respect OS taskbars
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    
    // Remove global margins & borders
    // This guarantees a true fullscreen background without the 8px default ImGui gap
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    // Add NoDocking Flag
    // Critical when using ImGui docking branch to prevent layout conflicts
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | 
                             ImGuiWindowFlags_NoDocking | 
                             ImGuiWindowFlags_NoMove | 
                             ImGuiWindowFlags_NoSavedSettings | 
                             ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::Begin("Project Hub", nullptr, flags);
    
    // Pop the 3 style vars immediately after Begin() so it doesn't affect child windows
    ImGui::PopStyleVar(3); 

    // Centered Hub Content
    ImVec2 windowSize = ImGui::GetWindowSize();
    float boxWidth = windowSize.x;
    float boxHeight = windowSize.y;
    ImGui::SetCursorPos(ImVec2((windowSize.x - boxWidth) * 0.5f, (windowSize.y - boxHeight) * 0.35f));

    // Restore inner padding for the content box
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(30.0f, 30.0f));
    ImGui::BeginChild("HubContent", ImVec2(boxWidth, boxHeight), true, ImGuiWindowFlags_NoScrollbar);
    ImGui::PopStyleVar();
    
    // Header Title
    ImGui::TextUnformatted(ICON_FA_CUBES " Foxvoid Engine - Project Hub");
    ImGui::Separator();
    ImGui::Spacing();

    float availWidth = ImGui::GetContentRegionAvail().x;
    float browseButtonWidth = 160.0f;
    
    float inputWithButtonWidth = availWidth - browseButtonWidth - ImGui::GetStyle().ItemSpacing.x;

    // Section 1: Create a new project
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), ICON_FA_PLUS " Create New Project");
    ImGui::Spacing();

    ImGui::TextUnformatted("Project Name");
    ImGui::SetNextItemWidth(availWidth);
    ImGui::InputText("##ProjectName", m_newProjectName, sizeof(m_newProjectName));
    
    ImGui::Spacing();

    ImGui::TextUnformatted("Directory");
    // Stretch to max width minus the size of the "Browse" button and the spacing between them
    ImGui::SetNextItemWidth(inputWithButtonWidth);
    ImGui::InputText("##Directory", m_newProjectPath, sizeof(m_newProjectPath));
    
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_FOLDER " Browse...##Create", ImVec2(browseButtonWidth, 0))) {
        auto dir = pfd::select_folder("Choose where to save your new project").result();
        if (!dir.empty()) {
            strncpy(m_newProjectPath, dir.c_str(), sizeof(m_newProjectPath) - 1);
        }
    }

    ImGui::Spacing();
    ImGui::Spacing();

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

    ImGui::TextUnformatted("Project File");
    ImGui::SetNextItemWidth(inputWithButtonWidth);
    ImGui::InputText("##ProjectFile", m_existingProjectPath, sizeof(m_existingProjectPath));
    
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_FOLDER " Browse...##Open", ImVec2(browseButtonWidth, 0))) {
        auto file = pfd::open_file("Select a project.json", ".", {"Foxvoid Project", "project.json"}).result();
        if (!file.empty()) {
            strncpy(m_existingProjectPath, file[0].c_str(), sizeof(m_existingProjectPath) - 1);
        }
    }

    ImGui::Spacing();
    ImGui::Spacing();

    if (ImGui::Button("Open Project", ImVec2(150, 40))) {
        std::string pathStr(m_existingProjectPath);
        if (!pathStr.empty()) {
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
