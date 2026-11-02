#pragma once

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>

#include <string>

namespace testfw {

// Removes SGR escape sequences (ESC [ ... m) so output can be compared as text.
inline std::string stripAnsi(const std::string& text) {
    std::string result;
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\x1b' && i + 1 < text.size() && text[i + 1] == '[') {
            std::size_t end = i + 2;
            while (end < text.size() &&
                   !((text[end] >= '@' && text[end] <= '~'))) {
                ++end;
            }
            i = end;
            continue;
        }
        result += text[i];
    }
    return result;
}

// Renders an element at its natural size and returns the raw terminal output.
inline std::string renderRaw(ftxui::Element element) {
    ftxui::Screen screen =
        ftxui::Screen::Create(ftxui::Dimension::Fit(element));
    ftxui::Render(screen, element);
    return screen.ToString();
}

inline std::string renderPlain(ftxui::Element element) {
    std::string text = stripAnsi(renderRaw(std::move(element)));
    // Normalize the platform line ending so assertions read the same on all
    // hosts.
    std::string normalized;
    normalized.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\r' && i + 1 < text.size() && text[i + 1] == '\n') {
            continue;
        }
        normalized += text[i];
    }
    while (!normalized.empty() && normalized.back() == '\n') {
        normalized.pop_back();
    }
    return normalized;
}

}  // namespace testfw
