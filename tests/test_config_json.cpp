#include "test_framework.hpp"

#include "config_json.hpp"
#include "text_util.hpp"

#include <stdexcept>

using namespace claudex;

// --- config_types helpers --------------------------------------------------

TEST_CASE("config/jsonStringField falls back for missing or wrong types") {
    const json object = {{"name", "value"}, {"number", 3}};
    CHECK_EQ(jsonStringField(object, "name", "fallback"), "value");
    CHECK_EQ(jsonStringField(object, "number", "fallback"), "fallback");
    CHECK_EQ(jsonStringField(object, "missing", "fallback"), "fallback");
    CHECK_EQ(jsonStringField(json::array(), "name", "fallback"), "fallback");
    CHECK_EQ(jsonStringField(json(), "name", "fallback"), "fallback");
}

TEST_CASE("config/jsonBoolField and jsonIntField validate types") {
    const json object = {{"flag", true}, {"count", 7}, {"text", "x"},
                         {"real", 1.5}};
    CHECK(jsonBoolField(object, "flag", false));
    CHECK_FALSE(jsonBoolField(object, "count", false));
    CHECK(jsonBoolField(object, "missing", true));

    CHECK_EQ(jsonIntField(object, "count", -1), 7);
    CHECK_EQ(jsonIntField(object, "text", -1), -1);
    CHECK_EQ(jsonIntField(object, "real", -1), -1);
    CHECK_EQ(jsonIntField(object, "missing", 42), 42);
    CHECK_EQ(jsonIntField(json::array(), "count", 5), 5);
}

TEST_CASE("config/messageText substitutes placeholders") {
    MenuTheme theme;
    theme.messages["greeting"] = "Hello {name}, welcome to {place}. {name}!";
    CHECK_EQ(messageText(theme, "greeting"),
             "Hello {name}, welcome to {place}. {name}!");
    CHECK_EQ(messageText(theme, "greeting",
                         {{"name", "Ada"}, {"place", "ClaudeX"}}),
             "Hello Ada, welcome to ClaudeX. Ada!");
}

TEST_CASE("config/messageText throws for an unknown key") {
    MenuTheme theme;
    bool threw = false;
    try {
        (void)messageText(theme, "missing");
    } catch (const std::out_of_range&) {
        threw = true;
    }
    CHECK(threw);
}

// --- keys and actions ------------------------------------------------------

TEST_CASE("config/keyListFromJson accepts strings and string arrays") {
    const std::vector<std::string> fallback = {"q"};
    CHECK_EQ(keyListFromJson(json("x"), fallback),
             std::vector<std::string>{"x"});
    CHECK_EQ(keyListFromJson(json::array({"a", "b"}), fallback),
             (std::vector<std::string>{"a", "b"}));
    CHECK_EQ(keyListFromJson(json::array({"a", 3, "b"}), fallback),
             (std::vector<std::string>{"a", "b"}));
}

TEST_CASE("config/keyListFromJson fallback rules") {
    const std::vector<std::string> fallback = {"q"};
    // Only invalid entries: keep the previous binding.
    CHECK_EQ(keyListFromJson(json::array({1, 2}), fallback), fallback);
    // An explicitly empty list unbinds the action.
    CHECK(keyListFromJson(json::array(), fallback).empty());
    CHECK_EQ(keyListFromJson(json(5), fallback), fallback);
    CHECK_EQ(keyListFromJson(json(), fallback), fallback);
}

TEST_CASE("config/keysFromJson overrides only listed actions") {
    KeyBindings keys;
    keys.quit = {"q", "esc"};
    keys.add = {"a"};

    const json document = {{"quit", "x"}, {"cancel", json::array({"n"})}};
    const KeyBindings result = keysFromJson(document, keys);
    CHECK_EQ(result.quit, std::vector<std::string>{"x"});
    CHECK_EQ(result.add, std::vector<std::string>{"a"});
    CHECK_EQ(result.cancel, std::vector<std::string>{"n"});

    const KeyBindings unchanged = keysFromJson(json::array(), keys);
    CHECK_EQ(unchanged.quit, keys.quit);
}

