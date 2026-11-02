#pragma once

#include "config_types.hpp"

#include <string>
#include <vector>

namespace claudex {

ScreenTheme screenThemeFromJson(
    const json& value, ScreenTheme screen,
    const std::map<std::string, std::string>* variables = nullptr);

std::string defaultActionLabel(const ProjectMenuTheme& labels,
                               const std::string& id);
std::vector<MenuAction> actionsFromJson(const json& array,
                                        const ProjectMenuTheme& labels);

std::vector<std::string> keyListFromJson(
    const json& value, const std::vector<std::string>& fallback);
KeyBindings keysFromJson(const json& value, KeyBindings keys);

// The embedded defaults are build inputs generated from resources/*.json.
const json& embeddedDefaultConfig();
const json& embeddedDefaultTheme();

// A string applies to every platform; an object can provide a different
// command for Windows and POSIX. Blank values keep the current value.
bool updateLaunchCommand(const json& value, LaunchCommand& command);
Settings settingsFromDefault(const json& document);

std::vector<std::string> stringListField(
    const json& object, const char* key,
    const std::vector<std::string>& fallback);

// `defaults` supplies values for optional keys missing from `document`.
MenuTheme menuThemeFromDocument(const json& document,
                                const MenuTheme* defaults = nullptr);

// Appends "<path>: missing" / "<path>: wrong type ..." for every key where
// `user` deviates from the shape of `defaults`.
void collectConfigIssues(const json& defaults, const json& user,
                         const std::string& prefix,
                         std::vector<std::string>& issues);
// Fills missing keys and replaces wrongly typed values using `defaults`.
json mergeConfigDefaults(const json& defaults, const json& user);

// Theme defaults, except that the variable definitions are left exactly as the
// user wrote them: they are never filled in or rewritten, so the written theme
// file only holds the user's own entries. Lookup still falls back to the
// default theme at runtime.
json mergeThemeDefaults(const json& defaults, const json& user);

bool isEmptyDefaultConfigValue(const json& document, const char* key);
bool hasEmptyDefaultConfigValues(const json& document);
void materializeLaunchCommand(json& document, const json& defaults);
void materializeConfigDefaults(json& document, const json& defaults);

}  // namespace claudex
