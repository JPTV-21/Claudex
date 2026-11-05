#include "default_focus.hpp"

#include "text_util.hpp"

#include <algorithm>

namespace claudex {

namespace {

bool isOneOf(const std::string& token,
             std::initializer_list<const char*> names) {
    for (const char* name : names) {
        if (token == name) {
            return true;
        }
    }
    return false;
}

// Case-insensitive on Windows, like the platform's path rules. Separators are
// normalized so a selector copied from either config or Explorer matches.
std::string matchKey(std::string value) {
    value = trim(std::move(value));
    std::replace(value.begin(), value.end(), '\\', '/');
#ifdef _WIN32
    value = lowerAscii(std::move(value));
#endif
    return value;
}

bool matchesProject(const Project& project, const std::string& selector) {
    const std::string target = matchKey(selector);
    if (target.empty()) {
        return false;
    }
    return matchKey(project.path) == target ||
           matchKey(project.name) == target;
}

}  // namespace

FocusSection focusSectionFromText(const std::string& spec) {
    const std::string token = normalizeMenuToken(spec);
    if (isOneOf(token, {"recent", "recents", "recentlist",
                        "recentprojects"})) {
        return FocusSection::Recent;
    }
    if (isOneOf(token, {"quickinput", "quicklaunch", "quick", "input"})) {
        return FocusSection::QuickInput;
    }
    if (isOneOf(token, {"actions", "action", "actionlist",
                        "projectactions", "projectmenu"})) {
        return FocusSection::Actions;
    }
    if (isOneOf(token, {"name", "nameinput", "projectname"})) {
        return FocusSection::PathName;
    }
    if (isOneOf(token, {"path", "pathinput", "folder", "folderpath"})) {
        return FocusSection::PathPath;
    }
    // "projects", its aliases, an empty value, and unknown values all keep
    // the built-in default: the project list.
    return FocusSection::Projects;
}

std::optional<DefaultFocus> defaultFocusFromJson(const json& value) {
    if (!value.is_object()) {
        return std::nullopt;
    }
    DefaultFocus focus;
    focus.section = jsonStringField(value, "section", "");
    focus.match = jsonStringField(value, "match", "");
    focus.item = jsonIntField(value, "item", 0);
    return focus;
}

json defaultFocusToJson(const DefaultFocus& focus) {
    json document;
    document["section"] = focus.section;
    document["match"] = focus.match;
    document["item"] = focus.item;
    return document;
}

std::vector<DefaultFocus> defaultFocusListFromJson(const json& value) {
    std::vector<DefaultFocus> targets;
    const auto append = [&](const json& item) {
        if (auto focus = defaultFocusFromJson(item)) {
            targets.push_back(std::move(*focus));
        }
    };
    if (value.is_object()) {
        append(value);
    } else if (value.is_array()) {
        for (const json& item : value) {
            append(item);
        }
    }
    return targets;
}

json defaultFocusListToJson(const std::vector<DefaultFocus>& targets) {
    if (targets.size() == 1) {
        return defaultFocusToJson(targets.front());
    }
    json array = json::array();
    for (const DefaultFocus& focus : targets) {
        array.push_back(defaultFocusToJson(focus));
    }
    return array;
}

const DefaultFocus* mainFocusTarget(
    const std::vector<DefaultFocus>& targets) {
    for (const DefaultFocus& focus : targets) {
        const FocusSection section = focusSectionFromText(focus.section);
        if (section == FocusSection::Projects ||
            section == FocusSection::Recent ||
            section == FocusSection::QuickInput) {
            return &focus;
        }
    }
    return nullptr;
}

const DefaultFocus* actionFocusTarget(
    const std::vector<DefaultFocus>& targets) {
    for (const DefaultFocus& focus : targets) {
        if (focusSectionFromText(focus.section) == FocusSection::Actions) {
            return &focus;
        }
    }
    return nullptr;
}

const DefaultFocus* pathFocusTarget(
    const std::vector<DefaultFocus>& targets) {
    for (const DefaultFocus& focus : targets) {
        const FocusSection section = focusSectionFromText(focus.section);
        if (section == FocusSection::PathName ||
            section == FocusSection::PathPath) {
            return &focus;
        }
    }
    return nullptr;
}

int resolveFocusRow(const DefaultFocus& focus, int count,
                    const std::function<int(const std::string&)>& matcher) {
    if (count <= 0) {
        return 0;
    }
    if (!focus.match.empty() && matcher) {
        const int row = matcher(focus.match);
        if (row >= 0 && row < count) {
            return row;
        }
    }
    // Negative means "last row"; anything past the end clamps so the config
    // keeps working when the list shrinks.
    return focus.item < 0 ? count - 1 : std::clamp(focus.item, 0, count - 1);
}

int projectRowForMatch(const std::vector<Project>& projects,
                       const std::string& selector) {
    for (int row = 0; row < static_cast<int>(projects.size()); ++row) {
        if (matchesProject(projects[static_cast<std::size_t>(row)],
                           selector)) {
            return row;
        }
    }
    return -1;
}

int recentRowForMatch(const std::vector<Project>& projects,
                      const std::vector<int>& recentIndices,
                      const std::string& selector) {
    for (int row = 0; row < static_cast<int>(recentIndices.size()); ++row) {
        const int projectIndex = recentIndices[static_cast<std::size_t>(row)];
        if (projectIndex < 0 ||
            projectIndex >= static_cast<int>(projects.size())) {
            continue;
        }
        if (matchesProject(projects[static_cast<std::size_t>(projectIndex)],
                           selector)) {
            return row;
        }
    }
    return -1;
}

int actionFocusIndex(const std::vector<MenuAction>& actions,
                     const DefaultFocus& focus) {
    if (actions.empty()) {
        return 0;
    }
    const auto matcher = [&](const std::string& selector) {
        const std::string key = matchKey(selector);
        for (int row = 0; row < static_cast<int>(actions.size()); ++row) {
            const MenuAction& action = actions[static_cast<std::size_t>(row)];
            if (matchKey(action.id) == key ||
                matchKey(action.label) == key) {
                return row;
            }
        }
        return -1;
    };
    return resolveFocusRow(focus, static_cast<int>(actions.size()), matcher);
}

FocusChoice resolveDefaultFocus(const DefaultFocus& focus,
                                const std::vector<int>& focusOrder,
                                int projectEntries, int recentEntries,
                                const std::vector<Project>& projects,
                                const std::vector<int>& recentIndices) {
    const FocusSection section = focusSectionFromText(focus.section);
    // Actions and path fields live on other pages. They must not move the
    // main-screen focus or borrow their item index there; the page they
    // belong to applies the selector when it opens.
    const bool mainSection = section == FocusSection::Projects ||
                             section == FocusSection::Recent ||
                             section == FocusSection::QuickInput;
    const DefaultFocus effective = mainSection ? focus : DefaultFocus{};

    const int requested =
        section == FocusSection::Recent
            ? kMainFocusRecent
            : (section == FocusSection::QuickInput ? kMainFocusQuickInput
                                                   : kMainFocusProjects);

    const auto available = [&](int node) {
        if (focusIndexOf(focusOrder, node) < 0) {
            return false;
        }
        switch (node) {
            case kMainFocusProjects:
                return projectEntries > 0;
            case kMainFocusRecent:
                return recentEntries > 0;
            default:
                return true;
        }
    };

    int node = requested;
    bool usedFallback = false;
    if (!available(node)) {
        bool found = false;
        for (const int candidate : {kMainFocusProjects, kMainFocusRecent,
                                    kMainFocusQuickInput}) {
            if (available(candidate)) {
                node = candidate;
                found = true;
                usedFallback = true;
                break;
            }
        }
        if (!found) {
            return {};
        }
    }

    FocusChoice choice;
    choice.node = node;

    if (node == kMainFocusProjects) {
        // "add" targets the "+ Add project" row; it exists only when the
        // project list has one more entry than the project array.
        const auto projectMatcher = [&](const std::string& selector) {
            const std::string key = matchKey(selector);
            if (key == "add" || key == "addproject" || key == "+") {
                return projectEntries > static_cast<int>(projects.size())
                           ? projectEntries - 1
                           : -1;
            }
            return projectRowForMatch(projects, selector);
        };
        choice.item =
            usedFallback
                ? std::max(projectMatcher(effective.match), 0)
                : resolveFocusRow(effective, projectEntries, projectMatcher);
    } else if (node == kMainFocusRecent) {
        choice.item = usedFallback
                          ? std::max(recentRowForMatch(
                                         projects, recentIndices,
                                         effective.match),
                                     0)
                          : resolveFocusRow(effective, recentEntries,
                                            [&](const std::string& selector) {
                                                return recentRowForMatch(
                                                    projects, recentIndices,
                                                    selector);
                                            });
    }
    return choice;
}

FocusChoice resolveDefaultFocus(const DefaultFocus& focus,
                                const std::vector<int>& focusOrder,
                                int projectEntries, int recentEntries) {
    static const std::vector<Project> kNoProjects;
    static const std::vector<int> kNoIndices;
    return resolveDefaultFocus(focus, focusOrder, projectEntries,
                               recentEntries, kNoProjects, kNoIndices);
}

}  // namespace claudex
