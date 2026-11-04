#include "test_framework.hpp"

#include "grid_menu.hpp"
#include "render_helper.hpp"

#include <ftxui/component/event.hpp>
#include <ftxui/screen/screen.hpp>

using namespace claudex;
using ftxui::Event;
using testfw::renderPlain;

namespace {

std::vector<std::string> entriesOf(int count) {
    std::vector<std::string> entries;
    for (int i = 1; i <= count; ++i) {
        entries.push_back("i" + std::to_string(i));
    }
    return entries;
}

// Renders a component at a fixed width so wrapping and padding are stable.
std::string renderAt(const ftxui::Component& component, int width) {
    ftxui::Element element = component->Render();
    ftxui::Screen screen = ftxui::Screen::Create(
        ftxui::Dimension::Fixed(width), ftxui::Dimension::Fit(element));
    ftxui::Render(screen, element);
    return testfw::stripAnsi(screen.ToString());
}

bool press(const ftxui::Component& component, Event event) {
    return component->OnEvent(event);
}

GridMenuOptions gridOptions(int columns) {
    GridMenuOptions options;
    options.columns = columns;
    options.activePrefix = "> ";
    options.inactivePrefix = "  ";
    options.columnGap = 1;
    return options;
}

}  // namespace

// --- column count ----------------------------------------------------------------

TEST_CASE("grid/column count fills the width") {
    CHECK_EQ(gridColumnCount(40, 10, 2, 10), 3);   // (40+2)/12 = 3
    CHECK_EQ(gridColumnCount(10, 10, 2, 10), 1);
    CHECK_EQ(gridColumnCount(24, 10, 2, 10), 2);
    CHECK_EQ(gridColumnCount(1000, 10, 2, 3), 3);  // never more than entries
    CHECK_EQ(gridColumnCount(0, 10, 2, 5), 1);
    CHECK_EQ(gridColumnCount(40, 0, 2, 5), 1);
    CHECK_EQ(gridColumnCount(40, 10, 2, 0), 1);
}

// --- conversion to events ----------------------------------------------------------

TEST_CASE("grid/up and down move by a row") {
    std::vector<std::string> entries = entriesOf(5);
    int selected = 0;
    const auto menu = GridMenu(&entries, &selected, gridOptions(2));

    CHECK(press(menu, Event::ArrowDown));
    CHECK_EQ(selected, 2);
    CHECK(press(menu, Event::ArrowDown));
    CHECK_EQ(selected, 4);
    // The last row is short: 4 is the last entry.
    CHECK_FALSE(press(menu, Event::ArrowDown));

    CHECK(press(menu, Event::ArrowUp));
    CHECK_EQ(selected, 2);
    CHECK(press(menu, Event::ArrowUp));
    CHECK_EQ(selected, 0);
    CHECK_FALSE(press(menu, Event::ArrowUp));
}

TEST_CASE("grid/down reaches the short last row") {
    std::vector<std::string> entries = entriesOf(5);
    int selected = 3;
    const auto menu = GridMenu(&entries, &selected, gridOptions(2));
    CHECK(press(menu, Event::ArrowDown));
    CHECK_EQ(selected, 4);
}

TEST_CASE("grid/left and right stay inside a row") {
    std::vector<std::string> entries = entriesOf(5);
    int selected = 0;
    const auto menu = GridMenu(&entries, &selected, gridOptions(2));

    CHECK(press(menu, Event::ArrowRight));
    CHECK_EQ(selected, 1);
    // Column 1 of a two-column grid cannot move right.
    CHECK_FALSE(press(menu, Event::ArrowRight));
    CHECK(press(menu, Event::ArrowLeft));
    CHECK_EQ(selected, 0);
    CHECK_FALSE(press(menu, Event::ArrowLeft));
}

TEST_CASE("grid/single column ignores left and right") {
    std::vector<std::string> entries = entriesOf(3);
    int selected = 1;
    const auto menu = GridMenu(&entries, &selected, gridOptions(1));
    CHECK_FALSE(press(menu, Event::ArrowLeft));
    CHECK_FALSE(press(menu, Event::ArrowRight));
    CHECK_EQ(selected, 1);
}

