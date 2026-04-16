#pragma once

#ifndef STANDALONE_MODE
#include <imgui.h>
#endif 

#include <string>
#include <vector>
#include <iostream>
#include <streambuf>

// Defines the severity of the log message
enum class LogLevel {
    Info,
    Warning,
    Error
};

// Represents a single line in our console
struct LogEntry {
    LogLevel level;
    std::string message;
};

class EditorConsole {
    public:
        EditorConsole() {
            Clear();
        }

        // Adds a new message to the console
        void AddLog(LogLevel level, const std::string& message) {
            m_logs.push_back({level, message});

            // Keep memory under control by removing old logs if we exceed 1000 lines
            if (m_logs.size() > 1000) {
                m_logs.erase(m_logs.begin());
            }
            m_scrollToBottom = true;
        }

        // Clears all messages
        void Clear() {
            m_logs.clear();
        }

        // Draw the ImGui window
        void Draw(const char* title, bool* p_open = nullptr) {
            ImGui::Begin(title, p_open);

            // Button to clear the console
            if (ImGui::Button("Clear")) {
                Clear();
            }
            ImGui::SameLine();

            // Checkbox to toggle auto-scrolling
            ImGui::Checkbox("Auto-Scroll", &m_autoScroll);

            ImGui::Separator();

            // Reserve space for the scrolling region (leaving room for potential future input fields)
            const float footerHeightToReserve = ImGui::GetStyle().ItemSpacing.y;
            ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footerHeightToReserve), false, ImGuiWindowFlags_HorizontalScrollbar);
        
            for (const auto& log : m_logs) {
                // Determine text color based an log level
                ImVec4 color;
                bool hasColor = false;

                if (log.level == LogLevel::Error) {
                    color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); // Red for errors
                    hasColor = true;
                }
                else if (log.level == LogLevel::Warning) {
                    color = ImVec4(1.0f, 1.0f, 0.4f, 1.0f); // Yellow for warnings
                    hasColor = true;
                }

                // Apply color if necessary
                if (hasColor) ImGui::PushStyleColor(ImGuiCol_Text, color);

                // Draw the text (Unformatted is safer if strings contain '%' symbols)
                ImGui::TextUnformatted(log.message.c_str());

                if (hasColor) ImGui::PopStyleColor();
            }

            // Handle auto-scrolling logic
            if (m_scrollToBottom || (m_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())) {
                ImGui::SetScrollHereY(1.0f);
                m_scrollToBottom = false;
            }

            ImGui::EndChild();
            ImGui::End();
        }

    private:
        std::vector<LogEntry> m_logs;
        bool m_autoScroll = true;
        bool m_scrollToBottom = false;
};

// Steam redirector class
// A custom standard buffer to redirect stream outputs (like std::cout) to the EditorConsole
class ConsoleSink : public std::basic_streambuf<char> {
    public:
        ConsoleSink(std::ostream& stream, LogLevel level, EditorConsole& console)
            : m_stream(stream), m_level(level), m_console(console) {
            // Save the original terminal buffer and replace it with this one
            m_originalBuffer = stream.rdbuf(this);
        }

        ~ConsoleSink() {
            // Restore the original buffer when this object is destroyed to prevent crashes
            m_stream.rdbuf(m_originalBuffer);
        }

    protected:
        // This method is triggered whenever characters are pushed into the stream
        virtual int_type overflow(int_type v) override {
            if (v == '\n') {
                // When we hit a newline, send the accumulated string to the ImGui console
                m_console.AddLog(m_level, m_buffer);
                
                // Also print to the original terminal so we don't lose terminal logs!
                if (m_originalBuffer) {
                    m_originalBuffer->sputn(m_buffer.c_str(), m_buffer.length());
                    m_originalBuffer->sputc('\n');
                }
                
                m_buffer.clear();
            } else if (v != traits_type::eof()) {
                m_buffer += traits_type::to_char_type(v);
            }
            return v;
        }

    private:
        std::ostream& m_stream;
        std::streambuf* m_originalBuffer;
        LogLevel m_level;
        EditorConsole& m_console;
        std::string m_buffer;
};
