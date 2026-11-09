#pragma once

#include "config_types.hpp"
#include "focus_order.hpp"

#include <functional>
#include <optional>
#include <vector>

namespace claudex {

// Focusable sections the layout and the action menu can expose. Unknown
// values keep the built-in default (the project list).
enum class FocusSection {
    Projects,
    Recent,
    QuickInput,
    Cwd,
    Actions,
    PathName,
    PathPath,
};

FocusSection focusSectionFromText(const std::string& spec);

// Parses one config target:
//   { "section": "recent", "match": "F:/work/app", "item": 1 }
// |match| is a stable identifier defined by the section (path or name for
// the project lists, id or label for the action menu). It survives reordering.
// When it is absent or does not match, |item| is a zero-based row and a
// negative value means the last row. Returns nullopt when the value is not an
// object, so the caller keeps the built-in default.
std::optional<DefaultFocus> defaultFocusFromJson(const json& value);

json defaultFocusToJson(const DefaultFocus& focus);

// The config key accepts one object or an array of objects, so several
// sections can each have a default. Invalid array entries are skipped.
std::vector<DefaultFocus> defaultFocusListFromJson(const json& value);

// Writes a single target as an object and several as an array, keeping the
// common one-target config unchanged.
json defaultFocusListToJson(const std::vector<DefaultFocus>& targets);

// First target that belongs to a page. Returns nullptr when that page has no
// configured target.
const DefaultFocus* mainFocusTarget(const std::vector<DefaultFocus>& targets);
const DefaultFocus* actionFocusTarget(
    const std::vector<DefaultFocus>& targets);
const DefaultFocus* pathFocusTarget(const std::vector<DefaultFocus>& targets);

// Resolves a row selector against |count| rows. A non-empty |match| is looked
// up first; when it is absent or |matcher| returns -1, the positional |item|
// is used. Negative items mean the last row and out-of-range items clamp.
int resolveFocusRow(const DefaultFocus& focus, int count,
                    const std::function<int(const std::string&)>& matcher);

// Row in the pinned project list whose stored path or display name matches.
int projectRowForMatch(const std::vector<Project>& projects,
                       const std::string& selector);

// Row in the recent list whose project path or display name matches.
int recentRowForMatch(const std::vector<Project>& projects,
                      const std::vector<int>& recentIndices,
                      const std::string& selector);

// Row in the project action list whose id or label matches.
int actionFocusIndex(const std::vector<MenuAction>& actions,
                     const DefaultFocus& focus);

struct FocusChoice {
    // kMainFocusProjects / kMainFocusRecent / kMainFocusQuickInput /
    // kMainFocusCwd, or -1 when the main screen has no focusable section.
    int node = -1;
    // Zero-based row chosen inside the section's list. Always 0 for the
    // quick-input and cwd sections.
    int item = 0;
};

// Picks the startup focus on the main screen. |projectEntries| counts the
// project menu rows (including the "+ Add project" row); |recentEntries|
// counts the recent rows. A section that lives on another page (actions,
// path fields) is ignored here; that page applies the selector when it opens.
//
// |targets| is the configured priority list: the first target whose section
// is drawn by the theme and currently available wins. Availability skips a
// hidden cwd section, an empty project list, and an empty recent list. When
// every configured target is unavailable, the first available section in
// |focusOrder| (the theme's authored block order) is used. An identity match
// from the first configured main target is attempted in that fallback
// section, so a project that moved from recent to the pinned list is still
// found.
FocusChoice resolveDefaultFocus(const std::vector<DefaultFocus>& targets,
                                const std::vector<int>& focusOrder,
                                int projectEntries, int recentEntries,
                                bool cwdAvailable,
                                const std::vector<Project>& projects,
                                const std::vector<int>& recentIndices);

// Compatibility overload for one configured target. It keeps the historical
// single-target behavior and treats the cwd section as unavailable.
FocusChoice resolveDefaultFocus(const DefaultFocus& focus,
                                const std::vector<int>& focusOrder,
                                int projectEntries, int recentEntries,
                                const std::vector<Project>& projects,
                                const std::vector<int>& recentIndices);

// Same, without identity matching.
FocusChoice resolveDefaultFocus(const DefaultFocus& focus,
                                const std::vector<int>& focusOrder,
                                int projectEntries, int recentEntries);

}  // namespace claudex
