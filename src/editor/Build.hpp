#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <mutex>
#include <atomic>
#include <thread>

class Build {
    public:
        // Spawns the background thread and starts the build process
        static void Start(const std::string& startScene, const std::string& outputDir, const std::filesystem::path& projectRoot, const std::string& engineRoot);

        // Thread-safe accessors for the Editor UI
        static bool IsBuilding();
        static int GetProgress();
        static std::string GetStatusMessage();
        static std::vector<std::string> GetLogs();

    private:
        static void RunThread(std::string startScene, std::string outputDir, std::filesystem::path projectRoot, std::string engineRoot);
        static int ExecuteCommandWithOutput(const std::string& cmd, int baseProgress, int maxProgress);

        // Thread-safe state
        static std::atomic<bool> s_isBuilding;
        static std::atomic<int> s_buildProgress;
        static std::mutex s_buildMutex;
        static std::string s_buildStatusMsg;
        static std::vector<std::string> s_buildLogs;
};
