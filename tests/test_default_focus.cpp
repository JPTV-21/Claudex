#include "test_framework.hpp"

#include "default_focus.hpp"

using namespace claudex;

namespace {

const std::vector<int> kAllSections = {
    kMainFocusProjects, kMainFocusRecent, kMainFocusQuickInput};

DefaultFocus focus(const std::string& section, int item) {
    DefaultFocus value;
    value.section = section;
    value.item = item;
    return value;
}

DefaultFocus projectFocus(const std::string& section,
                          const std::string& project, int item = 0) {
    DefaultFocus value = focus(section, item);
    value.match = project;
    return value;
}

Project project(const std::string& path, const std::string& name = "") {
    Project value;
    value.path = path;
    value.name = name;
    return value;
}

}  // namespace

TEST_CASE("default_focus/parses a section, project and item object") {
    const auto parsed = defaultFocusFromJson(
        json::parse(R"({"section": "recent", "match": "F:/work/app",
                        "item": 2})"));
    REQUIRE(parsed.has_value());
    CHECK_EQ(parsed->section, "recent");
    CHECK_EQ(parsed->match, "F:/work/app");
    CHECK_EQ(parsed->item, 2);
}

TEST_CASE("default_focus/uses defaults for a missing section or item") {
    const auto parsed = defaultFocusFromJson(json::parse("{}"));
    REQUIRE(parsed.has_value());
    CHECK(parsed->section.empty());
    CHECK_EQ(parsed->item, 0);
}

TEST_CASE("default_focus/rejects values that are not an object") {
    CHECK_FALSE(defaultFocusFromJson(json("recent")).has_value());
    CHECK_FALSE(defaultFocusFromJson(json::array()).has_value());
    CHECK_FALSE(defaultFocusFromJson(json(nullptr)).has_value());
}

TEST_CASE("default_focus/round-trips through JSON") {
    const DefaultFocus original = projectFocus("recent", "F:/work/app", -1);
    const auto parsed = defaultFocusFromJson(defaultFocusToJson(original));
    REQUIRE(parsed.has_value());
    CHECK_EQ(parsed->section, original.section);
    CHECK_EQ(parsed->match, original.match);
    CHECK_EQ(parsed->item, original.item);
}

TEST_CASE("default_focus/accepts one object or an array of targets") {
    const auto single = defaultFocusListFromJson(
        json::parse(R"({"section": "recent", "match": "a"})"));
    REQUIRE(single.size() == 1);
    CHECK_EQ(single[0].section, "recent");

    const auto many = defaultFocusListFromJson(json::parse(R"([
        {"section": "projects", "match": "a"},
        "not a target",
        {"section": "actions", "match": "edit"},
        [1, 2]
    ])"));
    REQUIRE(many.size() == 2);
    CHECK_EQ(many[0].section, "projects");
    CHECK_EQ(many[1].section, "actions");

    CHECK(defaultFocusListFromJson(json("bad")).empty());
    CHECK(defaultFocusListFromJson(json::array()).empty());
}

TEST_CASE("default_focus/serializes one target as an object and several as an "
          "array") {
    DefaultFocus one = projectFocus("recent", "a", 1);
    const json single = defaultFocusListToJson({one});
    CHECK(single.is_object());
    CHECK_EQ(single.at("section").get<std::string>(), "recent");

    DefaultFocus two = projectFocus("actions", "edit");
    const json many = defaultFocusListToJson({one, two});
    REQUIRE(many.is_array());
    REQUIRE(many.size() == 2);
    CHECK_EQ(many[1].at("match").get<std::string>(), "edit");
}

TEST_CASE("default_focus/finds the target owned by each page") {
    const std::vector<DefaultFocus> targets = {
        projectFocus("actions", "edit"),
        projectFocus("recent", "F:/work/app", 1),
        projectFocus("path", "name"),
        projectFocus("projects", "", 2),
    };

    const DefaultFocus* main = mainFocusTarget(targets);
    REQUIRE(main != nullptr);
    CHECK_EQ(main->section, "recent");

    const DefaultFocus* actions = actionFocusTarget(targets);
    REQUIRE(actions != nullptr);
    CHECK_EQ(actions->match, "edit");

    const DefaultFocus* path = pathFocusTarget(targets);
    REQUIRE(path != nullptr);
    CHECK_EQ(path->section, "path");

    CHECK(mainFocusTarget({}) == nullptr);
    CHECK(actionFocusTarget({}) == nullptr);
    CHECK(pathFocusTarget({}) == nullptr);
}