TEST_CASE("config/defaultActionLabel maps built-in ids") {
    ProjectMenuTheme labels;
    labels.launchLabel = "Launch";
    labels.editLabel = "Edit";
    labels.renameLabel = "Rename";
    labels.removeLabel = "Remove";
    labels.backLabel = "Back";
    labels.moveUpLabel = "Up";
    labels.moveDownLabel = "Down";
    labels.moveTopLabel = "Top";
    labels.moveBottomLabel = "Bottom";
    CHECK_EQ(defaultActionLabel(labels, "launch"), "Launch");
    CHECK_EQ(defaultActionLabel(labels, "edit"), "Edit");
    CHECK_EQ(defaultActionLabel(labels, "rename"), "Rename");
    CHECK_EQ(defaultActionLabel(labels, "remove"), "Remove");
    CHECK_EQ(defaultActionLabel(labels, "back"), "Back");
    CHECK_EQ(defaultActionLabel(labels, "moveup"), "Up");
    CHECK_EQ(defaultActionLabel(labels, "movedown"), "Down");
    CHECK_EQ(defaultActionLabel(labels, "movetop"), "Top");
    CHECK_EQ(defaultActionLabel(labels, "movebottom"), "Bottom");
    CHECK_EQ(defaultActionLabel(labels, "custom"), "");
}

TEST_CASE("config/actionsFromJson label fallbacks and filtering") {
    ProjectMenuTheme labels;
    labels.launchLabel = "Launch Claude";

    const json array = json::array({
        {{"id", " Launch "}},                              // label from labels
        {{"id", "shell"}, {"command", "cmd"}},             // label = command? no
        {{"command", "echo hi"}},                          // label = command
        {{"id", "named"}, {"label", "Pretty"}},            // explicit label
        {{"id", ""}, {"command", ""}},                     // dropped
        "not-an-object",                                   // dropped
    });
    const std::vector<MenuAction> actions = actionsFromJson(array, labels);
    REQUIRE(actions.size() == 4);

    CHECK_EQ(actions[0].id, "launch");
    CHECK_EQ(actions[0].label, "Launch Claude");

    CHECK_EQ(actions[1].id, "shell");
    CHECK_EQ(actions[1].label, "cmd");
    CHECK_EQ(actions[1].command, "cmd");

    CHECK_EQ(actions[2].id, "");
    CHECK_EQ(actions[2].label, "echo hi");

    CHECK_EQ(actions[3].label, "Pretty");

    CHECK(actionsFromJson(json::object(), labels).empty());
}

// --- launch command and settings -------------------------------------------

TEST_CASE("config/updateLaunchCommand from a string") {
    LaunchCommand command;
    command.windows = "old-win";
    command.posix = "old-posix";
    CHECK(updateLaunchCommand(json("  {claude} --x  "), command));
    CHECK_EQ(command.windows, "{claude} --x");
    CHECK_EQ(command.posix, "{claude} --x");
}

TEST_CASE("config/updateLaunchCommand rejects blank and non-command values") {
    LaunchCommand command;
    command.windows = "w";
    command.posix = "p";
    CHECK_FALSE(updateLaunchCommand(json("   "), command));
    CHECK_FALSE(updateLaunchCommand(json(5), command));
    CHECK_FALSE(updateLaunchCommand(json::array({"x"}), command));
    CHECK_EQ(command.windows, "w");
    CHECK_EQ(command.posix, "p");
}

TEST_CASE("config/updateLaunchCommand from a platform object") {
    LaunchCommand command;
    command.windows = "w";
    command.posix = "p";

    CHECK(updateLaunchCommand(json{{"posix", " new-posix "}}, command));
    CHECK_EQ(command.windows, "w");
    CHECK_EQ(command.posix, "new-posix");

    // Blank and non-string entries keep the previous value.
    CHECK_FALSE(
        updateLaunchCommand(json{{"windows", "  "}, {"posix", 3}}, command));
    CHECK_EQ(command.windows, "w");
    CHECK_EQ(command.posix, "new-posix");

    CHECK(updateLaunchCommand(json{{"windows", "nw"}, {"posix", "np"}}, command));
    CHECK_EQ(command.windows, "nw");
    CHECK_EQ(command.posix, "np");
}

TEST_CASE("config/LaunchCommand::current picks the host platform") {
    LaunchCommand command;
    command.windows = "win";
    command.posix = "posix";
#ifdef _WIN32
    CHECK_EQ(command.current(), "win");
#else
    CHECK_EQ(command.current(), "posix");
#endif
}

