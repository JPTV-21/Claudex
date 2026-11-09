#pragma once

#include "config_types.hpp"

#include <vector>

namespace claudex {

// Focus targets on the main page. The authored block order decides how they
// are traversed, so the constants only identify the components.
inline constexpr int kMainFocusProjects = 0;
inline constexpr int kMainFocusRecent = 1;
inline constexpr int kMainFocusQuickInput = 2;
inline constexpr int kMainFocusCwd = 3;

// Input fields on the path screen. The theme decides which ones exist and in
// which order, so Up/Down and Tab follow the drawn layout.
enum class PathField { Name, Path };

// Collects focus targets in the order the theme authored the blocks.
void collectPathFieldOrder(const json& blocks, std::vector<PathField>& order);
void collectMainFocusOrder(const json& blocks, std::vector<int>& order);

// Index of |node| in |order|, or -1 when it is not present.
int focusIndexOf(const std::vector<int>& order, int node);

// Next index within |enabled| from |current| moved by |step|. Indices that
// are not enabled are skipped. Returns -1 when nothing is enabled or when the
// move would leave the range without wrapping.
int moveEnabledIndex(const std::vector<bool>& enabled, int current, int step,
                     bool wrap);

}  // namespace claudex
