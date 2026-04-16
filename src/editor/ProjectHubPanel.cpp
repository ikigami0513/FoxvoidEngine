#include "ProjectHubPanel.hpp"
#include "core/ProjectSettings.hpp"

#ifndef STANDALONE_MODE
#include <imgui.h>
#include <extras/IconsFontAwesome6.h>
#include <portable-file-dialogs.h>
#endif

bool ProjectHubPanel::Draw() {
    bool projectLoaded = false;

#ifndef STANDALONE_MODE
    // Async PFD Handlers
    // Check if the folder creation dialog is ready
    if (m_folderDialog && m_folderDialog->ready()) {
        auto dir = m_folderDialog->result();
        if (!dir.empty()) {
            strncpy(m_newProjectPath, dir.c_str(), sizeof(m_newProjectPath) - 1);
        }
        m_folderDialog.reset(); // Destroy the dialog object to free memory
    }

    // Check if the open project dialog is ready
    if (m_fileDialog && m_fileDialog->ready()) {
        auto file = m_fileDialog->result();
        if (!file.empty()) {
            strncpy(m_existingProjectPath, file[0].c_str(), sizeof(m_existingProjectPath) - 1);
        }
        m_fileDialog.reset(); // Destroy the dialog object
    }

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    // Use WorkPos/WorkSize to respect OS taskbars
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    
    // Remove global margins & borders
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | 
                             ImGuiWindowFlags_NoDocking | 
                             ImGuiWindowFlags_NoMove | 
                             ImGuiWindowFlags_NoSavedSettings | 
                             ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::Begin("Project Hub", nullptr, flags);
    ImGui::PopStyleVar(3); 

    // Centered Hub Content
    ImVec2 windowSize = ImGui::GetWindowSize();
    float boxWidth = windowSize.x;
    float boxHeight = windowSize.y;
    ImGui::SetCursorPos(ImVec2((windowSize.x - boxWidth) * 0.5f, (windowSize.y - boxHeight) * 0.35f));

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
    ImGui::SetNextItemWidth(inputWithButtonWidth);
    ImGui::InputText("##Directory", m_newProjectPath, sizeof(m_newProjectPath));
    
    ImGui::SameLine();
    
    // Async Folder Browse Button
    if (m_folderDialog) {
        ImGui::BeginDisabled();
        ImGui::Button(ICON_FA_SPINNER " Browsing...", ImVec2(browseButtonWidth, 0));
        ImGui::EndDisabled();
    } else {
        if (ImGui::Button(ICON_FA_FOLDER " Browse...##Create", ImVec2(browseButtonWidth, 0))) {
            // Instantiate the dialog but DO NOT call .result() yet
            m_folderDialog = std::make_shared<pfd::select_folder>("Choose where to save your new project");
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
    
    // Async File Browse Button
    if (m_fileDialog) {
        ImGui::BeginDisabled();
        ImGui::Button(ICON_FA_SPINNER " Browsing...", ImVec2(browseButtonWidth, 0));
        ImGui::EndDisabled();
    } else {
        if (ImGui::Button(ICON_FA_FOLDER " Browse...##Open", ImVec2(browseButtonWidth, 0))) {
            // Provide the expected file filters
            std::vector<std::string> filters = {"Foxvoid Project", "project.json"};
            m_fileDialog = std::make_shared<pfd::open_file>("Select a project.json", ".", filters);
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
#endif

    return projectLoaded;
}
