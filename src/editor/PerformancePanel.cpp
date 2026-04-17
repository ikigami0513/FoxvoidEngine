#include "PerformancePanel.hpp"
#include <imgui.h>
#include <raylib.h>
#include <string>

PerformancePanel::PerformancePanel() : m_maxHistory(120), m_updateTimer(0.0f) {
    // Initialize vectors with zeros
    m_fpsHistory.resize(m_maxHistory, 0.0f);
    m_frameTimeHistory.resize(m_maxHistory, 0.0f);
}

void PerformancePanel::Draw() {
    // Set a default size for the window
    ImGui::SetNextWindowSize(ImVec2(350, 250), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Performance Profiler")) {
        // Retrieve current metrics from Raylib
        int currentFPS = GetFPS();
        float currentFrameTime = GetFrameTime() * 1000.0f; // Convert seconds to milliseconds

        // Update the graph data every 0.05 seconds (to avoid unreadable flickering)
        m_updateTimer += GetFrameTime();
        if (m_updateTimer >= 0.05f) {
            // Shift all elements to the left
            m_fpsHistory.erase(m_fpsHistory.begin());
            m_frameTimeHistory.erase(m_frameTimeHistory.begin());
            
            // Add new values at the end
            m_fpsHistory.push_back((float)currentFPS);
            m_frameTimeHistory.push_back(currentFrameTime);
            
            m_updateTimer = 0.0f;
        }

        // Text Metrics
        ImGui::Text("FPS: %d", currentFPS);
        ImGui::SameLine(150);
        
        // Color the frame time text (Red if taking longer than ~16.6ms for 60fps)
        if (currentFrameTime > 16.6f) {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Frame Time: %.2f ms", currentFrameTime);
        } else {
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Frame Time: %.2f ms", currentFrameTime);
        }

        ImGui::Separator();
        ImGui::Spacing();

        // FPS Graph
        std::string fpsOverlay = "Current: " + std::to_string(currentFPS);
        ImGui::PlotLines("##FPS", m_fpsHistory.data(), (int)m_fpsHistory.size(), 0, fpsOverlay.c_str(), 0.0f, 120.0f, ImVec2(ImGui::GetContentRegionAvail().x, 60));
        
        ImGui::Spacing();

        // Frame Time Graph
        // For a smooth 60fps, frame time should be below 16.6ms
        std::string timeOverlay = std::to_string(currentFrameTime).substr(0, 4) + " ms";
        ImGui::PlotHistogram("##FrameTime", m_frameTimeHistory.data(), (int)m_frameTimeHistory.size(), 0, timeOverlay.c_str(), 0.0f, 33.3f, ImVec2(ImGui::GetContentRegionAvail().x, 60));
    }
    ImGui::End();
}
