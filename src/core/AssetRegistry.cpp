#include "AssetRegistry.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

// Initialize static members
std::unordered_map<uint64_t, fs::path> AssetRegistry::s_Registry;
std::unordered_map<std::string, uint64_t> AssetRegistry::s_PathToUUID;
fs::path AssetRegistry::s_AssetsDirectory = "";

void AssetRegistry::Initialize(const fs::path& assetsDirectory) {
    s_AssetsDirectory = assetsDirectory;
    Refresh();
}

void AssetRegistry::Refresh() {
    s_Registry.clear();
    s_PathToUUID.clear();

    if (!fs::exists(s_AssetsDirectory)) {
        std::cerr << "[AssetRegistry] Assets directory does not exist: " << s_AssetsDirectory << std::endl;
        return;
    }

    std::cout << "[AssetRegistry] Scanning assets directory..." << std::endl;
    ProcessDirectory(s_AssetsDirectory);
    std::cout << "[AssetRegistry] Scan complete. Tracked assets: " << s_Registry.size() << std::endl;
}

void AssetRegistry::ProcessDirectory(const fs::path& directory) {
    for (const auto& entry : fs::directory_iterator(directory)) {
        if (entry.is_directory()) {
            if (entry.path().filename() == "__pycache__") continue;
            ProcessDirectory(entry.path());
        } 
        else {
            std::string extension = entry.path().extension().string();
            std::string filename = entry.path().filename().string();

            if (filename.empty() || filename[0] == '.') continue;
            if (extension == ".pyc") continue;
            if (extension == ".meta") continue;

            UUID assetUUID = LoadOrGenerateMetaFile(entry.path());
            
            s_Registry[(uint64_t)assetUUID] = entry.path();
            
            // Convert to absolute and standardized path before saving
            std::string normalizedPath = fs::absolute(entry.path()).lexically_normal().string();
            s_PathToUUID[normalizedPath] = (uint64_t)assetUUID;
        }
    }
}

UUID AssetRegistry::LoadOrGenerateMetaFile(const fs::path& assetPath) {
    fs::path metaPath = assetPath.string() + ".meta";
    
    if (fs::exists(metaPath)) {
        std::ifstream file(metaPath);
        if (file.is_open()) {
            nlohmann::json j;
            try {
                file >> j;
                if (j.contains("uuid")) {
                    return UUID(j["uuid"].get<uint64_t>());
                }
            } catch (const nlohmann::json::parse_error& e) {
                std::cerr << "[AssetRegistry] Failed to parse meta file: " << metaPath << " (" << e.what() << ")" << std::endl;
            }
        }
    }

    UUID newUUID;
    nlohmann::json j;
    j["uuid"] = (uint64_t)newUUID;

    std::ofstream outFile(metaPath);
    if (outFile.is_open()) {
        outFile << j.dump(4);
        outFile.close();
        std::cout << "[AssetRegistry] Generated new meta file for: " << assetPath.filename() << std::endl;
    } else {
        std::cerr << "[AssetRegistry] Failed to create meta file: " << metaPath << std::endl;
    }

    return newUUID;
}

fs::path AssetRegistry::GetPathForUUID(UUID uuid) {
    auto it = s_Registry.find((uint64_t)uuid);
    if (it != s_Registry.end()) {
        return it->second;
    }
    return fs::path(""); 
}

UUID AssetRegistry::GetUUIDForPath(const fs::path& path) {
    if (path.empty()) return UUID(0);

    // Convert the requested path to an absolute/standardized format to guarantee a match
    std::string normalizedPath = fs::absolute(path).lexically_normal().string();
    auto it = s_PathToUUID.find(normalizedPath);
    
    if (it != s_PathToUUID.end()) {
        return UUID(it->second);
    }

    // Just-In-Time (JIT) registration
    // If the file exists on disk but isn't in our registry yet, 
    // it means the user just added it via the OS explorer. Let's process it right now!
    if (fs::exists(normalizedPath) && !fs::is_directory(normalizedPath)) {
        // Ignore files that shouldn't have meta files
        std::string ext = fs::path(normalizedPath).extension().string();
        if (ext != ".meta" && ext != ".pyc" && fs::path(normalizedPath).filename().string()[0] != '.') {
            
            std::cout << "[AssetRegistry] Auto-registering new file: " << path.filename() << std::endl;
            
            UUID newUUID = LoadOrGenerateMetaFile(normalizedPath);
            s_Registry[(uint64_t)newUUID] = normalizedPath;
            s_PathToUUID[normalizedPath] = (uint64_t)newUUID;
            
            return newUUID;
        }
    }
    
    // Useful debug to know if a path failed to resolve
    std::cerr << "[AssetRegistry] Warning: Could not find UUID for path: " << normalizedPath << std::endl;
    return UUID(0); 
}
