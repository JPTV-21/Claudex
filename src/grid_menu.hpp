#pragma once

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace claudex {

struct GridMenuOptions {
    // >0 is a fixed column count; 0 with autoColumns fills the width instead.
    int columns = 1;
    bool autoColumns = false;
    int cellWidth = 0;  // 0 = widen to the widest entry
    int columnGap = 2;
    int pageRows = 5;
    std::string activePrefix;
    std::string inactivePrefix;
    bool inverseHighlight = true;
    // 0 means "ask the terminal"; the launcher passes the page width.
    int availableWidth = 0;
    std::function<std::string(int, const std::string&)> format;
    // Multi-line entries: takes precedence over |format| when set.
    std::function<std::vector<std::string>(int, const std::string&)> format_lines;
    // Extra line set for the selected cell while the grid owns the focus. An
    // empty result falls back to |format_lines| so a focused row never loses
    // its styling. Both variants are measured for the column width, which
    // keeps the layout stable when the selection moves.
    std::function<std::vector<std::string>(int, const std::string&)>
        focused_format_lines;
    std::function<void()> on_change;
    std::function<void(int)> on_enter;
    std::function<void(int)> on_right_click;
    const std::map<std::string, std::string>* variables = nullptr;
};

// How many cells of |cellWidth| plus |gap| fit in |availableWidth|. Always at
// least one column, never more than |entryCount|.
int gridColumnCount(int availableWidth, int cellWidth, int gap, int entryCount);

// A menu laid out in rows of N cells with two-dimensional arrow navigation.
// Up/Down move by a row, Left/Right within a row; the event is only left to
// the parent container at the first and last entry, so the list keeps
// scrolling until it really ends.
//
// The class is public because the launcher routes mouse events itself: it
// delegates hits inside the list to this component so hovering works before
// the list owns the keyboard focus.
class GridMenuComponent : public ftxui::ComponentBase {
public:
    GridMenuComponent(std::vector<std::string>* entries, int* selected,
                      GridMenuOptions options);

    bool Focusable() const override;
    ftxui::Element OnRender() override;
    bool OnEvent(ftxui::Event event) override;

    // Entry under the pointer, or -1.
    int CellAt(const ftxui::Mouse& mouse) const;
    bool Inside(const ftxui::Mouse& mouse) const;
    int columns() const;

private:
    std::vector<std::string> EntryLines(int index, const std::string& entry,
                                        bool focused) const;
    int ResolveColumns(int cellWidth, int entryCount) const;
    void EnsureColumns();
    void Select(int index);
    bool OnMouse(const ftxui::Mouse& mouse, int current, int size);

    std::vector<std::string>* entries_ = nullptr;
    int* selected_ = nullptr;
    GridMenuOptions options_;
    std::vector<ftxui::Box> cells_;
    ftxui::Box listBox_{};
    int columns_ = 0;  // 0 = not measured yet
};

ftxui::Component GridMenu(std::vector<std::string>* entries, int* selected,
                          GridMenuOptions options);

}  // namespace claudex
