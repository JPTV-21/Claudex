#pragma once

#include "config_types.hpp"

#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/box.hpp>

#include <functional>
#include <string>
#include <vector>

namespace claudex {

// A box that cannot be hit. Clickable rows start with this value so an event
// that arrives before the first draw matches nothing.
inline const ftxui::Box kEmptyHitBox{1, 0, 1, 0};

// Row index whose reflected box contains |mouse|, or -1.
int mouseRowAt(const std::vector<ftxui::Box>& boxes,
               const ftxui::Mouse& mouse);
bool mouseInside(const ftxui::Box& box, const ftxui::Mouse& mouse);

// A horizontal row of clickable labels. The labels are markup, hovered buttons
// invert, and a left click runs the matching callback. Keyboard shortcuts are
// handled separately by the page handlers, so the bar stays mouse-only.
struct DialogButtonBar {
    std::vector<std::string> labels;
    std::vector<ftxui::Box> boxes;
    int hovered = -1;
    std::function<void(int)> on_click;

    void set(std::vector<std::string> newLabels,
             std::function<void(int)> click);
    void clear();
    ftxui::Element render(const MenuTheme& theme);
    bool handle(ftxui::Event& event);
};

}  // namespace claudex
