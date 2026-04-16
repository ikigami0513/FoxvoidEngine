#include "core/Engine.hpp"
#include "core/ProjectSettings.hpp"
#include <iostream>
#include <filesystem>

#ifdef STANDALONE_MODE
#include "core/InputManager.hpp"
#include "core/GameStateManager.hpp"
#endif

int main(int argc, char** argv) {
    // Save the original working directory (which contains the engine's CMakeLists.txt)
    // before the editor changes it to the project's directory later on.
    ProjectSettings::SetEngineRoot(std::filesystem::current_path());

    std::filesystem::path projectPath;

    // Determine the project path
    if (argc > 1) {
        // If passed via command line (useful for debugging or shortcuts)
        projectPath = argv[1];
        if (std::filesystem::is_directory(projectPath)) {
            projectPath /= "project.json";
        }
    } else {
#ifndef STANDALONE_MODE
        // In Editor mode, if no arguments are passed, we leave the path empty.
        // The Engine will boot and the Editor will show the Project Hub.
#else
        // In Standalone mode, the built executable expects the project.json 
        // to be in the exact same directory as the executable itself.
        projectPath = "project.json";
#endif
    }

    // Attempt to load the project configuration
    if (!projectPath.empty() && std::filesystem::exists(projectPath)) {
        if (!ProjectSettings::Load(projectPath)) {
            std::cerr << "Failed to load project at: " << projectPath << std::endl;
#ifdef STANDALONE_MODE
            // In Standalone, failing to load the project configuration is a fatal error
            return -1;
#endif
        }
    } else if (argc > 1) {
        std::cerr << "Error: Provided project path does not exist: " << projectPath << std::endl;
    }

    try {
        // Initialize the Engine 
        // Uses loaded settings, or fallback defaults if in the Editor Hub
        Engine engine(
            ProjectSettings::GetWindowWidth(), 
            ProjectSettings::GetWindowHeight(), 
            ProjectSettings::GetProjectName()
        );

#ifdef STANDALONE_MODE
        // Load project specific settings in Standalone BEFORE loading the scene ---
        std::filesystem::path settingsPath = ProjectSettings::GetAssetsPath() / "settings";
        InputManager::Load((settingsPath / "inputs.json").string());
        GameStateManager::Load((settingsPath / "globals.json").string());
        
        std::cout << "[Standalone] Project settings loaded." << std::endl;

        // In Standalone Mode, we bypass the editor entirely and load the starting scene
        std::string startScene = ProjectSettings::GetStartScenePath();
        if (!startScene.empty()) {
            engine.LoadScene(startScene);
            // We force a flush/start here to ensure the scene is ready on frame 1
            engine.GetActiveScene().Start(); 
        } else {
            std::cerr << "WARNING: No Start Scene defined in project.json!" << std::endl;
        }
#endif

        // 5. Start the main game loop
        engine.Run();
        
    } catch (const std::exception& e) {
        // Catch and display any standard exceptions thrown during execution
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
