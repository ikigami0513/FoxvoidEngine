#pragma once

#include <vector>

class PerformancePanel {
    public:
        PerformancePanel();
        ~PerformancePanel() = default;

        // Draws the performance UI window
        // Takes a reference to a boolean so the window can close itself
        void Draw();

    private:
        // History buffers for the graphs
        std::vector<float> m_fpsHistory;
        std::vector<float> m_frameTimeHistory;

        int m_maxHistory; // How many points to keep in the graph
        float m_updateTimer;   // Timer to slow down graph updates for readability
};
