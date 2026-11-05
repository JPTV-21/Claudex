#include "test_framework.hpp"

#include "recent_list.hpp"

using namespace claudex;

namespace {

Project project(const std::string& path, const std::string& lastUsed) {
    Project item;
    item.path = path;
    item.lastUsed = lastUsed;
    return item;
}

}  // namespace

TEST_CASE("recent_list/orders by lastUsed and skips never-launched projects") {
    const std::vector<Project> projects = {
        project("a", "2026-01-01T00:00:00"),
        project("b", ""),
        project("c", "2026-03-01T00:00:00"),
        project("d", "2026-02-01T00:00:00"),
    };
    CHECK_EQ(recentProjectIndices(projects, 5),
             (std::vector<int>{2, 3, 0}));
}

TEST_CASE("recent_list/caps the list and handles non-positive limits") {
    const std::vector<Project> projects = {
        project("a", "1"), project("b", "2"), project("c", "3"),
    };
    CHECK_EQ(recentProjectIndices(projects, 2), (std::vector<int>{2, 1}));
    CHECK(recentProjectIndices(projects, 0).empty());
    CHECK(recentProjectIndices(projects, -1).empty());
}

TEST_CASE("recent_list/keeps equal timestamps in project order") {
    const std::vector<Project> projects = {
        project("a", "2026-01-01T00:00:00"),
        project("b", "2026-01-01T00:00:00"),
        project("c", "2026-02-01T00:00:00"),
    };
    CHECK_EQ(recentProjectIndices(projects, 5),
             (std::vector<int>{2, 0, 1}));
}

TEST_CASE("recent_list/selection follows the same project across refreshes") {
    // The selected row showed project 3; it moved to row 0 after refresh.
    CHECK_EQ(keptSelectionIndex({2, 3, 0}, 1, {3, 2, 0}), 0);
    // The project was removed: clamp the old row index.
    CHECK_EQ(keptSelectionIndex({2, 3, 0}, 2, {3, 2}), 1);
    CHECK_EQ(keptSelectionIndex({2, 3, 0}, 0, {}), 0);
    // No previous selection.
    CHECK_EQ(keptSelectionIndex({}, 0, {4, 5}), 0);
}
