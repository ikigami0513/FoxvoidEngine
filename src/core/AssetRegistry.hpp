#pragma once

#include "UUID.hpp"
#include <filesystem>
#include <unordered_map>
#include <string>

namespace fs = std::filesystem;

class [[gnu::visibility("default")]] AssetRegistry {
    public:
        // Scans the asset directory, reading or generating .meta files
        static void Initialize(const fs::path& assetsDirectory);

        // Rescans the directory (useful if the user adds files externally)
        static void Refresh();

        // Returns the path associated with a UUID. Returns empty path if not found.
        static fs::path GetPathForUUID(UUID uuid);

        // Returns the UUID associated with a file path. Returns 0 (invalid) if not found.
        static UUID GetUUIDForPath(const fs::path& path);

    private:
        static void ProcessDirectory(const fs::path& directory);
        static UUID LoadOrGenerateMetaFile(const fs::path& assetPath);

        // The core dictionaries mapping UUIDs to paths and vice versa
        static std::unordered_map<uint64_t, fs::path> s_Registry;
        static std::unordered_map<std::string, uint64_t> s_PathToUUID;
        
        static fs::path s_AssetsDirectory;
};
