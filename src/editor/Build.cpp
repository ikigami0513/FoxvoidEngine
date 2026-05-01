#include "Build.hpp"
#include "core/ProjectSettings.hpp"
#include "core/AssetRegistry.hpp"
#include "core/PakEntry.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>

// Initialize static members
std::atomic<bool> Build::s_isBuilding{false};
std::atomic<int> Build::s_buildProgress{0};
std::mutex Build::s_buildMutex;
std::string Build::s_buildStatusMsg = "";
std::vector<std::string> Build::s_buildLogs;

void Build::Start(const std::string& startScene, const std::string& outputDir, const std::filesystem::path& projectRoot, const std::string& engineRoot) {
    if (s_isBuilding) return; // Prevent multiple builds at the same time

    s_isBuilding = true;
    s_buildProgress = 0;
    
    {
        std::lock_guard<std::mutex> lock(s_buildMutex);
        s_buildStatusMsg = "Initializing Build...";
        s_buildLogs.clear();
    }

    // Spawn the thread and detach it
    std::thread(&Build::RunThread, startScene, outputDir, projectRoot, engineRoot).detach();
}

bool Build::IsBuilding() {
    return s_isBuilding.load();
}

int Build::GetProgress() {
    return s_buildProgress.load();
}

std::string Build::GetStatusMessage() {
    std::lock_guard<std::mutex> lock(s_buildMutex);
    return s_buildStatusMsg;
}

std::vector<std::string> Build::GetLogs() {
    std::lock_guard<std::mutex> lock(s_buildMutex);
    return s_buildLogs; // Returns a safe copy for the UI to iterate over
}

