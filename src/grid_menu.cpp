#include "grid_menu.hpp"

#include "menu_markup.hpp"

#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/terminal.hpp>

#include <algorithm>
#include <utility>

using namespace ftxui;

namespace claudex {

int gridColumnCount(int availableWidth, int cellWidth, int gap, int entryCount) {
    if (entryCount <= 0) {
        return 1;
    }
    if (availableWidth <= 0 || cellWidth <= 0) {
        return 1;
    }
    const int step = cellWidth + std::max(0, gap);
    const int columns = std::max(1, (availableWidth + std::max(0, gap)) / step);
    return std::min(columns, entryCount);
}

GridMenuComponent::GridMenuComponent(std::vector<std::string>* entries,
                                     int* selected, GridMenuOptions options)
    : entries_(entries), selected_(selected), options_(std::move(options)) {}

bool GridMenuComponent::Focusable() const {
    return entries_ != nullptr && !entries_->empty();
}

int GridMenuComponent::columns() const { return std::max(1, columns_); }

int GridMenuComponent::CellAt(const Mouse& mouse) const {
    for (std::size_t i = 0; i < cells_.size(); ++i) {
        if (!cells_[i].IsEmpty() && cells_[i].Contain(mouse.x, mouse.y)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

bool GridMenuComponent::Inside(const Mouse& mouse) const {
    return !listBox_.IsEmpty() && listBox_.Contain(mouse.x, mouse.y);
}

int GridMenuComponent::ResolveColumns(int cellWidth, int entryCount) const {
    if (!options_.autoColumns) {
        return std::clamp(options_.columns, 1, std::max(1, entryCount));
    }
    const int available = options_.availableWidth > 0
                              ? options_.availableWidth
                              : Terminal::Size().dimx;
    return gridColumnCount(available, cellWidth, options_.columnGap,
                           entryCount);
}

std::vector<std::string> GridMenuComponent::EntryLines(
    int index, const std::string& entry) const {
    if (options_.format_lines) {
        std::vector<std::string> lines = options_.format_lines(index, entry);
        if (lines.empty()) {
            lines.push_back(entry);
        }
        return lines;
    }
    return {options_.format ? options_.format(index, entry) : entry};
}

// Keys can arrive before the first draw; measure the labels the same way the
// renderer does so navigation already knows the row length.
void GridMenuComponent::EnsureColumns() {
    if (columns_ > 0 || entries_ == nullptr || entries_->empty()) {
        return;
    }
    int widest = 0;
    for (int i = 0; i < static_cast<int>(entries_->size()); ++i) {
        const std::string& entry = (*entries_)[static_cast<std::size_t>(i)];
        const std::vector<std::string> lines = EntryLines(i, entry);
        for (const std::string& line : lines) {
            widest = std::max(
                widest, menuMarkupDisplayWidth(line, options_.variables));
        }
    }
    const int prefixWidth =
        std::max(menuMarkupDisplayWidth(options_.activePrefix,
                                        options_.variables),
                 menuMarkupDisplayWidth(options_.inactivePrefix,
                                        options_.variables));
    const int cellWidth = options_.cellWidth > 0
                              ? options_.cellWidth
                              : widest + prefixWidth;
    columns_ = ResolveColumns(std::max(1, cellWidth),
                              static_cast<int>(entries_->size()));
}

void GridMenuComponent::Select(int index) {
    if (*selected_ == index) {
        return;
    }
    *selected_ = index;
    if (options_.on_change) {
        options_.on_change();
    }
}

Element GridMenuComponent::OnRender() {
    const std::vector<std::string>& entries = *entries_;
    if (entries.empty()) {
        return text("");
    }

    std::vector<std::vector<std::string>> labels;
    labels.reserve(entries.size());
    int widest = 0;
    for (int i = 0; i < static_cast<int>(entries.size()); ++i) {
        labels.push_back(EntryLines(i, entries[static_cast<std::size_t>(i)]));
        for (const std::string& line : labels.back()) {
            widest = std::max(
                widest, menuMarkupDisplayWidth(line, options_.variables));
        }
    }

    const int prefixWidth =
        std::max(menuMarkupDisplayWidth(options_.activePrefix,
                                        options_.variables),
                 menuMarkupDisplayWidth(options_.inactivePrefix,
                                        options_.variables));
    int cellWidth = options_.cellWidth;
    if (cellWidth <= 0) {
        cellWidth = widest + prefixWidth;
    }
    cellWidth = std::max(1, cellWidth);

    const int columns =
        ResolveColumns(cellWidth, static_cast<int>(entries.size()));
    columns_ = columns;

    const int selected =
        std::clamp(*selected_, 0, static_cast<int>(entries.size()) - 1);
    cells_.assign(entries.size(), Box{});
    const bool focused = Focused();

    Elements rows;
    for (int row = 0; row * columns < static_cast<int>(entries.size()); ++row) {
        Elements rowCells;
        for (int column = 0; column < columns; ++column) {
            const int index = row * columns + column;
            if (index >= static_cast<int>(entries.size())) {
                rowCells.push_back(filler());
            } else {
                const EntryState state{
                    labels[static_cast<std::size_t>(index)].front(), false,
                    index == selected, focused, index};
                Element cell = styleMenuEntryLines(
                    state, labels[static_cast<std::size_t>(index)], true,
                    focused, options_.activePrefix, options_.inactivePrefix,
                    options_.inverseHighlight, options_.variables, cellWidth);
                // Equal, not a maximum: every cell pads to the column width so
                // columns line up and continuation lines start at the same
                // offset.
                cell = cell | size(WIDTH, EQUAL, cellWidth) |
                       reflect(cells_[static_cast<std::size_t>(index)]);
                if (index == selected) {
                    // Lets the enclosing frame scroll to the selection.
                    cell = cell | focus;
                }
                rowCells.push_back(std::move(cell));
            }
            if (column + 1 < columns) {
                rowCells.push_back(
                    text(std::string(static_cast<std::size_t>(
                                         std::max(0, options_.columnGap)),
                                     ' ')));
            }
        }
        rows.push_back(hbox(std::move(rowCells)));
    }

    return vbox(std::move(rows)) | reflect(listBox_);
}

bool GridMenuComponent::OnEvent(Event event) {
    if (entries_ == nullptr || entries_->empty()) {
        return false;
    }
    const int size = static_cast<int>(entries_->size());
    const int current = std::clamp(*selected_, 0, size - 1);

    if (event.is_mouse()) {
        return OnMouse(event.mouse(), current, size);
    }

    EnsureColumns();

    if (event == Event::ArrowLeft) {
        if (columns_ <= 1 || current % columns_ == 0) {
            return false;
        }
        Select(current - 1);
        return true;
    }
    if (event == Event::ArrowRight) {
        if (columns_ <= 1 || current % columns_ == columns_ - 1 ||
            current + 1 >= size) {
            return false;
        }
        Select(current + 1);
        return true;
    }
    if (event == Event::ArrowUp) {
        if (current - columns_ < 0) {
            return false;
        }
        Select(current - columns_);
        return true;
    }
    if (event == Event::ArrowDown) {
        if (current + columns_ < size) {
            Select(current + columns_);
            return true;
        }
        // The last row may be short: move to its only entry instead of
        // handing the key to the container.
        if (current / columns_ != (size - 1) / columns_) {
            Select(size - 1);
            return true;
        }
        return false;
    }
    if (event == Event::Home) {
        Select(0);
        return true;
    }
    if (event == Event::End) {
        Select(size - 1);
        return true;
    }
    if (event == Event::PageUp) {
        Select(std::max(0, current - columns_ * std::max(1, options_.pageRows)));
        return true;
    }
    if (event == Event::PageDown) {
        Select(std::min(size - 1,
                        current + columns_ * std::max(1, options_.pageRows)));
        return true;
    }
    if (event == Event::Return) {
        if (options_.on_enter) {
            options_.on_enter(current);
        }
        return true;
    }
    return false;
}

bool GridMenuComponent::OnMouse(const Mouse& mouse, int current, int size) {
    const int cell = CellAt(mouse);

    if (mouse.button == Mouse::WheelUp || mouse.button == Mouse::WheelDown) {
        if (!Inside(mouse)) {
            return false;
        }
        TakeFocus();
        const int step = mouse.button == Mouse::WheelUp ? -columns_ : columns_;
        Select(std::clamp(current + step, 0, size - 1));
        // Always consume: the wheel must not switch sections.
        return true;
    }

    if (mouse.button == Mouse::None && mouse.motion == Mouse::Moved) {
        if (cell < 0) {
            return Inside(mouse);
        }
        TakeFocus();
        Select(cell);
        return true;
    }

    if (mouse.button == Mouse::Left && mouse.motion == Mouse::Pressed) {
        if (cell < 0) {
            return Inside(mouse);
        }
        TakeFocus();
        Select(cell);
        if (options_.on_enter) {
            options_.on_enter(cell);
        }
        return true;
    }

    if (mouse.button == Mouse::Right && mouse.motion == Mouse::Pressed) {
        if (cell < 0) {
            return Inside(mouse);
        }
        TakeFocus();
        Select(cell);
        if (options_.on_right_click) {
            options_.on_right_click(cell);
        }
        return true;
    }

    return Inside(mouse);
}

Component GridMenu(std::vector<std::string>* entries, int* selected,
                   GridMenuOptions options) {
    return Make<GridMenuComponent>(entries, selected, std::move(options));
}

}  // namespace claudex