TEST_CASE("default_focus/matches a project by path regardless of row order") {
    const std::vector<Project> projects = {
        project("a"), project("b"), project("c")};
    // The positional item says the first row, but the selector targets "c".
    const FocusChoice choice =
        resolveDefaultFocus(projectFocus("projects", "c", 0), kAllSections, 3,
                            0, projects, {});
    CHECK_EQ(choice.node, kMainFocusProjects);
    CHECK_EQ(choice.item, 2);
}

TEST_CASE("default_focus/matches a project by display name") {
    const std::vector<Project> projects = {
        project("a", "<accent>Alpha</accent>"), project("b", "Beta")};
    const FocusChoice choice =
        resolveDefaultFocus(projectFocus("projects", "Beta"), kAllSections, 2,
                            0, projects, {});
    CHECK_EQ(choice.item, 1);
}

TEST_CASE("default_focus/identity survives a reordered recent list") {
    const std::vector<Project> projects = {
        project("a"), project("b"), project("c")};
    const DefaultFocus selector = projectFocus("recent", "a");

    const FocusChoice first = resolveDefaultFocus(
        selector, kAllSections, 0, 3, projects, {2, 0, 1});
    CHECK_EQ(first.node, kMainFocusRecent);
    CHECK_EQ(first.item, 1);

    const FocusChoice reordered = resolveDefaultFocus(
        selector, kAllSections, 0, 3, projects, {0, 2, 1});
    CHECK_EQ(reordered.node, kMainFocusRecent);
    CHECK_EQ(reordered.item, 0);
}

TEST_CASE("default_focus/an identity selector still matches after falling "
          "back to the pinned list") {
    const std::vector<Project> projects = {project("a"), project("b")};
    const FocusChoice choice = resolveDefaultFocus(
        projectFocus("recent", "b"), kAllSections, 2, 0, projects, {});
    CHECK_EQ(choice.node, kMainFocusProjects);
    CHECK_EQ(choice.item, 1);
}

TEST_CASE("default_focus/falls back to the positional item when the selector "
          "does not match") {
    const std::vector<Project> projects = {project("a"), project("b")};
    const FocusChoice choice =
        resolveDefaultFocus(projectFocus("projects", "missing", 1),
                            kAllSections, 2, 0, projects, {});
    CHECK_EQ(choice.node, kMainFocusProjects);
    CHECK_EQ(choice.item, 1);
}

TEST_CASE("default_focus/matches the add-project row by name") {
    const std::vector<Project> projects = {project("a"), project("b")};
    // Two projects plus the "+ Add project" row.
    const FocusChoice choice = resolveDefaultFocus(
        projectFocus("projects", "add"), kAllSections, 3, 0, projects, {});
    CHECK_EQ(choice.node, kMainFocusProjects);
    CHECK_EQ(choice.item, 2);

    // Without the add row the selector does not match and item 0 wins.
    const FocusChoice withoutAdd = resolveDefaultFocus(
        projectFocus("projects", "add"), kAllSections, 2, 0, projects, {});
    CHECK_EQ(withoutAdd.item, 0);
}

TEST_CASE("default_focus/normalizes path separators for matching") {
    const std::vector<Project> projects = {project("F:/work/app")};
    const FocusChoice choice = resolveDefaultFocus(
        projectFocus("projects", "F:\\work\\app"), kAllSections, 1, 0,
        projects, {});
    CHECK_EQ(choice.node, kMainFocusProjects);
    CHECK_EQ(choice.item, 0);
}

TEST_CASE("default_focus/selects a project row") {
    const FocusChoice choice =
        resolveDefaultFocus(focus("projects", 2), kAllSections, 4, 2);
    CHECK_EQ(choice.node, kMainFocusProjects);
    CHECK_EQ(choice.item, 2);
}

TEST_CASE("default_focus/clamps an out-of-range item and treats a negative "
          "value as the last row") {
    CHECK_EQ(resolveDefaultFocus(focus("projects", 99), kAllSections, 4, 2)
                 .item,
             3);
    CHECK_EQ(resolveDefaultFocus(focus("projects", -1), kAllSections, 4, 2)
                 .item,
             3);
    CHECK_EQ(resolveDefaultFocus(focus("recent", -7), kAllSections, 4, 2)
                 .item,
             1);
}

TEST_CASE("default_focus/selects a recent row") {
    const FocusChoice choice =
        resolveDefaultFocus(focus("recent", 1), kAllSections, 4, 3);
    CHECK_EQ(choice.node, kMainFocusRecent);
    CHECK_EQ(choice.item, 1);
}

TEST_CASE("default_focus/falls back to projects when recent is empty") {
    const FocusChoice choice =
        resolveDefaultFocus(focus("recent", 1), kAllSections, 4, 0);
    CHECK_EQ(choice.node, kMainFocusProjects);
    CHECK_EQ(choice.item, 0);
}

