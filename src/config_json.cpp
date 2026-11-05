#include "config_json.hpp"

#include "default_focus.hpp"
#include "text_util.hpp"

#include "default_config.hpp"
#include "default_theme.hpp"

#include <algorithm>
#include <string_view>

namespace claudex {

ScreenTheme screenThemeFromJson(
    const json& value, ScreenTheme screen,
    const std::map<std::string, std::string>* variables) {
    if (!value.is_object()) {
        return screen;
    }
    if (value.contains("border") && value["border"].is_string()) {
        screen.border = value["border"].get<std::string>();
    }
    if (value.contains("align") && value["align"].is_string()) {
        screen.align = value["align"].get<std::string>();
    }
    if (value.contains("width")) {
        if (const auto width = resolveThemeInt(value["width"], variables)) {
            screen.width = *width;
        }
    }
    if (value.contains("addTitle") && value["addTitle"].is_string()) {
        screen.addTitle = value["addTitle"].get<std::string>();
    }
    if (value.contains("editTitle") && value["editTitle"].is_string()) {
        screen.editTitle = value["editTitle"].get<std::string>();
    }
    if (value.contains("renameTitle") && value["renameTitle"].is_string()) {
        screen.renameTitle = value["renameTitle"].get<std::string>();
    }
    if (value.contains("blocks") && value["blocks"].is_array()) {
        screen.blocks = value["blocks"];
    }
    return screen;
}

std::string defaultActionLabel(const ProjectMenuTheme& labels,
                               const std::string& id) {
    if (id == "launch") {
        return labels.launchLabel;
    }
    if (id == "edit") {
        return labels.editLabel;
    }
    if (id == "rename") {
        return labels.renameLabel;
    }
    if (id == "remove") {
        return labels.removeLabel;
    }
    if (id == "back") {
        return labels.backLabel;
    }
    if (id == "moveup") {
        return labels.moveUpLabel;
    }
    if (id == "movedown") {
        return labels.moveDownLabel;
    }
    if (id == "movetop") {
        return labels.moveTopLabel;
    }
    if (id == "movebottom") {
        return labels.moveBottomLabel;
    }
    return {};
}

std::vector<MenuAction> actionsFromJson(const json& array,
                                        const ProjectMenuTheme& labels) {
    std::vector<MenuAction> actions;
    if (!array.is_array()) {
        return actions;
    }

    // The old "rename" action opened the name-only view of the edit screen.
    // Edit project now covers the same screen with both fields, so drop the
    // duplicate. A theme that only lists rename keeps a working edit action.
    bool hasEditAction = false;
    for (const json& item : array) {
        if (item.is_object() &&
            lowerAscii(trim(jsonStringField(item, "id", ""))) == "edit") {
            hasEditAction = true;
            break;
        }
    }

    for (const json& item : array) {
        if (!item.is_object()) {
            continue;
        }
        MenuAction action;
        action.id = lowerAscii(trim(jsonStringField(item, "id", "")));
        if (action.id == "rename") {
            if (hasEditAction) {
                continue;
            }
            action.id = "edit";
        }
        action.command = jsonStringField(item, "command", "");
        action.label = jsonStringField(item, "label", "");
        if (action.label.empty()) {
            action.label = defaultActionLabel(labels, action.id);
        }
        if (action.label.empty()) {
            action.label =
                action.command.empty() ? action.id : action.command;
        }
        if (!action.id.empty() || !action.command.empty()) {
            actions.push_back(std::move(action));
        }
    }
    return actions;
}

std::vector<std::string> keyListFromJson(
    const json& value, const std::vector<std::string>& fallback) {
    if (value.is_string()) {
        return {value.get<std::string>()};
    }
    if (value.is_array()) {
        std::vector<std::string> keys;
        for (const json& item : value) {
            if (item.is_string()) {
                keys.push_back(item.get<std::string>());
            }
        }
        if (keys.empty() && !value.empty()) {
            return fallback;
        }
        return keys;
    }
    return fallback;
}

KeyBindings keysFromJson(const json& value, KeyBindings keys) {
    if (!value.is_object()) {
        return keys;
    }
    if (value.contains("quit")) {
        keys.quit = keyListFromJson(value["quit"], keys.quit);
    }
    if (value.contains("projectMenu")) {
        keys.projectMenu =
            keyListFromJson(value["projectMenu"], keys.projectMenu);
    }
    if (value.contains("remove")) {
        keys.remove = keyListFromJson(value["remove"], keys.remove);
    }
    if (value.contains("add")) {
        keys.add = keyListFromJson(value["add"], keys.add);
    }
    if (value.contains("quickInput")) {
        keys.quickInput =
            keyListFromJson(value["quickInput"], keys.quickInput);
    }
    if (value.contains("confirm")) {
        keys.confirm = keyListFromJson(value["confirm"], keys.confirm);
    }
    if (value.contains("cancel")) {
        keys.cancel = keyListFromJson(value["cancel"], keys.cancel);
    }
    if (value.contains("back")) {
        keys.back = keyListFromJson(value["back"], keys.back);
    }
    return keys;
}

// The embedded default config is a build input and is expected to be valid.
// It is deliberately parsed without a fallback: there must be one source of
// truth for default values.
const json& embeddedDefaultConfig() {
    static const json document = json::parse(claudex_defaults::kConfigJson);
    return document;
}

const json& embeddedDefaultTheme() {
    static const json document = json::parse(claudex_defaults::kThemeJson);
    return document;
}

// A string applies to every platform; an object can provide a different
// command for Windows and POSIX. Blank values keep the caller's current value
// so missing/blank platform entries fall back to the defaults.
bool updateLaunchCommand(const json& value, LaunchCommand& command) {
    if (value.is_string()) {
        std::string text = trim(value.get<std::string>());
        if (text.empty()) {
            return false;
        }
        command.windows = text;
        command.posix = std::move(text);
        return true;
    }
    if (!value.is_object()) {
        return false;
    }
    bool any = false;
    for (const char* key : {"windows", "posix"}) {
        if (!value.contains(key) || !value.at(key).is_string()) {
            continue;
        }
        std::string text = trim(value.at(key).get<std::string>());
        if (text.empty()) {
            continue;
        }
        if (std::string_view(key) == "windows") {
            command.windows = std::move(text);
        } else {
            command.posix = std::move(text);
        }
        any = true;
    }
    return any;
}

Settings settingsFromDefault(const json& document) {
    Settings settings;
    settings.themeFile = document.at("theme").get<std::string>();
    settings.configFile = document.at("configFile").get<std::string>();
    // The embedded default uses an array, but a string is accepted too so the
    // default file stays easy to edit by hand.
    const json& claudePath = document.at("claudePath");
    settings.claudePath =
        claudePath.is_string()
            ? std::vector<std::string>{claudePath.get<std::string>()}
            : claudePath.get<std::vector<std::string>>();
    const json& claudeExclude = document.at("claudeExclude");
    settings.claudeExclude =
        claudeExclude.is_string()
            ? std::vector<std::string>{claudeExclude.get<std::string>()}
            : claudeExclude.get<std::vector<std::string>>();
    const json& launchCommand = document.at("launchCommand");
    if (launchCommand.is_string()) {
        settings.launchCommand.windows =
            launchCommand.get<std::string>();
        settings.launchCommand.posix = settings.launchCommand.windows;
    } else {
        settings.launchCommand.windows =
            launchCommand.at("windows").get<std::string>();
        settings.launchCommand.posix =
            launchCommand.at("posix").get<std::string>();
    }
    settings.claudeConfigDir =
        document.at("claudeConfigDir").get<std::string>();
    settings.tempDir = document.at("tempDir").get<std::string>();
    settings.logFile = document.at("logFile").get<std::string>();
    settings.tabOnlySectionSwitch =
        document.at("tabOnlySectionSwitch").get<bool>();
    settings.defaultProjectName =
        document.at("defaultProjectName").get<std::string>();
    settings.defaultFocusTargets =
        defaultFocusListFromJson(document.at("defaultFocus"));
    return settings;
}

std::vector<std::string> stringListField(
    const json& object, const char* key,
    const std::vector<std::string>& fallback) {
    if (!object.is_object()) {
        return fallback;
    }
    auto item = object.find(key);
    if (item == object.end() || !item->is_array()) {
        return fallback;
    }
    std::vector<std::string> values;
    for (const json& value : *item) {
        if (!value.is_string()) {
            return fallback;
        }
        values.push_back(value.get<std::string>());
    }
    return values;
}

namespace {

// Copies a named-value object into the variable table. Integers are stored as
// their decimal text so the same table serves colors and sizes.
void applyThemeVariables(std::map<std::string, std::string>& variables,
                         const json& document, const char* key) {
    if (!document.is_object() || !document.contains(key)) {
        return;
    }
    const json& block = document[key];
    if (!block.is_object()) {
        return;
    }
    for (auto item = block.begin(); item != block.end(); ++item) {
        if (item.value().is_string()) {
            variables[normalizeMenuToken(item.key())] =
                item.value().get<std::string>();
        } else if (item.value().is_number_integer()) {
            variables[normalizeMenuToken(item.key())] = item.value().dump();
        }
    }
}

// Reports reference cycles once, starting from the smallest name in the cycle
// so a two-name loop is not reported twice.
void collectVariableCycles(const std::map<std::string, std::string>& variables,
                           std::vector<std::string>& warnings) {
    for (const auto& entry : variables) {
        std::vector<std::string> path{entry.first};
        std::string current = entry.second;
        for (int depth = 0; depth < 8; ++depth) {
            const std::string token = normalizeMenuToken(current);
            auto next = variables.find(token);
            if (next == variables.end()) {
                break;
            }
            auto seen = std::find(path.begin(), path.end(), token);
            if (seen != path.end()) {
                std::vector<std::string> cycle(seen, path.end());
                const std::string first = *std::min_element(
                    cycle.begin(), cycle.end());
                if (entry.first != first) {
                    break;
                }
                std::string message = "variable cycle:";
                for (const std::string& name : cycle) {
                    message += " " + name + " ->";
                }
                message += " " + first;
                warnings.push_back(std::move(message));
                break;
            }
            path.push_back(token);
            current = next->second;
        }
    }
}

}  // namespace

MenuTheme menuThemeFromDocument(const json& document,
                                const MenuTheme* defaults) {
    MenuTheme theme;
    if (defaults != nullptr) {
        theme.variables = defaults->variables;
        theme.messages = defaults->messages;
        theme.screens = defaults->screens;
        theme.keys = defaults->keys;
    }
    // "colors" is the older name of the same table; "variables" wins on a
    // duplicate key.
    applyThemeVariables(theme.variables, document, "colors");
    applyThemeVariables(theme.variables, document, "variables");
    collectVariableCycles(theme.variables, theme.warnings);

    theme.addProjectLabel =
        document.at("addProjectLabel").get<std::string>();
    theme.projectEntryFormat =
        document.at("projectEntryFormat").get<std::string>();
    theme.projectNameDisplay = projectNameDisplayFromText(
        jsonStringField(document, "projectNameDisplay",
                        defaults ? projectNameDisplayText(
                                       defaults->projectNameDisplay)
                                 : "both"),
        defaults ? defaults->projectNameDisplay : ProjectNameDisplay::Both);
    theme.titleFormat = document.at("titleFormat").get<std::string>();
    theme.titleAlign = document.at("titleAlign").get<std::string>();
    theme.borderStyle = document.at("borderStyle").get<std::string>();
    theme.borderCharacters =
        document.at("borderCharacters").get<std::string>();
    theme.separatorStyle = document.at("separatorStyle").get<std::string>();
    theme.separatorCharacter =
        document.at("separatorCharacter").get<std::string>();
    theme.activePrefix = document.at("activePrefix").get<std::string>();
    theme.inactivePrefix = document.at("inactivePrefix").get<std::string>();
    theme.hintText = document.at("hintText").get<std::string>();
    theme.pathPlaceholder =
        document.at("pathPlaceholder").get<std::string>();
    theme.footerAlign = document.at("footerAlign").get<std::string>();
    theme.footerLines = stringListField(
        document, "footerLines",
        defaults ? defaults->footerLines : std::vector<std::string>{});
    theme.recentTitleFormat = jsonStringField(
        document, "recentTitleFormat",
        defaults ? defaults->recentTitleFormat : std::string());
    theme.recentEntryFormat = jsonStringField(
        document, "recentEntryFormat",
        defaults ? defaults->recentEntryFormat : std::string());
    theme.recentEmptyText = jsonStringField(
        document, "recentEmptyText",
        defaults ? defaults->recentEmptyText : std::string());
    theme.recentLimit = std::max(
        0, jsonIntFieldResolved(document, "recentLimit",
                                defaults ? defaults->recentLimit : 5,
                                &theme.variables));
    theme.projectNamePlaceholder = jsonStringField(
        document, "projectNamePlaceholder",
        defaults ? defaults->projectNamePlaceholder : std::string());
    theme.quickInputLabel = jsonStringField(
        document, "quickInputLabel",
        defaults ? defaults->quickInputLabel : std::string());
    theme.quickInputPlaceholder = jsonStringField(
        document, "quickInputPlaceholder",
        defaults ? defaults->quickInputPlaceholder : std::string());
    theme.tinyHintEnabled = jsonBoolField(
        document, "tinyHintEnabled",
        defaults ? defaults->tinyHintEnabled : theme.tinyHintEnabled);
    theme.tinyHintText = jsonStringField(
        document, "tinyHintText",
        defaults ? defaults->tinyHintText : theme.tinyHintText);
    theme.tinyHintMinWidth = std::max(
        0, jsonIntFieldResolved(
               document, "tinyHintMinWidth",
               defaults ? defaults->tinyHintMinWidth : theme.tinyHintMinWidth,
               &theme.variables));
    theme.tinyHintMinHeight = std::max(
        0, jsonIntFieldResolved(
               document, "tinyHintMinHeight",
               defaults ? defaults->tinyHintMinHeight
                        : theme.tinyHintMinHeight,
               &theme.variables));
    theme.pathContinueLabel = jsonStringField(
        document, "pathContinueLabel",
        defaults ? defaults->pathContinueLabel : std::string());
    theme.pathCancelLabel = jsonStringField(
        document, "pathCancelLabel",
        defaults ? defaults->pathCancelLabel : std::string());
    theme.confirmCreateLabel = jsonStringField(
        document, "confirmCreateLabel",
        defaults ? defaults->confirmCreateLabel : std::string());
    theme.confirmCreateBackLabel = jsonStringField(
        document, "confirmCreateBackLabel",
        defaults ? defaults->confirmCreateBackLabel : std::string());
    theme.confirmRemoveLabel = jsonStringField(
        document, "confirmRemoveLabel",
        defaults ? defaults->confirmRemoveLabel : std::string());
    theme.confirmRemoveBackLabel = jsonStringField(
        document, "confirmRemoveBackLabel",
        defaults ? defaults->confirmRemoveBackLabel : std::string());

    const json& messages = document.at("messages");
    for (auto item = messages.begin(); item != messages.end(); ++item) {
        if (item.value().is_string()) {
            theme.messages[item.key()] =
                item.value().get<std::string>();
        }
    }

    const json& screens = document.at("screens");
    for (auto item = screens.begin(); item != screens.end(); ++item) {
        ScreenTheme fallback;
        auto existing = theme.screens.find(item.key());
        if (existing != theme.screens.end()) {
            fallback = existing->second;
        }
        theme.screens[item.key()] = screenThemeFromJson(
            item.value(), std::move(fallback), &theme.variables);
    }
    theme.keys = keysFromJson(document.at("keys"), theme.keys);

    const json& projectMenu = document.at("projectMenu");
    theme.projectMenu.titleFormat =
        projectMenu.at("titleFormat").get<std::string>();
    theme.projectMenu.projectFormat =
        projectMenu.at("projectFormat").get<std::string>();
    theme.projectMenu.launchLabel =
        projectMenu.at("launchLabel").get<std::string>();
    theme.projectMenu.editLabel =
        projectMenu.at("editLabel").get<std::string>();
    theme.projectMenu.renameLabel =
        projectMenu.at("renameLabel").get<std::string>();
    theme.projectMenu.removeLabel =
        projectMenu.at("removeLabel").get<std::string>();
    theme.projectMenu.backLabel =
        projectMenu.at("backLabel").get<std::string>();
    theme.projectMenu.moveUpLabel =
        projectMenu.at("moveUpLabel").get<std::string>();
    theme.projectMenu.moveDownLabel =
        projectMenu.at("moveDownLabel").get<std::string>();
    theme.projectMenu.moveTopLabel =
        projectMenu.at("moveTopLabel").get<std::string>();
    theme.projectMenu.moveBottomLabel =
        projectMenu.at("moveBottomLabel").get<std::string>();
    theme.projectMenu.footerLines = stringListField(
        projectMenu, "footerLines",
        defaults ? defaults->projectMenu.footerLines
                 : std::vector<std::string>{});

    const json& actions = projectMenu.at("actions");
    theme.actions = actionsFromJson(actions, theme.projectMenu);
    if (theme.actions.empty() && defaults != nullptr &&
        !actions.empty()) {
        theme.actions = defaults->actions;
    }
    return theme;
}

void collectConfigIssues(const json& defaults, const json& user,
                         const std::string& prefix,
                         std::vector<std::string>& issues) {
    if (!defaults.is_object() || !user.is_object()) {
        return;
    }

    for (auto item = defaults.begin(); item != defaults.end(); ++item) {
        const std::string path =
            prefix.empty() ? item.key() : prefix + "." + item.key();
        if (!user.contains(item.key())) {
            issues.push_back(path + ": missing");
            continue;
        }

        const json& defaultValue = item.value();
        const json& userValue = user.at(item.key());
        if (prefix.empty() && item.key() == "launchCommand") {
            if (userValue.is_string()) {
                continue;
            }
            if (!userValue.is_object()) {
                issues.push_back(
                    path +
                    ": wrong type (expected string or platform object)");
                continue;
            }
            collectConfigIssues(defaultValue, userValue, path, issues);
            continue;
        }
        // claudePath and claudeExclude accept either a single string or an
        // array of strings.
        if (prefix.empty() && (item.key() == "claudePath" ||
                               item.key() == "claudeExclude")) {
            bool valid = userValue.is_string();
            if (userValue.is_array()) {
                valid = true;
                for (const json& entry : userValue) {
                    if (!entry.is_string()) {
                        valid = false;
                        break;
                    }
                }
            }
            if (!valid) {
                issues.push_back(
                    path + ": wrong type (expected string or string array)");
            }
            continue;
        }
        // defaultFocus accepts one target object or an array of targets, so
        // the generic type check would reject a valid array.
        if (prefix.empty() && item.key() == "defaultFocus") {
            const auto checkTarget = [&](const json& target,
                                         const std::string& targetPath) {
                if (!target.is_object()) {
                    issues.push_back(
                        targetPath + ": wrong type (expected object)");
                    return;
                }
                for (const char* field : {"section", "match"}) {
                    if (target.contains(field) &&
                        !target.at(field).is_string()) {
                        issues.push_back(targetPath + "." + field +
                                         ": wrong type (expected string)");
                    }
                }
                if (target.contains("item") &&
                    !target.at("item").is_number_integer()) {
                    issues.push_back(targetPath +
                                     ".item: wrong type (expected integer)");
                }
            };
            if (userValue.is_array()) {
                for (std::size_t index = 0; index < userValue.size();
                     ++index) {
                    checkTarget(userValue[index],
                                path + "[" + std::to_string(index) + "]");
                }
            } else {
                checkTarget(userValue, path);
            }
            continue;
        }
        if (defaultValue.is_object()) {
            if (!userValue.is_object()) {
                issues.push_back(path + ": wrong type (expected object)");
            } else {
                collectConfigIssues(defaultValue, userValue, path, issues);
            }
        } else if (defaultValue.is_array()) {
            if (!userValue.is_array()) {
                issues.push_back(path + ": wrong type (expected array)");
            }
        } else if (userValue.type() != defaultValue.type()) {
            issues.push_back(path + ": wrong type");
        }
    }
}

json mergeConfigDefaults(const json& defaults, const json& user) {
    if (!defaults.is_object() || !user.is_object()) {
        return user.type() == defaults.type() ? user : defaults;
    }

    json merged = user;
    for (auto item = defaults.begin(); item != defaults.end(); ++item) {
        const std::string key = item.key();
        const json& defaultValue = item.value();
        if (!merged.contains(key)) {
            merged[key] = defaultValue;
            continue;
        }

        json& userValue = merged[key];
        if (key == "launchCommand") {
            if (userValue.is_string()) {
                continue;
            }
            if (userValue.is_object()) {
                userValue = mergeConfigDefaults(defaultValue, userValue);
                continue;
            }
            userValue = defaultValue;
            continue;
        }
        // claudePath and claudeExclude accept either a single string or an
        // array of strings.
        if (key == "claudePath" || key == "claudeExclude") {
            bool valid = userValue.is_string();
            if (userValue.is_array()) {
                valid = true;
                for (const json& entry : userValue) {
                    if (!entry.is_string()) {
                        valid = false;
                        break;
                    }
                }
            }
            if (valid) {
                continue;
            }
            userValue = defaultValue;
            continue;
        }
        // defaultFocus accepts one object or an array of objects. Fill the
        // missing fields of each target from the shipped default.
        if (key == "defaultFocus") {
            const json templateTarget =
                defaultValue.is_array()
                    ? (defaultValue.empty() ? json::object()
                                            : defaultValue.front())
                    : defaultValue;
            const json fallbackTarget =
                templateTarget.is_object() ? templateTarget : json::object();
            if (userValue.is_array()) {
                json mergedArray = json::array();
                for (const json& target : userValue) {
                    if (target.is_object()) {
                        mergedArray.push_back(
                            mergeConfigDefaults(fallbackTarget, target));
                    }
                }
                userValue = std::move(mergedArray);
            } else if (userValue.is_object()) {
                userValue =
                    mergeConfigDefaults(fallbackTarget, userValue);
            } else {
                userValue = defaultValue;
            }
            continue;
        }
        if (defaultValue.is_object()) {
            if (userValue.is_object()) {
                userValue = mergeConfigDefaults(defaultValue, userValue);
            } else {
                userValue = defaultValue;
            }
        } else if (defaultValue.is_array()) {
            if (!userValue.is_array()) {
                userValue = defaultValue;
            }
        } else if (userValue.type() != defaultValue.type()) {
            userValue = defaultValue;
        }
    }
    return merged;
}

json mergeThemeDefaults(const json& defaults, const json& user) {
    json merged = mergeConfigDefaults(defaults, user);
    if (!merged.is_object()) {
        return merged;
    }
    // Color and size definitions belong to the theme author. Repair must not
    // add the default palette to a theme that does not mention it.
    for (const char* key : {"variables", "colors"}) {
        if (user.is_object() && user.contains(key)) {
            merged[key] = user.at(key);
        } else {
            merged.erase(key);
        }
    }
    return merged;
}

namespace {

bool isBlankConfigString(const json& value) {
    return value.is_string() &&
           trim(value.get<std::string>()).empty();
}

// claudePath is the only default value that may be written as a string or an
// array, so emptiness has to consider both shapes.
bool isEmptyClaudePathValue(const json& value) {
    if (value.is_string()) {
        return trim(value.get<std::string>()).empty();
    }
    if (value.is_array()) {
        for (const json& item : value) {
            if (item.is_string() &&
                !trim(item.get<std::string>()).empty()) {
                return false;
            }
        }
        return true;
    }
    return true;
}

bool isEmptyLaunchCommandValue(const json& value) {
    if (value.is_string()) {
        return trim(value.get<std::string>()).empty();
    }
    if (value.is_object()) {
        for (const char* key : {"windows", "posix"}) {
            if (value.contains(key) && value.at(key).is_string() &&
                !trim(value.at(key).get<std::string>()).empty()) {
                return false;
            }
        }
        return true;
    }
    return true;
}

}  // namespace

// These keys treat "empty" as "use the resource default". logFile is
// deliberately excluded: an empty logFile disables logging.
bool isEmptyDefaultConfigValue(const json& document, const char* key) {
    if (!document.contains(key)) {
        return true;
    }
    const json& value = document.at(key);
    if (std::string_view(key) == "claudePath") {
        return isEmptyClaudePathValue(value);
    }
    if (std::string_view(key) == "launchCommand") {
        return isEmptyLaunchCommandValue(value);
    }
    return isBlankConfigString(value);
}

bool hasEmptyDefaultConfigValues(const json& document) {
    for (const char* key : {"theme", "claudePath", "launchCommand",
                            "claudeConfigDir", "tempDir"}) {
        if (isEmptyDefaultConfigValue(document, key)) {
            return true;
        }
    }
    return false;
}

void materializeLaunchCommand(json& document, const json& defaults) {
    if (!document.contains("launchCommand")) {
        document["launchCommand"] = defaults.at("launchCommand");
        return;
    }
    json& value = document["launchCommand"];
    if (value.is_string()) {
        if (trim(value.get<std::string>()).empty()) {
            value = defaults.at("launchCommand");
        }
        return;
    }
    if (!value.is_object()) {
        value = defaults.at("launchCommand");
        return;
    }
    const json& defaultCommand = defaults.at("launchCommand");
    for (const char* key : {"windows", "posix"}) {
        if (!value.contains(key) || !value.at(key).is_string() ||
            trim(value.at(key).get<std::string>()).empty()) {
            value[key] = defaultCommand.at(key);
        }
    }
}

void materializeConfigDefaults(json& document, const json& defaults) {
    for (const char* key : {"theme", "claudePath", "launchCommand",
                            "claudeConfigDir", "tempDir"}) {
        if (std::string_view(key) == "launchCommand") {
            materializeLaunchCommand(document, defaults);
            continue;
        }
        if (isEmptyDefaultConfigValue(document, key)) {
            document[key] = defaults.at(key);
        }
    }
}
}  // namespace claudex
