#include "recent_list.hpp"

#include <algorithm>

namespace claudex {

std::vector<int> recentProjectIndices(const std::vector<Project>& projects,
                                      int limit) {
    std::vector<int> indices;
    if (limit <= 0) {
        return indices;
    }

    indices.reserve(projects.size());
    for (int index = 0; index < static_cast<int>(projects.size()); ++index) {
        if (!projects[static_cast<std::size_t>(index)].lastUsed.empty()) {
            indices.push_back(index);
        }
    }

    // Stable so equal timestamps keep the authored project order.
    std::stable_sort(indices.begin(), indices.end(), [&](int lhs, int rhs) {
        return projects[static_cast<std::size_t>(lhs)].lastUsed >
               projects[static_cast<std::size_t>(rhs)].lastUsed;
    });
    if (static_cast<int>(indices.size()) > limit) {
        indices.resize(static_cast<std::size_t>(limit));
    }
    return indices;
}

int keptSelectionIndex(const std::vector<int>& previousIndices,
                       int previousSelected,
                       const std::vector<int>& nextIndices) {
    const int kept =
        previousSelected >= 0 &&
                previousSelected < static_cast<int>(previousIndices.size())
            ? previousIndices[static_cast<std::size_t>(previousSelected)]
            : -1;
    const auto found = std::find(nextIndices.begin(), nextIndices.end(), kept);
    if (found != nextIndices.end()) {
        return static_cast<int>(found - nextIndices.begin());
    }
    if (nextIndices.empty()) {
        return 0;
    }
    return std::clamp(previousSelected, 0,
                      static_cast<int>(nextIndices.size()) - 1);
}

}  // namespace claudex
