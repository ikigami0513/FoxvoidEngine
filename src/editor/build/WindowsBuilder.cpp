#include "WindowsBuilder.hpp"
#include "Build.hpp"
#include <vector>
#include <filesystem>

bool WindowsBuilder::Configure(const std::string& buildDir, const std::string& engineRoot) {
    std::string cmakeConfigCmd = "cmake -S \"" + engineRoot + "\" -B \"" + buildDir + "\" -DCMAKE_BUILD_TYPE=Release";

#if defined(__linux__) || defined(__APPLE__)
    // HOST is Unix, TARGET is Windows -> We MUST cross-compile using MinGW
    Build::LogMessage("Host is Unix. Cross-compiling for Windows using MinGW...");
    
    std::filesystem::path pyWinDir = std::filesystem::path(engineRoot) / "vendor" / "python_win";
    std::filesystem::path pyWinToolsDir = pyWinDir / "tools";
    
    // Automatic dependency resolution
    // Check if the Windows Python headers and libs are missing
    if (!std::filesystem::exists(pyWinToolsDir / "include") || !std::filesystem::exists(pyWinToolsDir / "vcruntime140.dll")) {
        Build::LogWarning("Windows Python dependencies are missing. Starting automatic download...");
        
        std::filesystem::create_directories(pyWinDir);
        
        // 1. Download NuGet Package (Contains the .lib and headers for compiling)
        Build::LogMessage("Downloading Python 3.11 Dev Package...");
        std::string cmdDownloadDev = "wget -q https://www.nuget.org/api/v2/package/python/3.11.9 -O \"" + (pyWinDir / "python_dev.zip").string() + "\"";
        if (Build::ExecuteCommandWithOutput(cmdDownloadDev, 0, 2) != 0) {
            Build::LogError("Failed to download Python Dev Package.");
            return false;
        }

        // 2. Download Embeddable Package (Contains the Microsoft VCRuntime DLLs for distribution)
        Build::LogMessage("Downloading Python 3.11 Embeddable Package...");
        std::string cmdDownloadEmbed = "wget -q https://www.python.org/ftp/python/3.11.9/python-3.11.9-embed-amd64.zip -O \"" + (pyWinDir / "python_embed.zip").string() + "\"";
        if (Build::ExecuteCommandWithOutput(cmdDownloadEmbed, 2, 4) != 0) {
            Build::LogError("Failed to download Python Embeddable Package.");
            return false;
        }

        // 3. Extract the Dev Package
        Build::LogMessage("Extracting dependencies (This may take a few seconds)...");
        std::string cmdUnzipDev = "unzip -q -o \"" + (pyWinDir / "python_dev.zip").string() + "\" -d \"" + pyWinDir.string() + "\"";
        Build::ExecuteCommandWithOutput(cmdUnzipDev, 4, 6);

        // 4. Extract ONLY the required DLLs from the Embeddable Package directly into the tools folder
        std::string cmdUnzipEmbed = "unzip -q -o \"" + (pyWinDir / "python_embed.zip").string() + "\" vcruntime140.dll vcruntime140_1.dll -d \"" + pyWinToolsDir.string() + "\"";
        Build::ExecuteCommandWithOutput(cmdUnzipEmbed, 6, 8);

        // 5. Cleanup the zip files to save space
        std::filesystem::remove(pyWinDir / "python_dev.zip");
        std::filesystem::remove(pyWinDir / "python_embed.zip");
        
        Build::LogMessage("Successfully downloaded and extracted Windows dependencies.");
    }

    cmakeConfigCmd += " -DCMAKE_TOOLCHAIN_FILE=\"" + engineRoot + "/cmake/mingw-toolchain.cmake\"";
    
    // Force Pybind11 to use the downloaded Windows Python binaries
    cmakeConfigCmd += " -DPYBIND11_FINDPYTHON=OFF";
    cmakeConfigCmd += " -DPYTHON_INCLUDE_DIR=\"" + pyWinToolsDir.string() + "/include\"";
    cmakeConfigCmd += " -DPYTHON_LIBRARY=\"" + pyWinToolsDir.string() + "/libs/python311.lib\"";
    cmakeConfigCmd += " -DPYTHON_MODULE_EXTENSION=\".pyd\"";

    cmakeConfigCmd += " -DPython3_ROOT_DIR=\"" + pyWinToolsDir.string() + "\"";
    cmakeConfigCmd += " -DPython3_INCLUDE_DIR=\"" + pyWinToolsDir.string() + "/include\"";
    cmakeConfigCmd += " -DPython3_LIBRARY=\"" + pyWinToolsDir.string() + "/libs/python311.lib\""; 
    
    cmakeConfigCmd += " -DPython3_FIND_REGISTRY=NEVER";
    cmakeConfigCmd += " -DPython3_FIND_STRATEGY=LOCATION";

#elif defined(_WIN32)
    // HOST is Windows, TARGET is Windows -> Native Compilation
    Build::LogMessage("Host is Windows. Using native compilation...");
    // Native Windows compilation will use the system's Python installation via CMake's FindPython3.
#endif

    // Execute CMake Config (giving it the progress bar range 10% to 20% since download took 0-8%)
    return Build::ExecuteCommandWithOutput(cmakeConfigCmd, 10, 20) == 0;
}

bool WindowsBuilder::Compile(const std::string& buildDir) {
    std::string cmakeBuildCmd = "cmake --build \"" + buildDir + "\" --target FoxvoidStandalone --config Release";
    return Build::ExecuteCommandWithOutput(cmakeBuildCmd, 20, 90) == 0;
}

bool WindowsBuilder::CopyDependencies(const std::filesystem::path& buildDir, const std::string& engineRoot, ScreenOrientation orientation) {
    // List of required external DLLs for Windows
    std::vector<std::string> dllsToCopy = {
        "python311.dll",
        "vcruntime140.dll",
        "vcruntime140_1.dll"
    };

    bool success = true;
    for (const auto& dllName : dllsToCopy) {
        std::filesystem::path dllSource = std::filesystem::path(engineRoot) / "vendor" / "python_win" / "tools" / dllName;
        std::filesystem::path dllDest = buildDir / dllName;
        
        if (std::filesystem::exists(dllSource)) {
            std::filesystem::copy_file(dllSource, dllDest, std::filesystem::copy_options::overwrite_existing);
        } else {
            Build::LogWarning(dllName + " not found in vendor folder! The game may crash on Windows.");
            success = false;
        }
    }
    return success;
}
