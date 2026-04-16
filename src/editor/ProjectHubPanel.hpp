#pragma once

#include <string>

class ProjectHubPanel {
    public:
        // Renders the Hub. Returns true if a project was successfully loaded or created.
        bool Draw();

    private:
        // Buffers for ImGui text inputs
        char m_newProjectPath[256] = "";
        char m_newProjectName[64] = "MyNewGame";
        
        char m_existingProjectPath[256] = "";
        
        // Holds error messages to display in the UI
        std::string m_errorMessage = "";
};
