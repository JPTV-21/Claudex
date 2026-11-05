#pragma once

#include "config_types.hpp"

#include <vector>

namespace claudex {

// Indices of the projects that have been launched, newest first, capped at
// |limit|. Projects without a lastUsed value never appear.
std::vector<int> recentProjectIndices(const std::vector<Project>& projects,
                                      int limit);

// Selection for a refreshed list. The previously selected project keeps its
// row when it is still present; otherwise the old row index is clamped to the
// new list.
int keptSelectionIndex(const std::vector<int>& previousIndices,
                       int previousSelected,
                       const std::vector<int>& nextIndices);

}  // namespace claudex
