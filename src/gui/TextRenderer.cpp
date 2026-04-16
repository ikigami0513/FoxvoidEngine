#include "TextRenderer.hpp"
#include "editor/EditorUI.hpp"
#include "editor/commands/CommandHistory.hpp"
#include "editor/commands/ModifyComponentCommand.hpp"
#include "physics/Transform2d.hpp"
#include "world/GameObject.hpp"

TextRenderer::TextRenderer()
    : text("New Text"), fontSize(20.0f), spacing(1.0f), color(BLACK), isHUD(true),
      fontPath(""), m_isFontLoaded(false), m_customFont{0} {}

TextRenderer::~TextRenderer() {
    // Free the GPU memory when the component is destroyed
    if (m_isFontLoaded) {
        UnloadFont(m_customFont);
    }
}

void TextRenderer::SetFontPath(const std::string& path) {
    if (fontPath == path) return;
    fontPath = path;
    LoadFontFromDisk();
}

void TextRenderer::LoadFontFromDisk() {
    // Unload the previous font if one was loaded
    if (m_isFontLoaded) {
        UnloadFont(m_customFont);
        m_isFontLoaded = false;
    }

    if (!fontPath.empty() && std::filesystem::exists(fontPath)) {
        // We load the font at a high resolution (128) so it stays crisp even if 'fontSize' is large.
        // Raylib will automatically scale it down perfectly during DrawTextEx.
        m_customFont = LoadFontEx(fontPath.c_str(), 128, 0, 250);
        
        // Use Point filtering for crisp pixel-art fonts, or Bilinear for smooth modern fonts.
        // (You could make this an Inspector toggle later!)
        SetTextureFilter(m_customFont.texture, TEXTURE_FILTER_BILINEAR); 
        
        m_isFontLoaded = true;
    } else if (!fontPath.empty()) {
        std::cerr << "[TextRenderer] Error: Font not found at " << fontPath << std::endl;
    }
}

void TextRenderer::Render() {
    // If it's a HUD element, we don't draw it in the world pass
    if (isHUD || !owner) return;

    auto transform = owner->GetComponent<Transform2d>();
    if (!transform) return;

    // Use custom font if loaded, else fallback to Raylib's default
    Font f = m_isFontLoaded ? m_customFont : GetFontDefault();
    DrawTextEx(f, text.c_str(), transform->position, fontSize, spacing, color);
}

void TextRenderer::RenderHUD() {
    // If it's a world element, we don't draw it in the HUD pass
    if (!isHUD || !owner) return;

    auto transform = owner->GetComponent<Transform2d>();
    if (!transform) return;

    // The Transform position now acts as X/Y coordinates on the screen, completely ignoring the camera!
    Font f = m_isFontLoaded ? m_customFont : GetFontDefault();
    DrawTextEx(f, text.c_str(), transform->position, fontSize, spacing, color);
}

std::string TextRenderer::GetName() const {
    return "Text Renderer";
}

void TextRenderer::OnInspector() {
    // Text Input with Undo/Redo support
    char buffer[512];
    strncpy(buffer, text.c_str(), sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';

    static nlohmann::json initialState;

    // We use a multi-line input to allow for paragraphs or line breaks (\n)
    ImGui::InputTextMultiline("Text", buffer, sizeof(buffer), ImVec2(-1.0f, ImGui::GetTextLineHeight() * 4));

    if (ImGui::IsItemActivated()) {
        initialState = Serialize();
    }
    
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        text = buffer;
        CommandHistory::AddCommand(std::make_unique<ModifyComponentCommand>(this, initialState, Serialize()));
    }

    ImGui::Separator();

    // Font path input & Drag-Drop
    char pathBuffer[256];
    strncpy(pathBuffer, fontPath.c_str(), sizeof(pathBuffer));
    pathBuffer[sizeof(pathBuffer) - 1] = '\0';

    ImGui::InputText("Font Path", pathBuffer, sizeof(pathBuffer));

    // Drag & Drop for .ttf files from the Content Browser
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM")) {
            std::string droppedPath = (const char*)payload->Data;
            std::filesystem::path fsPath(droppedPath);
            
            if (fsPath.extension() == ".ttf" || fsPath.extension() == ".otf") {
                nlohmann::json stateBeforeDrop = Serialize();
                SetFontPath(droppedPath);
                CommandHistory::AddCommand(std::make_unique<ModifyComponentCommand>(this, stateBeforeDrop, Serialize()));
            }
        }
        ImGui::EndDragDropTarget();
    }

    if (ImGui::IsItemActivated()) initialState = Serialize();
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        SetFontPath(pathBuffer);
        CommandHistory::AddCommand(std::make_unique<ModifyComponentCommand>(this, initialState, Serialize()));
    }

    ImGui::TextDisabled(m_isFontLoaded ? "Status: Custom Font Loaded" : "Status: Using Default Font");

    ImGui::Separator();

    // Styling settings using our custom EditorUI
    EditorUI::DragFloat("Font Size", &fontSize, 0.5f, this, 1.0f, 200.0f);
    EditorUI::DragFloat("Letter Spacing", &spacing, 0.1f, this, 0.0f, 20.0f);
    EditorUI::ColorEdit4("Color", &color, this);
    
    ImGui::Separator();
    
    // Rendering Mode
    EditorUI::Checkbox("Is HUD (Screen Space)", &isHUD, this);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("If checked, the text sticks to the screen. If unchecked, it exists in the world.");
    }
}

nlohmann::json TextRenderer::Serialize() const {
    return {
        {"type", "TextRenderer"},
        {"text", text},
        {"fontPath", fontPath},
        {"fontSize", fontSize},
        {"spacing", spacing},
        {"isHUD", isHUD},
        {"color_r", color.r},
        {"color_g", color.g},
        {"color_b", color.b},
        {"color_a", color.a}
    };
}

void TextRenderer::Deserialize(const nlohmann::json& j) {
    text = j.value("text", "New Text");
    fontSize = j.value("fontSize", 20.0f);
    spacing = j.value("spacing", 1.0f);
    isHUD = j.value("isHUD", true);

    color.r = j.value("color_r", 0);
    color.g = j.value("color_g", 0);
    color.b = j.value("color_b", 0);
    color.a = j.value("color_a", 255);

    // Call the setter to ensure the font is physically loaded from disk
    SetFontPath(j.value("fontPath", ""));
}
