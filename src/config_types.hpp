#pragma once

#include <nlohmann/json.hpp>

#include <initializer_list>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace claudex {

using json = nlohmann::json;

struct LaunchCommand {
    std::string windows;
    std::string posix;

    const std::string& current() const {
#ifdef _WIN32
        return windows;
#else
        return posix;
#endif
    }
};

struct Settings {
    std::string themeFile;
    // Optional path of another config file to load instead of this one. An
    // empty value keeps the launcher on the file it was started with.
    std::string configFile;
    std::vector<std::string> claudePath;
    std::vector<std::string> claudeExclude;
    LaunchCommand launchCommand;
    std::string claudeConfigDir;
    std::string tempDir;
    std::string logFile;
    bool tabOnlySectionSwitch = false;
};

struct Project {
    std::string path;
    std::string lastUsed;
    // Optional display name. It is markup, so it can carry <color> tags that
    // resolve through the theme's variables.
    std::string name;
};

struct ProjectMenuTheme {
    std::string titleFormat;
    std::string projectFormat;
    std::string launchLabel;
    std::string editLabel;
    std::string renameLabel;
    std::string removeLabel;
    std::string backLabel;
    std::string moveUpLabel;
    std::string moveDownLabel;
    std::string moveTopLabel;
    std::string moveBottomLabel;
    std::vector<std::string> footerLines;
};

struct MenuAction {
    std::string id;
    std::string label;
    std::string command;
};

// One configurable page. A page is a stack of blocks; see the block renderer
// for the available block types. Storing the authored block list as JSON
// keeps the layout language open-ended without a second schema to maintain.
struct ScreenTheme {
    std::string border;
    std::string align;
    int width = 0;
    std::string addTitle;
    std::string editTitle;
    std::string renameTitle;
    json blocks = json::array();
};

// Where a project's optional display name is rendered. Recent rows reuse the
// same project data as the pinned list, so the theme can keep names in one
// place or hide them entirely.
enum class ProjectNameDisplay { Both, Pinned, Recent, None };

struct KeyBindings {
    std::vector<std::string> quit;
    std::vector<std::string> projectMenu;
    std::vector<std::string> remove;
    std::vector<std::string> add;
    std::vector<std::string> quickInput;
    std::vector<std::string> confirm;
    std::vector<std::string> cancel;
    std::vector<std::string> back;
};

struct MenuTheme {
    // Named values shared by colors and sizes. Integer sizes are stored as
    // their decimal text so one table resolves both kinds.
    std::map<std::string, std::string> variables;
    std::vector<std::string> warnings;
    std::string addProjectLabel;
    std::string projectEntryFormat;
    ProjectNameDisplay projectNameDisplay = ProjectNameDisplay::Both;
    std::string titleFormat;
    std::string titleAlign;
    std::string borderStyle;
    std::string borderCharacters;
    std::string separatorStyle;
    std::string separatorCharacter;
    std::string activePrefix;
    std::string inactivePrefix;
    std::string hintText;
    std::string pathPlaceholder;
    std::string footerAlign;
    std::vector<std::string> footerLines;
    std::string recentTitleFormat;
    std::string recentEntryFormat;
    std::string recentEmptyText;
    int recentLimit = 5;
    std::string projectNamePlaceholder;
    std::string quickInputLabel;
    std::string quickInputPlaceholder;
    std::string pathContinueLabel;
    std::string pathCancelLabel;
    std::string confirmCreateLabel;
    std::string confirmCreateBackLabel;
    std::string confirmRemoveLabel;
    std::string confirmRemoveBackLabel;
    std::map<std::string, std::string> messages;
    std::map<std::string, ScreenTheme> screens;
    std::vector<MenuAction> actions;
    KeyBindings keys;
    ProjectMenuTheme projectMenu;
};

// Looks up a status message by key; throws std::out_of_range if it is missing.
std::string messageText(const MenuTheme& theme, const char* key);
std::string messageText(
    const MenuTheme& theme, const char* key,
    std::initializer_list<std::pair<std::string, std::string>> values);

// Accepts "both"/"all", "pinned"/"projects", "recent", and
// "none"/"hidden"/"off" (case- and separator-insensitive). Unknown values
// return |fallback| so an older theme keeps the built-in behavior.
ProjectNameDisplay projectNameDisplayFromText(const std::string& text,
                                              ProjectNameDisplay fallback);
std::string projectNameDisplayText(ProjectNameDisplay display);

std::string jsonStringField(const json& object, const char* key,
                            const std::string& fallback);
bool jsonBoolField(const json& object, const char* key, bool fallback);
int jsonIntField(const json& object, const char* key, int fallback);

// Resolves a size value: an integer, a numeric string, or the name of a
// numeric variable. Unknown names and reference cycles resolve to nullopt.
std::optional<int> resolveThemeInt(
    const json& value, const std::map<std::string, std::string>* variables,
    int depth = 0);
int jsonIntFieldResolved(const json& object, const char* key, int fallback,
                         const std::map<std::string, std::string>* variables);

}  // namespace claudex
