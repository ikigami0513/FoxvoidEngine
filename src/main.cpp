#include "core/Engine.hpp"
#include "core/ProjectSettings.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include "scripting/PythonStubs.hpp"

#ifdef STANDALONE_MODE
#include "core/InputManager.hpp"
#include "core/GameStateManager.hpp"
#include "core/AssetRegistry.hpp"
#include "scripting/ScriptEngine.hpp"
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
        // In Standalone mode, we mount the VFS immediately using the executable's directory
        std::filesystem::path exeDir = std::filesystem::current_path();
        AssetRegistry::MountVFS(exeDir);
        
        // We set the target to project.json, which will now be intercepted by the VFS
        projectPath = "project.json";
#endif
    }

    // Attempt to load the project configuration
    // We removed std::filesystem::exists() because project.json is hidden inside the VFS!
    if (!projectPath.empty()) {
        if (!ProjectSettings::Load(projectPath)) {
            std::cerr << "Failed to load project at: " << projectPath << std::endl;
#ifdef STANDALONE_MODE
            // In Standalone, failing to load the project configuration is a fatal error
            return -1;
#endif
        }
    } else if (argc > 1) {
        std::cerr << "Error: Provided project path is invalid: " << projectPath << std::endl;
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
        // NOUVEAU: The old std::filesystem::current_path change was removed here 
        // to avoid conflicts with the VFS.

        // Initialize the global Asset Registry
        // In Standalone, this will just skip the physical scan since IsPacked() is true
        AssetRegistry::Initialize(ProjectSettings::GetAssetsPath());

        // Register the project's script folder in Python
        // Allows Pybind11 to find and import the user's Python component classes
        ScriptEngine::AddScriptPath(ProjectSettings::GetAssetsPath() / "scripts");

        // Load project specific settings in Standalone BEFORE loading the scene
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
        std::ofstream crashLog("crash.log");
        if (crashLog.is_open()) {
            crashLog << "=== FATAL ERROR ===" << std::endl;
            crashLog << e.what() << std::endl;
            crashLog.close();
        }

        // Catch and display any standard exceptions thrown during execution
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
