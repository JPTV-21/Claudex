#pragma once

#include <string>

namespace claudex {

// True when the caret has another wrapped line in that direction, in which
// case an Input should move the cursor instead of the focus.
bool cursorHasLine(const std::string& text, int cursor, bool down);

// Inserts '\n' at the caret and moves the caret past it.
void insertLineBreak(std::string& text, int& cursor);

}  // namespace claudex
