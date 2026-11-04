#include "scroll_bar.hpp"

#include "menu_markup.hpp"
#include "text_util.hpp"

#include <ftxui/dom/node.hpp>
#include <ftxui/screen/box.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/screen/string.hpp>

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

namespace claudex {

namespace {

// One rendered cell, so a double-width or empty string cannot shift the list
// layout when it replaces the track.
std::string glyphField(const json& characters, const char* key,
                       const std::string& fallback) {
    const std::string value = jsonStringField(characters, key, fallback);
    if (value.empty() || ftxui::string_width(value) != 1) {
        return fallback;
    }
    return value;
}

class ScrollbarNode : public ftxui::Node {
public:
    ScrollbarNode(ftxui::Element child, ScrollbarStyle style)
        : Node(ftxui::Elements{std::move(child)}), style_(std::move(style)) {
        requested_columns_ = scrollbarWidth(style_);
    }

    void ComputeRequirement() override {
        Node::ComputeRequirement();
        requirement_.min_x += requested_columns_;
    }

    void SetBox(ftxui::Box box) override {
        box_ = box;
        // Keep at least one column for the content, even when the page is
        // narrower than the configured bar.
        const int available = box.x_max - box.x_min;
        columns_ = std::clamp(requested_columns_, 0, std::max(0, available));
        if (columns_ > 0) {
            if (style_.position == ScrollbarPosition::Left) {
                box.x_min += columns_;
            } else {
                box.x_max -= columns_;
            }
        }
        children_[0]->SetBox(box);
    }

    void Render(ftxui::Screen& screen) override {
        const ftxui::Box stencil = screen.stencil;
        // A frame hands its child a box that can be wider than the visible
        // area, so cap by the stencil and always leave one visible column for
        // the content.
        const int stencil_width = stencil.x_max - stencil.x_min + 1;
        const int columns =
            std::clamp(columns_, 0, std::max(0, stencil_width - 1));
        if (columns <= 0) {
            Node::Render(screen);
            return;
        }

        // Save the style from the parents before the content draws over it.
        std::vector<ftxui::Cell> saved;
        saved.reserve(static_cast<std::size_t>(columns) *
                      static_cast<std::size_t>(stencil.y_max -
                                               stencil.y_min + 1));
        for (int y = stencil.y_min; y <= stencil.y_max; ++y) {
            for (int i = 0; i < columns; ++i) {
                saved.push_back(screen.CellAt(columnX(stencil, i), y));
            }
        }

        Node::Render(screen);

        const int size_inner = box_.y_max - box_.y_min;
        if (size_inner <= 0) {
            return;
        }
        const int size_outter = stencil.y_max - stencil.y_min + 1;
        if (size_outter >= size_inner) {
            return;
        }

        int size = 2 * size_outter * size_outter / size_inner;
        size = std::max(size, 1);
        const int start_y =
            2 * stencil.y_min +
            2 * (stencil.y_min - box_.y_min) * size_outter / size_inner;

        for (int y = stencil.y_min; y <= stencil.y_max; ++y) {
            const int y_up = 2 * y;
            const int y_down = 2 * y + 1;
            const bool up = (start_y <= y_up) && (y_up <= start_y + size);
            const bool down =
                (start_y <= y_down) && (y_down <= start_y + size);
            const std::string& glyph =
                up ? (down ? style_.full : style_.bottom)
                   : (down ? style_.top : style_.empty);

            for (int i = 0; i < columns; ++i) {
                ftxui::Cell& cell = screen.CellAt(columnX(stencil, i), y);
                // The background follows the content, as the plain indicator
                // does, unless the theme asks for a fixed one.
                const ftxui::Color background = cell.background_color;
                cell = saved[static_cast<std::size_t>(y - stencil.y_min) *
                                 static_cast<std::size_t>(columns) +
                             static_cast<std::size_t>(i)];
                cell.background_color = background;
                cell.character = glyph;
                if (style_.foreground) {
                    cell.foreground_color = *style_.foreground;
                }
                if (style_.background) {
                    cell.background_color = *style_.background;
                }
            }
        }
    }

private:
    int columnX(const ftxui::Box& stencil, int index) const {
        return style_.position == ScrollbarPosition::Left
                   ? stencil.x_min + index
                   : stencil.x_max - index;
    }

    ScrollbarStyle style_;
    int requested_columns_ = 1;
    int columns_ = 1;
};

}  // namespace

ScrollbarStyle parseScrollbarStyle(
    const json* block, const std::map<std::string, std::string>* variables) {
    ScrollbarStyle style;
    if (block == nullptr) {
        return style;
    }
    const auto found = block->find("scrollbar");
    if (found == block->end() || !found->is_object()) {
        return style;
    }
    const json& options = *found;

    style.position =
        normalizeMenuToken(jsonStringField(options, "position", "right")) ==
                "left"
            ? ScrollbarPosition::Left
            : ScrollbarPosition::Right;
    style.width =
        std::max(1, jsonIntFieldResolved(options, "width", 1, variables));
    style.maxWidth = std::max(
        0, jsonIntFieldResolved(options, "maxWidth", 0, variables));

    style.foreground = parseThemeColor(
        jsonStringField(options, "color", ""), variables);
    style.background = parseThemeColor(
        jsonStringField(options, "backgroundColor",
                        jsonStringField(options, "bgColor", "")),
        variables);

    if (options.contains("characters") && options["characters"].is_object()) {
        const json& characters = options["characters"];
        style.full = glyphField(characters, "full", style.full);
        style.top = glyphField(characters, "top", style.top);
        style.bottom = glyphField(characters, "bottom", style.bottom);
        style.empty = glyphField(characters, "empty", style.empty);
    }

    return style;
}

int scrollbarWidth(const ScrollbarStyle& style) {
    int width = std::max(1, style.width);
    if (style.maxWidth > 0) {
        width = std::min(width, style.maxWidth);
    }
    return std::max(1, width);
}

ftxui::Element addScrollbar(ftxui::Element child,
                            const ScrollbarStyle& style) {
    return std::make_shared<ScrollbarNode>(std::move(child), style);
}

}  // namespace claudex
