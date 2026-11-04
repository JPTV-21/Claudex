#pragma once

#include "config_types.hpp"

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>

#include <map>
#include <optional>
#include <string>

namespace claudex {

enum class ScrollbarPosition { Left, Right };

// Per-list scrollbar settings, read from a list block's optional "scrollbar"
// object. An unset color inherits the list block's color, which is what the
// FTXUI indicator did before this was configurable.
struct ScrollbarStyle {
    ScrollbarPosition position = ScrollbarPosition::Right;
    // Reserved columns; |maxWidth| (0 = unset) caps |width|.
    int width = 1;
    int maxWidth = 0;
    std::optional<ftxui::Color> foreground;
    std::optional<ftxui::Color> background;
    // One-cell glyphs. |full| covers both halves of a row; |top| and |bottom|
    // are used for the half-covered rows at the top and bottom end of the
    // thumb; |empty| is the track.
    std::string full = "┃";
    std::string top = "╻";
    std::string bottom = "╹";
    std::string empty = " ";
};

// Reads |block|["scrollbar"]. |block| may be null, and a missing or malformed
// object keeps the defaults. Color names resolve through |variables|.
ScrollbarStyle parseScrollbarStyle(
    const json* block, const std::map<std::string, std::string>* variables);

// Number of columns the bar reserves after applying |maxWidth|.
int scrollbarWidth(const ScrollbarStyle& style);

// Draws a vertical scroll indicator around |child|. The indicator occupies the
// outermost |width| columns on the configured side and leaves the rest to the
// child, mirroring ftxui::vscroll_indicator.
ftxui::Element addScrollbar(ftxui::Element child, const ScrollbarStyle& style);

}  // namespace claudex