void Build::RunThread(std::string startSceneStr, std::string outputDirStr, std::filesystem::path projectRoot, std::string engineRoot) {
    std::filesystem::path buildDir = projectRoot / outputDirStr;
    
    {
        std::lock_guard<std::mutex> lock(s_buildMutex);
        s_buildLogs.clear();
        s_buildStatusMsg = "Step 1/3: Configuring CMake...";
    }
    s_buildProgress = 0;
    
    std::string cmakeConfigCmd = "cmake -S \"" + engineRoot + "\" -B \"" + buildDir.string() + "\" -DCMAKE_BUILD_TYPE=Release";
    int configResult = ExecuteCommandWithOutput(cmakeConfigCmd, 0, 10);

    if (configResult == 0) {
        {
            std::lock_guard<std::mutex> lock(s_buildMutex);
            s_buildStatusMsg = "Step 2/3: Compiling Game (This will take a moment)...";
        }
        
        std::string cmakeBuildCmd = "cmake --build \"" + buildDir.string() + "\" --target FoxvoidStandalone --config Release";
        int buildResult = ExecuteCommandWithOutput(cmakeBuildCmd, 10, 90);

        if (buildResult == 0) {
            {
                std::lock_guard<std::mutex> lock(s_buildMutex);
                s_buildStatusMsg = "Step 3/3: Packing Assets...";
            }
            s_buildProgress = 95;

            try {
                // Copy the project configuration
                std::filesystem::copy_file(projectRoot / "project.json", buildDir / "project.json", std::filesystem::copy_options::overwrite_existing);
                
                // The packer (Virtual File System)
                {
                    std::lock_guard<std::mutex> lock(s_buildMutex);
                    s_buildStatusMsg = "Step 3/3: Packing Assets into data.pak...";
                }

                std::filesystem::path pakPath = buildDir / "data.pak";
                std::ofstream pakFile(pakPath, std::ios::binary);

                if (pakFile.is_open()) {
                    std::vector<std::filesystem::path> filesToPack;
                    for (const auto& entry : std::filesystem::recursive_directory_iterator(projectRoot / "assets")) {
                        if (entry.is_regular_file()) {
                            std::string ext = entry.path().extension().string();
                            std::string name = entry.path().filename().string();
                            
                            if (ext != ".meta" && ext != ".pyc" && name[0] != '.') {
                                filesToPack.push_back(entry.path());
                            }
                        }
                    }

                    uint32_t numFiles = static_cast<uint32_t>(filesToPack.size());
                    pakFile.write(reinterpret_cast<const char*>(&numFiles), sizeof(uint32_t));

                    std::vector<PakEntry> toc;

                    uint64_t tocOffset = pakFile.tellp();
                    uint64_t tocSize = numFiles * sizeof(PakEntry);
                    pakFile.seekp(tocOffset + tocSize);

                    for (const auto& filePath : filesToPack) {
                        UUID fileUUID = AssetRegistry::GetUUIDForPath(filePath);
                        
                        if (fileUUID != 0) {
                            std::ifstream sourceFile(filePath, std::ios::binary | std::ios::ate);
                            if (sourceFile.is_open()) {
                                uint64_t fileSize = sourceFile.tellg();
                                sourceFile.seekg(0);

                                uint64_t currentOffset = pakFile.tellp();

                                std::string relPath = std::filesystem::relative(filePath, projectRoot).generic_string();

                                PakEntry entry = {(uint64_t)fileUUID, currentOffset, fileSize, {0}};
                                strncpy(entry.path, relPath.c_str(), sizeof(entry.path) - 1);

                                toc.push_back(entry);

                                std::vector<char> buffer(fileSize);
                                sourceFile.read(buffer.data(), fileSize);
                                pakFile.write(buffer.data(), fileSize);

                                sourceFile.close();
                            }
                        }
                    }

                    pakFile.seekp(tocOffset);
                    pakFile.write(reinterpret_cast<const char*>(toc.data()), toc.size() * sizeof(PakEntry));
                    pakFile.close();
                } else {
                    throw std::runtime_error("Could not create data.pak file.");
                }

                // Rename the executable based on the Project Name
                std::string projectName = ProjectSettings::GetProjectName();
                std::string safeProjectName = projectName;
                std::replace(safeProjectName.begin(), safeProjectName.end(), ' ', '_');

                std::filesystem::path originalExe;
                std::filesystem::path newExe;

#if defined(_WIN32)
                originalExe = buildDir / "FoxvoidStandalone.exe";
                if (!std::filesystem::exists(originalExe)) {
                    originalExe = buildDir / "Release" / "FoxvoidStandalone.exe"; 
                }
                newExe = buildDir / (safeProjectName + ".exe");
#else
                originalExe = buildDir / "FoxvoidStandalone";
                newExe = buildDir / safeProjectName;
#endif

                if (std::filesystem::exists(originalExe)) {
                    std::filesystem::copy_file(originalExe, newExe, std::filesystem::copy_options::overwrite_existing);
                    std::filesystem::remove(originalExe);
                } else {
                    std::lock_guard<std::mutex> lock(s_buildMutex);
                    s_buildLogs.push_back("[Warning] Could not find the executable to rename.");
                }

                {
                    std::lock_guard<std::mutex> lock(s_buildMutex);
                    s_buildStatusMsg = "SUCCESS! Game exported to: " + outputDirStr;
                    s_buildLogs.push_back("--- BUILD FINISHED SUCCESSFULLY ---");
                }
                s_buildProgress = 100;
            } catch(std::filesystem::filesystem_error& e) {
                {
                    std::lock_guard<std::mutex> lock(s_buildMutex);
                    s_buildStatusMsg = "Error copying assets.";
                    s_buildLogs.push_back(std::string("Filesystem Error: ") + e.what());
                }
                s_buildProgress = -1; 
            }
        } else {
            {
                std::lock_guard<std::mutex> lock(s_buildMutex);
                s_buildStatusMsg = "FAILED during compilation.";
            }
            s_buildProgress = -1;
        }
    } else {
        {
            std::lock_guard<std::mutex> lock(s_buildMutex);
            s_buildStatusMsg = "FAILED during CMake configuration.";
        }
        s_buildProgress = -1;
    }

    s_isBuilding = false;
}

int Build::ExecuteCommandWithOutput(const std::string& cmd, int baseProgress, int maxProgress) {
    std::string fullCmd = cmd + " 2>&1";
    FILE* pipe = popen(fullCmd.c_str(), "r");
    if (!pipe) {
        std::lock_guard<std::mutex> lock(s_buildMutex);
        s_buildLogs.push_back("[Error] Failed to open process pipe.");
        return -1;
    }

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        std::string line(buffer);
        if (!line.empty() && line.back() == '\n') line.pop_back();

        {
            std::lock_guard<std::mutex> lock(s_buildMutex);
            s_buildLogs.push_back(line);
        }

        size_t pctPos = line.find("%");
        if (pctPos != std::string::npos && pctPos > 0) {
            size_t start = pctPos - 1;
            while (start > 0 && std::isdigit(line[start])) start--;
            if (!std::isdigit(line[start])) start++;

            if (start < pctPos) {
                try {
                    int extractedPct = std::stoi(line.substr(start, pctPos - start));
                    float fraction = extractedPct / 100.0f;
                    s_buildProgress = baseProgress + static_cast<int>(fraction * (maxProgress - baseProgress));
                } catch(...) {}
            }
        }
    }

    return pclose(pipe);
}
