#include "test_framework.hpp"

#include "render_helper.hpp"
#include "screens.hpp"
#include "scroll_bar.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>

#include <map>
#include <string>
#include <utility>
#include <vector>

using namespace claudex;
using ftxui::Element;
using ftxui::Elements;
using ftxui::Render;
using ftxui::Screen;
using ftxui::text;
using ftxui::vbox;

namespace {

void normalizeNewlines(std::string& text) {
    text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());
    while (!text.empty() && text.back() == '\n') {
        text.pop_back();
    }
}

// Renders on a fixed screen so a frame-limited list has something to scroll.
std::string renderFixed(Element element, int width, int height) {
    Screen screen(width, height);
    Render(screen, element);
    std::string plain = testfw::stripAnsi(screen.ToString());
    normalizeNewlines(plain);
    return plain;
}

Element listElement(int focused, int count, const ScrollbarStyle& style) {
    Elements lines;
    for (int i = 0; i < count; ++i) {
        Element line = text(std::to_string(i));
        if (i == focused) {
            line |= ftxui::focus;
        }
        lines.push_back(std::move(line));
    }
    return addScrollbar(vbox(std::move(lines)), style) | ftxui::frame |
           ftxui::border;
}

std::string renderList(int focused, int count, const ScrollbarStyle& style) {
    return renderFixed(listElement(focused, count, style), 6, 6);
}

}  // namespace

// --- parsing -----------------------------------------------------------------

TEST_CASE("scrollbar/defaults match the built-in indicator") {
    const json block = json::object();
    const ScrollbarStyle style = parseScrollbarStyle(&block, nullptr);
    CHECK(style.position == ScrollbarPosition::Right);
    CHECK_EQ(style.width, 1);
    CHECK_EQ(style.maxWidth, 0);
    CHECK_EQ(scrollbarWidth(style), 1);
    CHECK_FALSE(style.foreground.has_value());
    CHECK_FALSE(style.background.has_value());
    CHECK_EQ(style.full, "┃");
    CHECK_EQ(style.top, "╻");
    CHECK_EQ(style.bottom, "╹");
    CHECK_EQ(style.empty, " ");

    const ScrollbarStyle missing = parseScrollbarStyle(nullptr, nullptr);
    CHECK(missing.position == ScrollbarPosition::Right);
    CHECK_EQ(missing.width, 1);
}

TEST_CASE("scrollbar/reads position, size, colors and glyphs") {
    const json block = {
        {"scrollbar",
         {{"position", " LEFT "},
          {"width", 3},
          {"maxWidth", 2},
          {"color", "accent"},
          {"bgColor", "#102030"},
          {"characters",
           {{"full", "█"}, {"top", "▀"}, {"bottom", "▄"}, {"empty", "░"}}}}}};
    const std::map<std::string, std::string> variables = {
        {"accent", "#ff8800"}};

    const ScrollbarStyle style = parseScrollbarStyle(&block, &variables);
    CHECK(style.position == ScrollbarPosition::Left);
    CHECK_EQ(style.width, 3);
    CHECK_EQ(style.maxWidth, 2);
    CHECK_EQ(scrollbarWidth(style), 2);
    CHECK(style.foreground.has_value());
    CHECK(style.background.has_value());
    CHECK_EQ(style.full, "█");
    CHECK_EQ(style.top, "▀");
    CHECK_EQ(style.bottom, "▄");
    CHECK_EQ(style.empty, "░");
}

TEST_CASE("scrollbar/resolves sizes through theme variables") {
    const json block = {
        {"scrollbar", {{"width", "barWidth"}, {"maxWidth", "barMax"}}}};
    // Theme variable names are stored normalized (lowercase, no separators).
    const std::map<std::string, std::string> variables = {
        {"barwidth", "4"}, {"barmax", "3"}};
    const ScrollbarStyle style = parseScrollbarStyle(&block, &variables);
    CHECK_EQ(style.width, 4);
    CHECK_EQ(style.maxWidth, 3);
    CHECK_EQ(scrollbarWidth(style), 3);
}

TEST_CASE("scrollbar/ignores malformed values") {
    const json block = {
        {"scrollbar",
         {{"position", "diagonal"},
          {"width", 0},
          {"maxWidth", -2},
          {"color", "not-a-color"},
          {"characters",
           {{"full", "ab"}, {"top", ""}, {"bottom", "▶"}}}}}};
    const ScrollbarStyle style = parseScrollbarStyle(&block, nullptr);
    CHECK(style.position == ScrollbarPosition::Right);
    CHECK_EQ(style.width, 1);
    CHECK_EQ(style.maxWidth, 0);
    CHECK_EQ(scrollbarWidth(style), 1);
    CHECK_FALSE(style.foreground.has_value());
    // Non-single-cell strings fall back; a valid custom glyph is kept.
    CHECK_EQ(style.full, "┃");
    CHECK_EQ(style.top, "╻");
    CHECK_EQ(style.bottom, "▶");
    CHECK_EQ(style.empty, " ");
}