TEST_CASE("grid/home and end jump to the first and last entry") {
    std::vector<std::string> entries = entriesOf(5);
    int selected = 2;
    const auto menu = GridMenu(&entries, &selected, gridOptions(2));
    CHECK(press(menu, Event::End));
    CHECK_EQ(selected, 4);
    CHECK(press(menu, Event::Home));
    CHECK_EQ(selected, 0);
}

TEST_CASE("grid/enter reports the selected index") {
    std::vector<std::string> entries = entriesOf(4);
    int selected = 2;
    int entered = -1;
    GridMenuOptions options = gridOptions(2);
    options.on_enter = [&](int index) { entered = index; };
    const auto menu = GridMenu(&entries, &selected, options);

    CHECK(press(menu, Event::Return));
    CHECK_EQ(entered, 2);
}

TEST_CASE("grid/selection changes call on_change") {
    std::vector<std::string> entries = entriesOf(4);
    int selected = 0;
    int changes = 0;
    GridMenuOptions options = gridOptions(2);
    options.on_change = [&]() { ++changes; };
    const auto menu = GridMenu(&entries, &selected, options);

    press(menu, Event::ArrowRight);
    press(menu, Event::ArrowDown);
    CHECK_EQ(changes, 2);
    // The last entry cannot move down, so nothing is reported.
    CHECK_FALSE(press(menu, Event::ArrowDown));
    CHECK_EQ(changes, 2);
}

TEST_CASE("grid/an empty list is not focusable") {
    std::vector<std::string> entries;
    int selected = 0;
    const auto menu = GridMenu(&entries, &selected, gridOptions(2));
    CHECK_FALSE(menu->Focusable());
    CHECK_FALSE(press(menu, Event::ArrowDown));
}

// --- rendering ---------------------------------------------------------------------

TEST_CASE("grid/renders rows of cells") {
    std::vector<std::string> entries = {"aa", "bb", "cc", "dd"};
    int selected = 0;
    GridMenuOptions options = gridOptions(2);
    options.activePrefix = "";
    options.inactivePrefix = "";
    options.columnGap = 2;
    const auto menu = GridMenu(&entries, &selected, options);

    CHECK_EQ(renderPlain(menu->Render()), "aa  bb\ncc  dd");
}

TEST_CASE("grid/only the selected cell gets the active prefix") {
    std::vector<std::string> entries = {"aa", "bb"};
    int selected = 1;
    GridMenuOptions options = gridOptions(2);
    options.columnGap = 1;
    const auto menu = GridMenu(&entries, &selected, options);

    CHECK_EQ(renderPlain(menu->Render()), "  aa > bb");
}

TEST_CASE("grid/cells are truncated to the cell width") {
    std::vector<std::string> entries = {"abcdefgh", "ij"};
    int selected = 0;
    GridMenuOptions options = gridOptions(2);
    options.activePrefix = "";
    options.inactivePrefix = "";
    options.cellWidth = 5;
    options.columnGap = 1;
    const auto menu = GridMenu(&entries, &selected, options);

    const std::string out = renderPlain(menu->Render());
    CHECK_EQ(out.substr(0, 5), "ab...");
}

TEST_CASE("grid/format_lines renders multi-line cells") {
    std::vector<std::string> entries = {"a", "bb", "c", "dd"};
    int selected = 0;
    GridMenuOptions options = gridOptions(2);
    options.activePrefix = "";
    options.inactivePrefix = "";
    options.columnGap = 2;
    options.format_lines = [](int, const std::string& entry) {
        return std::vector<std::string>{entry, "path-" + entry};
    };
    const auto menu = GridMenu(&entries, &selected, options);
    const std::string out = renderPlain(menu->Render());

    // Two rows of two cells, each cell two lines tall.
    CHECK_EQ(std::count(out.begin(), out.end(), '\n'), 3);
    CHECK(out.find("path-a") != std::string::npos);
    CHECK(out.find("path-bb") != std::string::npos);
    CHECK(out.find("path-dd") != std::string::npos);
    // The second cell's continuation line starts under its own column.
    const std::size_t firstRow = out.find('\n');
    const std::size_t secondRow = out.find('\n', firstRow + 1);
    const std::string row1 = out.substr(0, firstRow);
    const std::string row2 = out.substr(firstRow + 1, secondRow - firstRow - 1);
    CHECK_EQ(row1.find("bb"), row2.find("path-bb"));
}

