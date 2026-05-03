#pragma once

#include "IBuilder.hpp"

class WindowsBuilder : public IBuilder {
    public:
        bool Configure(const std::string& buildDir, const std::string& engineRoot) override;
        bool Compile(const std::string& buildDir) override;
        std::string GetExecutableExtension() const override { return ".exe"; }
        bool CopyDependencies(const std::filesystem::path& buildDir, const std::string& engineRoot, ScreenOrientation orientation = ScreenOrientation::Landscape) override;
};
