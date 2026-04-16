#pragma once

#include "world/Component.hpp"
#include <string>
#include <raylib.h>

class TextRenderer : public Component {
    public:
        std::string text;
        float fontSize;
        float spacing;
        Color color;
        bool isHUD; // If true, renders in Screen Space, If false, renders in World Space.

        std::string fontPath;

        TextRenderer();
        ~TextRenderer() override;

        void Render() override;
        void RenderHUD() override;

        std::string GetName() const override;
        void OnInspector() override;

        nlohmann::json Serialize() const override;
        void Deserialize(const nlohmann::json& j) override;

        // Called when the path changes (from UI, JSON or Python)
        void SetFontPath(const std::string& path);

    private:
        Font m_customFont;
        bool m_isFontLoaded;

        void LoadFontFromDisk();
};
