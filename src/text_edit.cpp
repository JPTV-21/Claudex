#include "text_edit.hpp"

#include <algorithm>

namespace claudex {

bool cursorHasLine(const std::string& text, int cursor, bool down) {
    const std::size_t position = static_cast<std::size_t>(
        std::clamp(cursor, 0, static_cast<int>(text.size())));
    if (down) {
        return position < text.size() &&
               text.find('\n', position) != std::string::npos;
    }
    return position > 0 &&
           text.rfind('\n', position - 1) != std::string::npos;
}

void insertLineBreak(std::string& text, int& cursor) {
    const int position =
        std::clamp(cursor, 0, static_cast<int>(text.size()));
    text.insert(static_cast<std::size_t>(position), 1, '\n');
    cursor = position + 1;
}

}  // namespace claudex
