#include "WindowsBuilder.hpp"
#include "Build.hpp"
#include <vector>

bool WindowsBuilder::Configure(const std::string& buildDir, const std::string& engineRoot) {
    std::string cmakeConfigCmd = "cmake -S \"" + engineRoot + "\" -B \"" + buildDir + "\" -DCMAKE_BUILD_TYPE=Release";

#if defined(__linux__) || defined(__APPLE__)
    // HOST is Linux/Mac, TARGET is Windows -> We MUST cross-compile using MinGW
    Build::LogMessage("Host is Unix. Cross-compiling for Windows using MinGW...");
    
    cmakeConfigCmd += " -DCMAKE_TOOLCHAIN_FILE=\"" + engineRoot + "/cmake/mingw-toolchain.cmake\"";
    
    std::string pyWinDir = engineRoot + "/vendor/python_win/tools";
    
    // Force Pybind11 to use the downloaded Windows Python binaries
    cmakeConfigCmd += " -DPYBIND11_FINDPYTHON=OFF";
    cmakeConfigCmd += " -DPYTHON_INCLUDE_DIR=\"" + pyWinDir + "/include\"";
    cmakeConfigCmd += " -DPYTHON_LIBRARY=\"" + pyWinDir + "/libs/python311.lib\"";
    cmakeConfigCmd += " -DPYTHON_MODULE_EXTENSION=\".pyd\"";

    cmakeConfigCmd += " -DPython3_ROOT_DIR=\"" + pyWinDir + "\"";
    cmakeConfigCmd += " -DPython3_INCLUDE_DIR=\"" + pyWinDir + "/include\"";
    cmakeConfigCmd += " -DPython3_LIBRARY=\"" + pyWinDir + "/libs/python311.lib\""; 
    
    cmakeConfigCmd += " -DPython3_FIND_REGISTRY=NEVER";
    cmakeConfigCmd += " -DPython3_FIND_STRATEGY=LOCATION";

#elif defined(_WIN32)
    // HOST is Windows, TARGET is Windows -> Native Compilation
    Build::LogMessage("Host is Windows. Using native compilation...");
    // Nothing special to add, CMake will use MSVC automatically.
#endif

    return Build::ExecuteCommandWithOutput(cmakeConfigCmd, 0, 10) == 0;
}

bool WindowsBuilder::Compile(const std::string& buildDir) {
    std::string cmakeBuildCmd = "cmake --build \"" + buildDir + "\" --target FoxvoidStandalone --config Release";
    return Build::ExecuteCommandWithOutput(cmakeBuildCmd, 10, 90) == 0;
}

bool WindowsBuilder::CopyDependencies(const std::filesystem::path& buildDir, const std::string& engineRoot) {
    // List of required external DLLs for Windows (Python + MSVC Runtimes)
    std::vector<std::string> dllsToCopy = {
        "python311.dll",
        "vcruntime140.dll",
        "vcruntime140_1.dll"
    };

    bool success = true;
    for (const auto& dllName : dllsToCopy) {
        std::filesystem::path dllSource = std::filesystem::path(engineRoot) / "vendor/python_win/tools" / dllName;
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