TEST_CASE("config/settingsFromDefault reads the embedded defaults") {
    const json& document = embeddedDefaultConfig();
    REQUIRE(document.is_object());
    const Settings settings = settingsFromDefault(document);
    CHECK_EQ(settings.themeFile, document.at("theme").get<std::string>());
    CHECK_FALSE(settings.claudePath.empty());
    CHECK_FALSE(settings.launchCommand.windows.empty());
    CHECK_FALSE(settings.launchCommand.posix.empty());
    CHECK_FALSE(settings.claudeConfigDir.empty());
    CHECK_FALSE(settings.tempDir.empty());
}

TEST_CASE("config/settingsFromDefault accepts string forms") {
    json document = embeddedDefaultConfig();
    document["claudePath"] = "bin/claude";
    document["claudeExclude"] = "ClaudeX*";
    document["launchCommand"] = "{claude}";
    const Settings settings = settingsFromDefault(document);
    CHECK_EQ(settings.claudePath, std::vector<std::string>{"bin/claude"});
    CHECK_EQ(settings.claudeExclude, std::vector<std::string>{"ClaudeX*"});
    CHECK_EQ(settings.launchCommand.windows, "{claude}");
    CHECK_EQ(settings.launchCommand.posix, "{claude}");
}

TEST_CASE("config/stringListField only accepts all-string arrays") {
    const std::vector<std::string> fallback = {"fb"};
    const json object = {{"good", json::array({"a", "b"})},
                         {"mixed", json::array({"a", 1})},
                         {"text", "a"}};
    CHECK_EQ(stringListField(object, "good", fallback),
             (std::vector<std::string>{"a", "b"}));
    CHECK_EQ(stringListField(object, "mixed", fallback), fallback);
    CHECK_EQ(stringListField(object, "text", fallback), fallback);
    CHECK_EQ(stringListField(object, "missing", fallback), fallback);
    CHECK_EQ(stringListField(json::array(), "good", fallback), fallback);
}

// --- embedded defaults -----------------------------------------------------

TEST_CASE("config/embedded defaults are valid objects") {
    CHECK(embeddedDefaultConfig().is_object());
    CHECK(embeddedDefaultTheme().is_object());
    CHECK(embeddedDefaultConfig().contains("projects"));
    CHECK(embeddedDefaultTheme().contains("screens"));
}

TEST_CASE("config/menuThemeFromDocument parses the embedded default theme") {
    const MenuTheme theme = menuThemeFromDocument(embeddedDefaultTheme());
    CHECK_FALSE(theme.addProjectLabel.empty());
    CHECK_FALSE(theme.messages.empty());
    CHECK(theme.screens.count("main") == 1);
    CHECK_FALSE(theme.actions.empty());
    CHECK_FALSE(theme.keys.quit.empty());
    CHECK(theme.recentLimit >= 0);
    // Every message the code asks for by key must be defined.
    for (const char* key : {"configParseFailed", "themeCreated",
                            "claudeProjectMissing"}) {
        CHECK(theme.messages.count(key) == 1);
    }
}

TEST_CASE("config/menuThemeFromDocument keeps defaults for missing optional keys") {
    const MenuTheme defaults = menuThemeFromDocument(embeddedDefaultTheme());

    json document = embeddedDefaultTheme();
    document.erase("recentTitleFormat");
    document.erase("recentLimit");
    document["colors"] = json{{"Brand", "#ff0000"}};

    const MenuTheme theme = menuThemeFromDocument(document, &defaults);
    CHECK_EQ(theme.recentTitleFormat, defaults.recentTitleFormat);
    CHECK_EQ(theme.recentLimit, defaults.recentLimit);
    CHECK_EQ(theme.variables.at("brand"), "#ff0000");
}

