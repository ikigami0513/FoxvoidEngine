#pragma once

#include <string>
#include <filesystem>

// The Strategy Interface for all OS-specific build processes
class IBuilder {
    public:
        virtual ~IBuilder() = default;

        // Step 1: Configures CMake for the target OS
        virtual bool Configure(const std::string& buildDir, const std::string& engineRoot) = 0;

        // Step 2: Compiles the generated CMake project
        virtual bool Compile(const std::string& buildDir) = 0;

        // Step 3: Returns the OS-specific executable extension (e.g., ".exe" for Windows, "" for Linux)
        virtual std::string GetExecutableExtension() const = 0;

        // Step 4: Copies OS-specific dependencies (e.g., DLLs for Windows) next to the executable
        virtual bool CopyDependencies(const std::filesystem::path& buildDir, const std::string& engineRoot) = 0;
};
