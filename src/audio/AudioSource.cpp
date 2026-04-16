#include "AudioSource.hpp"
#include <iostream>
#include <filesystem>

#ifndef STANDALONE_MODE
#include "editor/EditorUI.hpp"
#include "editor/commands/CommandHistory.hpp"
#include "editor/commands/ModifyComponentCommand.hpp"
#endif

AudioSource::AudioSource() : m_isMusicLoaded(false), m_musicVolume(1.0f), playOnStart(true) {
    m_currentMusic = {0};
}

AudioSource::~AudioSource() {
    // We must free the GPU/Audio memory when the component is destroyed
    for (auto& pair : m_sfx) {
        UnloadSound(pair.second);
    }
    m_sfx.clear();

    if (m_isMusicLoaded) {
        UnloadMusicStream(m_currentMusic);
    }
}

void AudioSource::Start() {
    if (playOnStart && m_isMusicLoaded) {
        // Automatically start the background music if the toggle is checked
        PlayMusic();
    }
}

void AudioSource::Update(float deltaTime) {
    // Music streams need to be fed buffer data every single frame
    if (m_isMusicLoaded && IsMusicStreamPlaying(m_currentMusic)) {
        UpdateMusicStream(m_currentMusic);
    }
}

void AudioSource::LoadSFX(const std::string& name, const std::string& path) {
    // If a sound with this name already exists, unload the old one first
    if (m_sfx.find(name) != m_sfx.end()) {
        UnloadSound(m_sfx[name]);
    }

    Sound newSound = LoadSound(path.c_str());
    if (newSound.frameCount > 0) { // Raylib way to check if load was successful
        m_sfx[name] = newSound;
        m_sfxPaths[name] = path;
    } else {
        std::cerr << "[AudioSource] Failed to load SFX: " << path << std::endl;
    }
}

void AudioSource::PlaySFX(const std::string& name) {
    if (m_sfx.find(name) != m_sfx.end()) {
        // Raylib allows the same sound to be triggered multiple times overlapping
        PlaySound(m_sfx[name]); 
    } else {
        std::cerr << "[AudioSource] SFX not found: " << name << std::endl;
    }
}

void AudioSource::LoadMusic(const std::string& path) {
    if (m_isMusicLoaded) {
        UnloadMusicStream(m_currentMusic);
        m_isMusicLoaded = false;
    }

    if (path.empty()) return;

    m_currentMusic = LoadMusicStream(path.c_str());
    if (m_currentMusic.frameCount > 0) {
        m_isMusicLoaded = true;
        m_musicPath = path;
        SetMusicVolume(m_musicVolume); // Apply current volume setting
    } else {
        std::cerr << "[AudioSource] Failed to load Music: " << path << std::endl;
    }
}

void AudioSource::PlayMusic() {
    if (m_isMusicLoaded) {
        PlayMusicStream(m_currentMusic);
    }
}

void AudioSource::StopMusic() {
    if (m_isMusicLoaded) {
        StopMusicStream(m_currentMusic);
    }
}

void AudioSource::SetMusicVolume(float volume) {
    m_musicVolume = volume;
    if (m_isMusicLoaded) {
        ::SetMusicVolume(m_currentMusic, m_musicVolume); // Call Raylib's global function
    }
}

std::string AudioSource::GetName() const {
    return "Audio Source";
}