TEST_CASE("config/projectNameDisplay parses aliases and falls back") {
    CHECK(projectNameDisplayFromText("both", ProjectNameDisplay::None) ==
          ProjectNameDisplay::Both);
    CHECK(projectNameDisplayFromText("All", ProjectNameDisplay::None) ==
          ProjectNameDisplay::Both);
    CHECK(projectNameDisplayFromText("pinned", ProjectNameDisplay::Both) ==
          ProjectNameDisplay::Pinned);
    CHECK(projectNameDisplayFromText("project-list",
                                     ProjectNameDisplay::Both) ==
          ProjectNameDisplay::Pinned);
    CHECK(projectNameDisplayFromText("recent", ProjectNameDisplay::Both) ==
          ProjectNameDisplay::Recent);
    CHECK(projectNameDisplayFromText("hidden", ProjectNameDisplay::Both) ==
          ProjectNameDisplay::None);
    CHECK(projectNameDisplayFromText("nonsense",
                                     ProjectNameDisplay::Pinned) ==
          ProjectNameDisplay::Pinned);
    CHECK_EQ(projectNameDisplayText(ProjectNameDisplay::Both), "both");
    CHECK_EQ(projectNameDisplayText(ProjectNameDisplay::Pinned), "pinned");
    CHECK_EQ(projectNameDisplayText(ProjectNameDisplay::Recent), "recent");
    CHECK_EQ(projectNameDisplayText(ProjectNameDisplay::None), "none");
}

TEST_CASE("config/menuThemeFromDocument reads projectNameDisplay") {
    json document = embeddedDefaultTheme();
    document["projectNameDisplay"] = "Pinned";
    CHECK(menuThemeFromDocument(document).projectNameDisplay ==
          ProjectNameDisplay::Pinned);

    document["projectNameDisplay"] = "invalid";
    const MenuTheme defaults = menuThemeFromDocument(embeddedDefaultTheme());
    CHECK(menuThemeFromDocument(document, &defaults).projectNameDisplay ==
          defaults.projectNameDisplay);
}

TEST_CASE("config/menuThemeFromDocument clamps a negative recentLimit") {
    json document = embeddedDefaultTheme();
    document["recentLimit"] = -4;
    CHECK_EQ(menuThemeFromDocument(document).recentLimit, 0);
}

// --- variables ---------------------------------------------------------------

TEST_CASE("config/variables hold colors and integer sizes in one table") {
    json document = embeddedDefaultTheme();
    document["variables"] = {{"accent", "cyan"},
                             {"Sidebar", 34},
                             {"sideWidth", "sidebar"}};
    const MenuTheme theme = menuThemeFromDocument(document);

    CHECK_EQ(theme.variables.at("accent"), "cyan");
    CHECK_EQ(theme.variables.at("sidebar"), "34");
    CHECK_EQ(theme.variables.at("sidewidth"), "sidebar");
}

TEST_CASE("config/variables override colors for the same name") {
    json document = embeddedDefaultTheme();
    document["colors"] = {{"accent", "red"}};
    document["variables"] = {{"accent", "blue"}};
    const MenuTheme theme = menuThemeFromDocument(document);
    CHECK_EQ(theme.variables.at("accent"), "blue");
}

TEST_CASE("config/colors remain available as the older name") {
    json document = embeddedDefaultTheme();
    document["colors"] = {{"Brand", "#ff0000"}};
    const MenuTheme theme = menuThemeFromDocument(document);
    CHECK_EQ(theme.variables.at("brand"), "#ff0000");
}

TEST_CASE("config/size variables resolve through references") {
    json document = embeddedDefaultTheme();
    document["variables"] = {{"sidebar", 34}, {"sideWidth", "sidebar"}};
    document["recentLimit"] = "sidebar";
    document["screens"]["main"]["width"] = "sideWidth";

    const MenuTheme theme = menuThemeFromDocument(document);
    CHECK_EQ(theme.recentLimit, 34);
    CHECK_EQ(theme.screens.at("main").width, 34);
}

TEST_CASE("config/numeric string sizes resolve") {
    json document = embeddedDefaultTheme();
    document["recentLimit"] = "7";
    CHECK_EQ(menuThemeFromDocument(document).recentLimit, 7);
}

TEST_CASE("config/unknown size names fall back to the default") {
    json document = embeddedDefaultTheme();
    document["variables"] = json::object();
    document["recentLimit"] = "not-defined";
    CHECK_EQ(menuThemeFromDocument(document).recentLimit,
             menuThemeFromDocument(embeddedDefaultTheme()).recentLimit);
}

