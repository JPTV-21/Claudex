#include "test_framework.hpp"

#include "focus_order.hpp"

using namespace claudex;

TEST_CASE("focus_order/collects path fields in authored order") {
    const json blocks = json::parse(R"([
        {"type": "text"},
        {"type": "vbox", "children": [
            {"type": "pathInput"},
            {"type": "nameInput"}
        ]},
        {"type": "pathInput"}
    ])");

    std::vector<PathField> order;
    collectPathFieldOrder(blocks, order);
    REQUIRE(order.size() == 2);
    CHECK(order[0] == PathField::Path);
    CHECK(order[1] == PathField::Name);
}

TEST_CASE("focus_order/collects main focus targets and drops duplicates") {
    const json blocks = json::parse(R"([
        {"type": "projectList"},
        {"type": "hbox", "children": [
            {"type": "quickInput"},
            {"type": "recentList"},
            {"type": "recentList"}
        ]},
        {"type": "projectList"}
    ])");

    std::vector<int> order;
    collectMainFocusOrder(blocks, order);
    REQUIRE(order.size() == 3);
    CHECK_EQ(order[0], kMainFocusProjects);
    CHECK_EQ(order[1], kMainFocusQuickInput);
    CHECK_EQ(order[2], kMainFocusRecent);
}

TEST_CASE("focus_order/collects the cwd section in authored order") {
    const json blocks = json::parse(R"([
        {"type": "cwdList"},
        {"type": "hbox", "children": [
            {"type": "projectList"},
            {"type": "cwdList"},
            {"type": "quickInput"}
        ]}
    ])");

    std::vector<int> order;
    collectMainFocusOrder(blocks, order);
    REQUIRE(order.size() == 3);
    CHECK_EQ(order[0], kMainFocusCwd);
    CHECK_EQ(order[1], kMainFocusProjects);
    CHECK_EQ(order[2], kMainFocusQuickInput);
}

TEST_CASE("focus_order/ignores malformed block lists") {
    std::vector<int> mainOrder;
    collectMainFocusOrder(json("not an array"), mainOrder);
    CHECK(mainOrder.empty());

    std::vector<PathField> pathOrder;
    collectPathFieldOrder(json::parse(R"([42, {"type": "nameinput"}])"),
                          pathOrder);
    REQUIRE(pathOrder.size() == 1);
    CHECK(pathOrder[0] == PathField::Name);
}

TEST_CASE("focus_order/stops at a pathological nesting depth") {
    json deep = json::parse(R"([{"type": "nameInput"}])");
    for (int level = 0; level < 100; ++level) {
        deep = json::array({json{{"type", "vbox"}, {"children", deep}}});
    }

    std::vector<PathField> order;
    collectPathFieldOrder(deep, order);
    CHECK(order.empty());

    std::vector<int> mainOrder;
    collectMainFocusOrder(deep, mainOrder);
    CHECK(mainOrder.empty());
}

TEST_CASE("focus_order/focusIndexOf reports absent nodes") {
    const std::vector<int> order = {kMainFocusProjects, kMainFocusRecent};
    CHECK_EQ(focusIndexOf(order, kMainFocusProjects), 0);
    CHECK_EQ(focusIndexOf(order, kMainFocusQuickInput), -1);
    CHECK_EQ(focusIndexOf({}, kMainFocusProjects), -1);
}

TEST_CASE("focus_order/moveEnabledIndex wraps and skips disabled slots") {
    const std::vector<bool> enabled = {true, false, true, true};
    CHECK_EQ(moveEnabledIndex(enabled, 0, 1, true), 2);
    CHECK_EQ(moveEnabledIndex(enabled, 2, 1, true), 3);
    CHECK_EQ(moveEnabledIndex(enabled, 3, 1, true), 0);
    CHECK_EQ(moveEnabledIndex(enabled, 0, -1, true), 3);
    CHECK_EQ(moveEnabledIndex(enabled, 3, -1, true), 2);
}

TEST_CASE("focus_order/moveEnabledIndex respects the no-wrap boundary") {
    const std::vector<bool> enabled = {true, false, true};
    CHECK_EQ(moveEnabledIndex(enabled, 0, 1, false), 2);
    CHECK_EQ(moveEnabledIndex(enabled, 0, -1, false), -1);
    CHECK_EQ(moveEnabledIndex(enabled, 2, 1, false), -1);
    CHECK_EQ(moveEnabledIndex({}, 0, 1, true), -1);
    CHECK_EQ(moveEnabledIndex({false, false}, 0, 1, true), -1);
}

TEST_CASE("focus_order/moveEnabledIndex starts from the first entry when the "
          "current index is disabled") {
    const std::vector<bool> enabled = {true, true, false};
    CHECK_EQ(moveEnabledIndex(enabled, 2, 1, true), 1);
    CHECK_EQ(moveEnabledIndex(enabled, 2, -1, true), 1);
}
