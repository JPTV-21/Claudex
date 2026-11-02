#include "key_events.hpp"

#include "text_util.hpp"

#include <cctype>
#include <string>

using ftxui::Event;

namespace claudex {

bool eventMatchesKeySpec(const Event& event, const std::string& spec) {
    const std::string token = lowerAscii(trim(spec));
    if (token.empty()) {
        return false;
    }
    if (token.size() == 1) {
        const char lower = token[0];
        const char upper = static_cast<char>(
            std::toupper(static_cast<unsigned char>(lower)));
        return event == Event::Character(lower) ||
               event == Event::Character(upper);
    }
    if (token == "esc" || token == "escape") {
        return event == Event::Escape;
    }
    if (token == "enter" || token == "return") {
        return event == Event::Return;
    }
    if (token == "tab") {
        return event == Event::Tab;
    }
    if (token == "tabreverse" || token == "shift-tab") {
        return event == Event::TabReverse;
    }
    if (token == "space") {
        return event == Event::Character(' ');
    }
    if (token == "backspace") {
        return event == Event::Backspace;
    }
    if (token == "delete" || token == "del") {
        return event == Event::Delete;
    }
    if (token == "insert" || token == "ins") {
        return event == Event::Insert;
    }
    if (token == "home") {
        return event == Event::Home;
    }
    if (token == "end") {
        return event == Event::End;
    }
    if (token == "pageup" || token == "pgup") {
        return event == Event::PageUp;
    }
    if (token == "pagedown" || token == "pgdn") {
        return event == Event::PageDown;
    }
    if (token == "up") {
        return event == Event::ArrowUp;
    }
    if (token == "down") {
        return event == Event::ArrowDown;
    }
    if (token == "left") {
        return event == Event::ArrowLeft;
    }
    if (token == "right") {
        return event == Event::ArrowRight;
    }
    if (token.size() >= 2 && token[0] == 'f') {
        try {
            switch (std::stoi(token.substr(1))) {
                case 1: return event == Event::F1;
                case 2: return event == Event::F2;
                case 3: return event == Event::F3;
                case 4: return event == Event::F4;
                case 5: return event == Event::F5;
                case 6: return event == Event::F6;
                case 7: return event == Event::F7;
                case 8: return event == Event::F8;
                case 9: return event == Event::F9;
                case 10: return event == Event::F10;
                case 11: return event == Event::F11;
                case 12: return event == Event::F12;
                default: break;
            }
        } catch (...) {
        }
    }
    return false;
}

bool eventMatchesAny(const Event& event,
                     const std::vector<std::string>& specs) {
    for (const std::string& spec : specs) {
        if (eventMatchesKeySpec(event, spec)) {
            return true;
        }
    }
    return false;
}

bool isShiftEnter(const Event& event) {
    // Event compares raw input bytes, so these are the sequences FTXUI leaves
    // untouched when it does not recognize them.
    return event == Event::Special("\x1b[13;2u") ||      // CSI-u / kitty
           event == Event::Special("\x1b[27;2;13~") ||   // xterm modifyOtherKeys
           event == Event::Special("\x1b\r");            // Alt+Enter fallback
}

}  // namespace claudex