TEST_CASE("config/variable cycles are reported and resolve to the fallback") {
    json document = embeddedDefaultTheme();
    document["variables"] = {{"loopa", "loopb"}, {"loopb", "loopa"}};
    document["recentLimit"] = "loopa";

    const MenuTheme theme = menuThemeFromDocument(document);
    CHECK_EQ(theme.recentLimit,
             menuThemeFromDocument(embeddedDefaultTheme()).recentLimit);
    REQUIRE(theme.warnings.size() == 1);
    CHECK(theme.warnings.front().find("variable cycle") != std::string::npos);
    CHECK(theme.warnings.front().find("loopa") != std::string::npos);
    CHECK(theme.warnings.front().find("loopb") != std::string::npos);
}

TEST_CASE("config/a self referencing variable is reported once") {
    json document = embeddedDefaultTheme();
    document["variables"] = {{"self", "self"}};
    const MenuTheme theme = menuThemeFromDocument(document);
    REQUIRE(theme.warnings.size() == 1);
    CHECK(theme.warnings.front().find("self") != std::string::npos);
}

TEST_CASE("config/the default theme supplies the color names") {
    const MenuTheme theme = menuThemeFromDocument(embeddedDefaultTheme());
    // No built-in names in code: everything comes from the theme table.
    CHECK_EQ(theme.variables.at("red"), "#cc0000");
    CHECK_EQ(theme.variables.at("success"), "green");
}

TEST_CASE("config/mergeThemeDefaults keeps the user's definitions untouched") {
    const json user = {{"addProjectLabel", "+ Mine"},
                       {"variables", {{"brand", "#123456"}}}};
    const json merged = mergeThemeDefaults(embeddedDefaultTheme(), user);
    // The user's block is used verbatim; no default entries are injected.
    REQUIRE(merged.contains("variables"));
    CHECK_EQ(merged.at("variables").size(), static_cast<std::size_t>(1));
    CHECK_EQ(merged.at("variables").at("brand").get<std::string>(), "#123456");
    CHECK_FALSE(merged.at("variables").contains("red"));
    // Everything else is still filled in.
    CHECK_EQ(merged.at("addProjectLabel").get<std::string>(), "+ Mine");
    CHECK(merged.contains("screens"));
}

TEST_CASE("config/mergeThemeDefaults adds no definitions when the user has none") {
    const json user = {{"addProjectLabel", "+ Mine"}};
    const json merged = mergeThemeDefaults(embeddedDefaultTheme(), user);
    CHECK_FALSE(merged.contains("variables"));
    CHECK_FALSE(merged.contains("colors"));
    CHECK(merged.contains("screens"));
}

TEST_CASE("config/mergeThemeDefaults preserves the older colors block") {
    const json user = {{"colors", {{"brand", "#123456"}}}};
    const json merged = mergeThemeDefaults(embeddedDefaultTheme(), user);
    REQUIRE(merged.contains("colors"));
    CHECK_EQ(merged.at("colors").size(), static_cast<std::size_t>(1));
}

TEST_CASE("config/healthy themes produce no variable warnings") {
    CHECK(menuThemeFromDocument(embeddedDefaultTheme()).warnings.empty());
}

TEST_CASE("config/resolveThemeInt accepts integers, numeric strings and names") {
    // Variable names are stored normalized, the same way the theme parser
    // stores them.
    const std::map<std::string, std::string> variables = {
        {"sidebar", "34"}, {"sidewidth", "sidebar"}};
    CHECK_EQ(resolveThemeInt(json(12), &variables).value_or(-1), 12);
    CHECK_EQ(resolveThemeInt(json("12"), &variables).value_or(-1), 12);
    CHECK_EQ(resolveThemeInt(json("Side Width"), &variables).value_or(-1), 34);
    CHECK_FALSE(resolveThemeInt(json("nope"), &variables).has_value());
    CHECK_FALSE(resolveThemeInt(json(""), &variables).has_value());
    CHECK_FALSE(resolveThemeInt(json(true), &variables).has_value());
    // A numeric string needs no table; a name does.
    CHECK_EQ(resolveThemeInt(json("12"), nullptr).value_or(-1), 12);
    CHECK_FALSE(resolveThemeInt(json("sidebar"), nullptr).has_value());
}

TEST_CASE("config/screenThemeFromJson only overrides valid fields") {
    ScreenTheme base;
    base.border = "light";
    base.width = 40;
    base.blocks = json::array({{{"type", "text"}}});

    const json value = {{"border", "double"},
                        {"width", "wide"},
                        {"align", "center"},
                        {"blocks", json::array()}};
    const ScreenTheme screen = screenThemeFromJson(value, base);
    CHECK_EQ(screen.border, "double");
    CHECK_EQ(screen.align, "center");
    CHECK_EQ(screen.width, 40);
    CHECK(screen.blocks.empty());

    const ScreenTheme untouched = screenThemeFromJson(json(), base);
    CHECK_EQ(untouched.border, "light");
}