TEST_CASE("default_focus/focuses the quick input and ignores its item") {
    const FocusChoice choice =
        resolveDefaultFocus(focus("quickInput", 5), kAllSections, 4, 3);
    CHECK_EQ(choice.node, kMainFocusQuickInput);
    CHECK_EQ(choice.item, 0);
}

TEST_CASE("default_focus/falls back when the section is not in the layout") {
    const std::vector<int> withoutRecent = {kMainFocusProjects,
                                            kMainFocusQuickInput};
    const FocusChoice choice =
        resolveDefaultFocus(focus("recent", 1), withoutRecent, 4, 3);
    CHECK_EQ(choice.node, kMainFocusProjects);
}

TEST_CASE("default_focus/accepts section aliases") {
    CHECK_EQ(resolveDefaultFocus(focus("ProjectList", 0), kAllSections, 4, 2)
                 .node,
             kMainFocusProjects);
    CHECK_EQ(resolveDefaultFocus(focus("recent-projects", 0), kAllSections, 4,
                                 2)
                 .node,
             kMainFocusRecent);
    CHECK_EQ(resolveDefaultFocus(focus("quick launch", 0), kAllSections, 4, 2)
                 .node,
             kMainFocusQuickInput);
}

TEST_CASE("default_focus/treats an unknown section as the built-in default") {
    const FocusChoice choice =
        resolveDefaultFocus(focus("actions", 1), kAllSections, 4, 2);
    CHECK_EQ(choice.node, kMainFocusProjects);
    CHECK_EQ(choice.item, 0);
}

TEST_CASE("default_focus/uses the first available section when the "
          "requested one has no rows") {
    const FocusChoice choice =
        resolveDefaultFocus(focus("projects", 0), kAllSections, 0, 0);
    CHECK_EQ(choice.node, kMainFocusQuickInput);
    CHECK_EQ(choice.item, 0);
}

TEST_CASE("default_focus/returns nothing when the screen has no sections") {
    const FocusChoice choice =
        resolveDefaultFocus(focus("projects", 0), {}, 4, 2);
    CHECK_EQ(choice.node, -1);
}

TEST_CASE("default_focus/classifies every focusable section") {
    CHECK(focusSectionFromText("projects") == FocusSection::Projects);
    CHECK(focusSectionFromText("ProjectList") == FocusSection::Projects);
    CHECK(focusSectionFromText("recent-projects") == FocusSection::Recent);
    CHECK(focusSectionFromText("quick launch") == FocusSection::QuickInput);
    CHECK(focusSectionFromText("actionList") == FocusSection::Actions);
    CHECK(focusSectionFromText("projectMenu") == FocusSection::Actions);
    CHECK(focusSectionFromText("nameInput") == FocusSection::PathName);
    CHECK(focusSectionFromText("path") == FocusSection::PathPath);
    CHECK(focusSectionFromText("unknown") == FocusSection::Projects);
}

TEST_CASE("default_focus/a non-main section leaves the main focus alone") {
    // An action or path target is applied by its own page. It must not borrow
    // its item index for the project list.
    DefaultFocus target = projectFocus("actions", "edit", 2);
    const FocusChoice actions = resolveDefaultFocus(
        target, kAllSections, 4, 2, {project("a"), project("b")}, {});
    CHECK_EQ(actions.node, kMainFocusProjects);
    CHECK_EQ(actions.item, 0);

    target = projectFocus("path", "name", 3);
    const FocusChoice path = resolveDefaultFocus(
        target, kAllSections, 4, 2, {project("a"), project("b")}, {});
    CHECK_EQ(path.node, kMainFocusProjects);
    CHECK_EQ(path.item, 0);
}

TEST_CASE("default_focus/matches an action by id or label") {
    std::vector<MenuAction> actions;
    actions.push_back({"launch", "Launch", ""});
    actions.push_back({"edit", "Edit project", ""});
    actions.push_back({"remove", "Remove", ""});

    DefaultFocus target = projectFocus("actions", "edit");
    CHECK_EQ(actionFocusIndex(actions, target), 1);

    target.match = "Remove";
    CHECK_EQ(actionFocusIndex(actions, target), 2);
}

TEST_CASE("default_focus/an action selector falls back to the item") {
    std::vector<MenuAction> actions;
    actions.push_back({"launch", "Launch", ""});
    actions.push_back({"edit", "Edit", ""});
    actions.push_back({"remove", "Remove", ""});

    DefaultFocus target = projectFocus("actions", "missing", -1);
    CHECK_EQ(actionFocusIndex(actions, target), 2);

    target.item = 99;
    CHECK_EQ(actionFocusIndex(actions, target), 2);

    target.item = 0;
    CHECK_EQ(actionFocusIndex(actions, target), 0);
    CHECK_EQ(actionFocusIndex({}, target), 0);
}
