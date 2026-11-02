#pragma once

#include <ftxui/component/event.hpp>

#include <string>
#include <vector>

namespace claudex {

// Matches an event against a key spec from the theme: a single character,
// "esc", "enter", "tab", "f1".."f12", "up", "pageDown", and so on.
bool eventMatchesKeySpec(const ftxui::Event& event, const std::string& spec);
bool eventMatchesAny(const ftxui::Event& event,
                     const std::vector<std::string>& specs);

}  // namespace claudex