// --- collectConfigIssues / mergeConfigDefaults -------------------------------

TEST_CASE("config/collectConfigIssues reports missing keys and wrong types") {
    const json defaults = {{"version", 1},
                           {"theme", "t.json"},
                           {"nested", {{"flag", true}, {"name", "n"}}},
                           {"list", json::array()}};
    const json user = {{"version", "one"},
                       {"nested", {{"flag", 1}}},
                       {"list", "not-a-list"}};

    std::vector<std::string> issues;
    collectConfigIssues(defaults, user, "", issues);

    auto has = [&](const std::string& text) {
        return std::find(issues.begin(), issues.end(), text) != issues.end();
    };
    CHECK(has("version: wrong type"));
    CHECK(has("theme: missing"));
    CHECK(has("nested.flag: wrong type"));
    CHECK(has("nested.name: missing"));
    CHECK(has("list: wrong type (expected array)"));
}

TEST_CASE("config/collectConfigIssues accepts a conforming document") {
    std::vector<std::string> issues;
    collectConfigIssues(embeddedDefaultConfig(), embeddedDefaultConfig(), "",
                        issues);
    CHECK(issues.empty());
}

TEST_CASE("config/collectConfigIssues special-cases flexible keys") {
    json user = embeddedDefaultConfig();
    user["claudePath"] = "single-string";
    user["claudeExclude"] = json::array({"a", "b"});
    user["launchCommand"] = "{claude}";
    std::vector<std::string> issues;
    collectConfigIssues(embeddedDefaultConfig(), user, "", issues);
    CHECK(issues.empty());

    user["claudePath"] = json::array({"ok", 5});
    user["launchCommand"] = 5;
    issues.clear();
    collectConfigIssues(embeddedDefaultConfig(), user, "", issues);
    auto has = [&](const std::string& text) {
        return std::find(issues.begin(), issues.end(), text) != issues.end();
    };
    CHECK(has("claudePath: wrong type (expected string or string array)"));
    CHECK(has("launchCommand: wrong type (expected string or platform object)"));
}

TEST_CASE("config/collectConfigIssues ignores non-object inputs") {
    std::vector<std::string> issues;
    collectConfigIssues(json::array(), json::object(), "", issues);
    collectConfigIssues(json::object(), json::array(), "", issues);
    CHECK(issues.empty());
}

TEST_CASE("config/mergeConfigDefaults fills gaps and keeps user values") {
    const json defaults = {{"a", 1}, {"b", "two"}, {"c", {{"x", 1}, {"y", 2}}}};
    const json user = {{"a", 10}, {"c", {{"x", 9}}}, {"extra", true}};

    const json merged = mergeConfigDefaults(defaults, user);
    CHECK_EQ(merged.at("a").get<int>(), 10);
    CHECK_EQ(merged.at("b").get<std::string>(), "two");
    CHECK_EQ(merged.at("c").at("x").get<int>(), 9);
    CHECK_EQ(merged.at("c").at("y").get<int>(), 2);
    CHECK(merged.at("extra").get<bool>());
}

TEST_CASE("config/mergeConfigDefaults replaces wrongly typed values") {
    const json defaults = {{"num", 1},
                           {"obj", {{"k", "v"}}},
                           {"arr", json::array({1})}};
    const json user = {{"num", "text"}, {"obj", 5}, {"arr", "no"}};
    const json merged = mergeConfigDefaults(defaults, user);
    CHECK_EQ(merged.at("num").get<int>(), 1);
    CHECK_EQ(merged.at("obj"), defaults.at("obj"));
    CHECK_EQ(merged.at("arr"), defaults.at("arr"));
}