TEST_CASE("scrollbar/accepts non-object scrollbar values as defaults") {
    const json block = {{"scrollbar", true}};
    const ScrollbarStyle style = parseScrollbarStyle(&block, nullptr);
    CHECK_EQ(style.width, 1);
    CHECK(style.position == ScrollbarPosition::Right);
}

// --- rendering ---------------------------------------------------------------

TEST_CASE("scrollbar/right edge keeps the built-in layout") {
    const ScrollbarStyle style;
    CHECK_EQ(renderList(0, 10, style),
             "╭────╮\n"
             "│0  ┃│\n"
             "│1  ┃│\n"
             "│2   │\n"
             "│3   │\n"
             "╰────╯");
}

TEST_CASE("scrollbar/left position moves the bar to the first column") {
    ScrollbarStyle style;
    style.position = ScrollbarPosition::Left;
    CHECK_EQ(renderList(0, 10, style),
             "╭────╮\n"
             "│┃0  │\n"
             "│┃1  │\n"
             "│ 2  │\n"
             "│ 3  │\n"
             "╰────╯");
}

TEST_CASE("scrollbar/width reserves several columns") {
    ScrollbarStyle style;
    style.width = 2;
    CHECK_EQ(renderList(0, 10, style),
             "╭────╮\n"
             "│0 ┃┃│\n"
             "│1 ┃┃│\n"
             "│2   │\n"
             "│3   │\n"
             "╰────╯");
}

TEST_CASE("scrollbar/maxWidth caps the reserved columns") {
    ScrollbarStyle style;
    style.width = 3;
    style.maxWidth = 2;
    CHECK_EQ(renderList(0, 10, style),
             "╭────╮\n"
             "│0 ┃┃│\n"
             "│1 ┃┃│\n"
             "│2   │\n"
             "│3   │\n"
             "╰────╯");
}

TEST_CASE("scrollbar/custom glyphs replace the track and thumb") {
    ScrollbarStyle style;
    style.full = "█";
    // |top| fills the lower half of the thumb's first row, |bottom| the upper
    // half of its last row.
    style.top = "▄";
    style.bottom = "▀";
    style.empty = "░";
    CHECK_EQ(renderList(3, 10, style),
             "╭────╮\n"
             "│2  ▄│\n"
             "│3  █│\n"
             "│4  ▀│\n"
             "│5  ░│\n"
             "╰────╯");
}

TEST_CASE("scrollbar/multi-line entries are measured by their lines") {
    std::vector<std::string> entries{"0", "1", "2", "3", "4", "5", "6",
                                     "7"};
    int selected = 0;
    ftxui::MenuOption options = ftxui::MenuOption::Vertical();
    options.entries_option.transform = [](const ftxui::EntryState& state) {
        return vbox({text(state.label + " name"),
                     text(state.label + " path")});
    };
    auto menu = ftxui::Menu(&entries, &selected, options);

    // Eight two-line entries ask for sixteen rows, not eight.
    Element measured = menu->Render();
    measured->ComputeRequirement();
    CHECK_EQ(measured->requirement().min_y, 16);

    ScrollbarStyle style;
    Element list = addScrollbar(std::move(measured), style);
    list = list | ftxui::frame;
    list = list | ftxui::size(ftxui::HEIGHT, ftxui::LESS_THAN, 10);

    MenuTheme theme;
    theme.variables["listheight"] = "10";
    BlockRenderContext context;
    context.theme = &theme;
    context.hasProjectList = true;
    context.projectList = list;

    ScreenTheme screen;
    screen.blocks = json::array(
        {{{"type", "projectList"}, {"maxHeight", "listHeight"}}});
    const std::string plain =
        renderFixed(renderScreen(screen, context), 100, 30);
    CHECK(plain.find("┃") != std::string::npos);
}

TEST_CASE("scrollbar/never takes the last content column") {
    // The bar asks for more columns than the page has: the content keeps one.
    ScrollbarStyle style;
    style.width = 10;
    const std::string output = renderList(0, 10, style);
    CHECK_EQ(output,
             "╭────╮\n"
             "│0┃┃┃│\n"
             "│1┃┃┃│\n"
             "│2   │\n"
             "│3   │\n"
             "╰────╯");
}
