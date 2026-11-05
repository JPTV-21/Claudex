#include "dialog_buttons.hpp"

#include "menu_markup.hpp"

#include <utility>

using namespace ftxui;

namespace claudex {

int mouseRowAt(const std::vector<Box>& boxes, const Mouse& mouse) {
    for (int row = 0; row < static_cast<int>(boxes.size()); ++row) {
        const Box& box = boxes[static_cast<std::size_t>(row)];
        if (!box.IsEmpty() && box.Contain(mouse.x, mouse.y)) {
            return row;
        }
    }
    return -1;
}

bool mouseInside(const Box& box, const Mouse& mouse) {
    return !box.IsEmpty() && box.Contain(mouse.x, mouse.y);
}

void DialogButtonBar::set(std::vector<std::string> newLabels,
                          std::function<void(int)> click) {
    labels = std::move(newLabels);
    on_click = std::move(click);
    boxes.assign(labels.size(), kEmptyHitBox);
    hovered = -1;
}

void DialogButtonBar::clear() {
    labels.clear();
    boxes.clear();
    hovered = -1;
    on_click = nullptr;
}

Element DialogButtonBar::render(const MenuTheme& theme) {
    Elements row;
    for (int i = 0; i < static_cast<int>(labels.size()); ++i) {
        if (i != 0) {
            row.push_back(text("   "));
        }
        Element button =
            renderThemeText(theme, labels[static_cast<std::size_t>(i)]);
        if (i == hovered) {
            button = button | inverted;
        }
        button = button | reflect(boxes[static_cast<std::size_t>(i)]);
        row.push_back(std::move(button));
    }
    return hbox(std::move(row)) | hcenter;
}

bool DialogButtonBar::handle(Event& event) {
    if (!event.is_mouse()) {
        return false;
    }
    const Mouse& mouse = event.mouse();
    const int row = mouseRowAt(boxes, mouse);
    if (mouse.button == Mouse::None && mouse.motion == Mouse::Moved) {
        if (row != hovered) {
            hovered = row;
            return true;
        }
        return false;
    }
    if (mouse.button == Mouse::Left && mouse.motion == Mouse::Pressed) {
        if (row >= 0 && on_click) {
            // Keep the callback alive even if the action switches pages and
            // replaces this bar's callback.
            auto click = on_click;
            click(row);
            return true;
        }
        return false;
    }
    return false;
}

}  // namespace claudex