TEST_CASE("grid/focused_format_lines applies to the selected cell") {
    std::vector<std::string> entries = {"a", "b"};
    int selected = 1;
    GridMenuOptions options = gridOptions(2);
    options.activePrefix = "";
    options.inactivePrefix = "";
    options.columnGap = 1;
    options.format_lines = [](int, const std::string& entry) {
        return std::vector<std::string>{entry};
    };
    options.focused_format_lines = [](int, const std::string& entry) {
        return std::vector<std::string>{entry + "!"};
    };
    const auto menu = GridMenu(&entries, &selected, options);

    const std::string out = renderPlain(menu->Render());
    CHECK(out.find("a ") != std::string::npos);
    CHECK(out.find("b!") != std::string::npos);

    selected = 0;
    const std::string moved = renderPlain(menu->Render());
    CHECK(moved.find("a!") != std::string::npos);
}

TEST_CASE("grid/focused_format_lines falls back when empty") {
    std::vector<std::string> entries = {"a", "b"};
    int selected = 0;
    GridMenuOptions options = gridOptions(2);
    options.activePrefix = "";
    options.inactivePrefix = "";
    options.columnGap = 1;
    options.format_lines = [](int, const std::string& entry) {
        return std::vector<std::string>{entry};
    };
    options.focused_format_lines = [](int, const std::string&) {
        return std::vector<std::string>{};
    };
    const auto menu = GridMenu(&entries, &selected, options);
    CHECK_EQ(renderPlain(menu->Render()), "a b");
}

TEST_CASE("grid/focused variant width participates in the column layout") {
    std::vector<std::string> entries = {"aa", "bb"};
    int selected = 0;
    GridMenuOptions options = gridOptions(2);
    options.activePrefix = "";
    options.inactivePrefix = "";
    options.columnGap = 1;
    options.format_lines = [](int, const std::string& entry) {
        return std::vector<std::string>{entry};
    };
    options.focused_format_lines = [](int index, const std::string& entry) {
        return std::vector<std::string>{
            index == 1 ? std::string("focus-long") : entry};
    };
    const auto menu = GridMenu(&entries, &selected, options);

    // The unselected second cell's focused variant is the widest line, so the
    // second column starts after it even though that variant is not drawn.
    const std::string out = renderPlain(menu->Render());
    CHECK_EQ(out.find("bb"), static_cast<std::size_t>(11));
}

TEST_CASE("grid/focused_format_lines follows component focus") {
    std::vector<std::string> entries = {"a"};
    int selected = 0;
    GridMenuOptions options = gridOptions(1);
    options.activePrefix = "";
    options.inactivePrefix = "";
    options.format_lines = [](int, const std::string& entry) {
        return std::vector<std::string>{entry};
    };
    options.focused_format_lines = [](int, const std::string& entry) {
        return std::vector<std::string>{entry + "!"};
    };
    const auto menu = GridMenu(&entries, &selected, options);

    std::string siblingValue;
    const auto sibling = ftxui::Input(&siblingValue);
    int selector = 0;
    const auto container =
        ftxui::Container::Vertical({sibling, menu}, &selector);
    (void)container;

    const std::string unfocused = renderPlain(menu->Render());
    CHECK(unfocused.find('!') == std::string::npos);
    CHECK(unfocused.find('a') != std::string::npos);
    selector = 1;
    CHECK(renderPlain(menu->Render()).find("a!") != std::string::npos);
}

TEST_CASE("grid/cells pad to the column width") {
    std::vector<std::string> entries = {"a", "long-entry"};
    int selected = 0;
    GridMenuOptions options = gridOptions(2);
    options.activePrefix = "";
    options.inactivePrefix = "";
    options.columnGap = 1;
    const auto menu = GridMenu(&entries, &selected, options);
    const std::string out = renderPlain(menu->Render());
    // The first cell is padded so the second column starts at a fixed offset.
    CHECK_EQ(out.find("long-entry"), static_cast<std::size_t>(11));
}

TEST_CASE("grid/renders at a fixed width") {
    std::vector<std::string> entries = entriesOf(4);
    int selected = 0;
    const auto menu = GridMenu(&entries, &selected, gridOptions(2));
    const std::string out = renderAt(menu, 20);
    CHECK(out.find("i1") != std::string::npos);
    CHECK(out.find("i4") != std::string::npos);
    CHECK_EQ(std::count(out.begin(), out.end(), '\n'), 1);
}