TEST_CASE("config/mergeConfigDefaults special-cases flexible keys") {
    json user = embeddedDefaultConfig();
    user["claudePath"] = "custom";
    user["launchCommand"] = "{claude} --x";
    json merged = mergeConfigDefaults(embeddedDefaultConfig(), user);
    CHECK_EQ(merged.at("claudePath").get<std::string>(), "custom");
    CHECK_EQ(merged.at("launchCommand").get<std::string>(), "{claude} --x");

    user["claudePath"] = json::array({1});
    user["launchCommand"] = json{{"posix", "only-posix"}};
    merged = mergeConfigDefaults(embeddedDefaultConfig(), user);
    CHECK_EQ(merged.at("claudePath"), embeddedDefaultConfig().at("claudePath"));
    CHECK_EQ(merged.at("launchCommand").at("posix").get<std::string>(),
             "only-posix");
    CHECK_EQ(merged.at("launchCommand").at("windows"),
             embeddedDefaultConfig().at("launchCommand").at("windows"));
}

TEST_CASE("config/mergeConfigDefaults with non-objects prefers matching types") {
    CHECK_EQ(mergeConfigDefaults(json(1), json(2)), json(2));
    CHECK_EQ(mergeConfigDefaults(json(1), json("x")), json(1));
}

// --- empty values and obsolete keys -----------------------------------------

TEST_CASE("config/isEmptyDefaultConfigValue") {
    json document = embeddedDefaultConfig();
    CHECK_FALSE(isEmptyDefaultConfigValue(document, "theme"));
    CHECK_FALSE(hasEmptyDefaultConfigValues(document));

    document["theme"] = "   ";
    CHECK(isEmptyDefaultConfigValue(document, "theme"));
    CHECK(hasEmptyDefaultConfigValues(document));

    document = embeddedDefaultConfig();
    document["claudePath"] = json::array({"", "  "});
    CHECK(isEmptyDefaultConfigValue(document, "claudePath"));
    document["claudePath"] = json::array({"claude"});
    CHECK_FALSE(isEmptyDefaultConfigValue(document, "claudePath"));

    document["launchCommand"] = json{{"windows", " "}, {"posix", ""}};
    CHECK(isEmptyDefaultConfigValue(document, "launchCommand"));
    document["launchCommand"] = json{{"windows", " "}, {"posix", "x"}};
    CHECK_FALSE(isEmptyDefaultConfigValue(document, "launchCommand"));

    document.erase("tempDir");
    CHECK(isEmptyDefaultConfigValue(document, "tempDir"));
}

TEST_CASE("config/logFile may stay empty") {
    json document = embeddedDefaultConfig();
    document["logFile"] = "";
    // An empty logFile disables logging; it is not a value to repair.
    CHECK_FALSE(hasEmptyDefaultConfigValues(document));
}

TEST_CASE("config/materializeConfigDefaults fills blank values") {
    const json& defaults = embeddedDefaultConfig();
    json document = defaults;
    document["theme"] = "";
    document["tempDir"] = "  ";
    document["claudePath"] = json::array();
    document["launchCommand"] = json{{"windows", ""}, {"posix", "custom"}};
    document["claudeConfigDir"] = "kept-because-valid";

    materializeConfigDefaults(document, defaults);
    CHECK_EQ(document.at("theme"), defaults.at("theme"));
    CHECK_EQ(document.at("tempDir"), defaults.at("tempDir"));
    CHECK_EQ(document.at("claudePath"), defaults.at("claudePath"));
    CHECK_EQ(document.at("launchCommand").at("windows"),
             defaults.at("launchCommand").at("windows"));
    CHECK_EQ(document.at("launchCommand").at("posix").get<std::string>(),
             "custom");
    CHECK_EQ(document.at("claudeConfigDir").get<std::string>(),
             "kept-because-valid");
}

TEST_CASE("config/materializeLaunchCommand handles every shape") {
    const json& defaults = embeddedDefaultConfig();

    json missing = json::object();
    materializeLaunchCommand(missing, defaults);
    CHECK_EQ(missing.at("launchCommand"), defaults.at("launchCommand"));

    json blankString = {{"launchCommand", "  "}};
    materializeLaunchCommand(blankString, defaults);
    CHECK_EQ(blankString.at("launchCommand"), defaults.at("launchCommand"));

    json goodString = {{"launchCommand", "{claude}"}};
    materializeLaunchCommand(goodString, defaults);
    CHECK_EQ(goodString.at("launchCommand").get<std::string>(), "{claude}");

    json wrongType = {{"launchCommand", 5}};
    materializeLaunchCommand(wrongType, defaults);
    CHECK_EQ(wrongType.at("launchCommand"), defaults.at("launchCommand"));
}