#ifndef STANDALONE_MODE
void AudioSource::OnInspector() {
    ImGui::TextDisabled("Background Music");
    
    char musicBuffer[256];
    strncpy(musicBuffer, m_musicPath.c_str(), sizeof(musicBuffer));
    
    ImGui::InputText("Music Path", musicBuffer, sizeof(musicBuffer));
    
    // Drag & Drop for Background Music
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM")) {
            std::string droppedPath = (const char*)payload->Data;
            std::filesystem::path fsPath(droppedPath);
            std::string ext = fsPath.extension().string();
            
            // Accept standard audio formats
            if (ext == ".ogg" || ext == ".wav" || ext == ".mp3") {
                nlohmann::json initialState = Serialize();
                LoadMusic(droppedPath);
                CommandHistory::AddCommand(std::make_unique<ModifyComponentCommand>(this, initialState, Serialize()));
            }
        }
        ImGui::EndDragDropTarget();
    }

    if (ImGui::IsItemDeactivatedAfterEdit()) {
        nlohmann::json initialState = Serialize();
        LoadMusic(musicBuffer);
        CommandHistory::AddCommand(std::make_unique<ModifyComponentCommand>(this, initialState, Serialize()));
    }

    // Play on Start checkbox
    EditorUI::Checkbox("Play on Start", &playOnStart, this);

    // Music playback controls for the Editor
    if (m_isMusicLoaded) {
        if (ImGui::Button("Play BGM")) PlayMusic();
        ImGui::SameLine();
        if (ImGui::Button("Stop BGM")) StopMusic();

        if (EditorUI::DragFloat("Volume", &m_musicVolume, 0.01f, this, 0.0f, 1.0f)) {
            SetMusicVolume(m_musicVolume);
        }
    }

    ImGui::Separator();
    ImGui::TextDisabled("Sound Effects (SFX)");

    // Form to add a new SFX
    static char sfxName[64] = "";
    static char sfxPath[256] = "";

    ImGui::InputText("Name##sfx", sfxName, sizeof(sfxName));
    ImGui::InputText("Path##sfx", sfxPath, sizeof(sfxPath));
    
    // Drag & Drop for SFX
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM")) {
            std::string droppedPath = (const char*)payload->Data;
            std::filesystem::path fsPath(droppedPath);
            std::string ext = fsPath.extension().string();
            
            if (ext == ".ogg" || ext == ".wav" || ext == ".mp3") {
                // Fill the path buffer automatically
                strncpy(sfxPath, droppedPath.c_str(), sizeof(sfxPath));
                sfxPath[sizeof(sfxPath) - 1] = '\0';
                
                // Bonus: Auto-fill the name buffer with the filename (without extension) if empty
                if (strlen(sfxName) == 0) {
                    std::string stem = fsPath.stem().string();
                    strncpy(sfxName, stem.c_str(), sizeof(sfxName));
                    sfxName[sizeof(sfxName) - 1] = '\0';
                }
            }
        }
        ImGui::EndDragDropTarget();
    }

    if (ImGui::Button("Add SFX", ImVec2(-1, 0))) {
        std::string nameStr(sfxName);
        std::string pathStr(sfxPath);
        if (!nameStr.empty() && !pathStr.empty()) {
            nlohmann::json initialState = Serialize();
            LoadSFX(nameStr, pathStr);
            CommandHistory::AddCommand(std::make_unique<ModifyComponentCommand>(this, initialState, Serialize()));
            
            sfxName[0] = '\0';
            sfxPath[0] = '\0';
        }
    }

    // List loaded SFX
    if (!m_sfxPaths.empty()) {
        ImGui::Spacing();
        for (auto it = m_sfxPaths.begin(); it != m_sfxPaths.end(); ) {
            ImGui::PushID(it->first.c_str());
            ImGui::TextUnformatted(it->first.c_str());
            ImGui::SameLine(ImGui::GetWindowWidth() - 90);
            
            if (ImGui::Button("Play")) {
                PlaySFX(it->first);
            }
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
            if (ImGui::Button("X")) {
                nlohmann::json initialState = Serialize();
                
                // Unload from GPU/RAM
                UnloadSound(m_sfx[it->first]);
                m_sfx.erase(it->first);
                it = m_sfxPaths.erase(it);
                
                CommandHistory::AddCommand(std::make_unique<ModifyComponentCommand>(this, initialState, Serialize()));
                
                ImGui::PopStyleColor();
                ImGui::PopID();
                continue;
            }
            ImGui::PopStyleColor();
            ImGui::PopID();
            ++it;
        }
    }
}
#endif

nlohmann::json AudioSource::Serialize() const {
    return {
        {"type", "AudioSource"},
        {"musicPath", m_musicPath},
        {"musicVolume", m_musicVolume},
        {"playOnStart", playOnStart},
        {"sfxPaths", m_sfxPaths} // nlohmann::json natively serializes unordered_map!
    };
}

void AudioSource::Deserialize(const nlohmann::json& j) {
    m_musicVolume = j.value("musicVolume", 1.0f);
    playOnStart = j.value("playOnStart", true);
    
    std::string loadedMusicPath = j.value("musicPath", "");
    if (!loadedMusicPath.empty()) {
        LoadMusic(loadedMusicPath);
    }

    if (j.contains("sfxPaths")) {
        auto paths = j["sfxPaths"].get<std::unordered_map<std::string, std::string>>();
        for (const auto& pair : paths) {
            LoadSFX(pair.first, pair.second);
        }
    }
}
