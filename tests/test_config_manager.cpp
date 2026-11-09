#include "test_framework.hpp"

#include "config_json.hpp"
#include "config_manager.hpp"
#include "text_util.hpp"

using namespace claudex;
using testfw::readFile;
using testfw::TempDir;
using testfw::writeFile;

namespace {

fs::path configPath(const TempDir& root) {
    return root.path() / "claudex-config.json";
}

json readJson(const fs::path& file) {
    return json::parse(readFile(file));
}

bool contains(const std::string& text, const std::string& needle) {
    return text.find(needle) != std::string::npos;
}

bool hasIssue(const ConfigManager& config, const std::string& issue) {
    const auto& issues = config.configIssues();
    return std::find(issues.begin(), issues.end(), issue) != issues.end();
}

}  // namespace

TEST_CASE("manager/fresh start without a config file") {
    TempDir root;
    ConfigManager config(root.path(), configPath(root));
    config.load();

    CHECK(config.fileWasMissing());
    CHECK(config.canRepair());
    CHECK_FALSE(config.loadHadError());
    CHECK(config.projects.empty());
    CHECK_EQ(config.claudePathText(), "auto");
    CHECK_FALSE(config.needsRepair());
    // The default theme is written on first start; the config only on save.
    CHECK(fs::exists(root.path() / "claudex-theme.json"));
    CHECK_FALSE(fs::exists(configPath(root)));
}

TEST_CASE("manager/defaults match the embedded resources") {
    TempDir root;
    ConfigManager config(root.path(), configPath(root));
    config.load();

    const Settings defaults = settingsFromDefault(embeddedDefaultConfig());
    CHECK_EQ(config.settings.themeFile, defaults.themeFile);
    CHECK_EQ(config.settings.configFile, defaults.configFile);
    CHECK(defaults.configFile.empty());
    CHECK_EQ(config.settings.claudePath, defaults.claudePath);
    CHECK_EQ(config.settings.claudeExclude, defaults.claudeExclude);
    CHECK_EQ(config.settings.launchCommand.windows,
             defaults.launchCommand.windows);
    CHECK_EQ(config.settings.launchCommand.posix,
             defaults.launchCommand.posix);
    CHECK_EQ(config.settings.claudeConfigDir, defaults.claudeConfigDir);
    CHECK_EQ(config.settings.tempDir, defaults.tempDir);
    CHECK_EQ(config.settings.logFile, defaults.logFile);
    CHECK_EQ(config.settings.defaultProjectName,
             defaults.defaultProjectName);
    REQUIRE(config.settings.defaultFocusTargets.size() ==
            defaults.defaultFocusTargets.size());
    for (std::size_t i = 0;
         i < config.settings.defaultFocusTargets.size(); ++i) {
        CHECK_EQ(config.settings.defaultFocusTargets[i].section,
                 defaults.defaultFocusTargets[i].section);
        CHECK_EQ(config.settings.defaultFocusTargets[i].match,
                 defaults.defaultFocusTargets[i].match);
        CHECK_EQ(config.settings.defaultFocusTargets[i].item,
                 defaults.defaultFocusTargets[i].item);
    }
}

TEST_CASE("manager/save then load round-trips projects") {
    TempDir root;
    {
        ConfigManager config(root.path(), configPath(root));
        config.load();
        config.projects.push_back({"proj-a", "2026-01-02T03:04:05", ""});
        config.projects.push_back({"proj-b", "", ""});
        std::string error;
        REQUIRE(config.save(error));
        CHECK(error.empty());
    }

    const json document = readJson(configPath(root));
    CHECK_EQ(document.at("version").get<int>(), 1);
    REQUIRE(document.at("projects").size() == 2);
    CHECK_EQ(document.at("projects").at(0).at("path").get<std::string>(),
             "proj-a");
    CHECK(document.at("launchCommand").is_object());

    ConfigManager reloaded(root.path(), configPath(root));
    reloaded.load();
    CHECK_FALSE(reloaded.fileWasMissing());
    CHECK_FALSE(reloaded.loadHadError());
    CHECK(reloaded.configIssues().empty());
    REQUIRE(reloaded.projects.size() == 2);
    CHECK_EQ(reloaded.projects[0].path, "proj-a");
    CHECK_EQ(reloaded.projects[0].lastUsed, "2026-01-02T03:04:05");
    CHECK_EQ(reloaded.projects[1].path, "proj-b");
}

TEST_CASE("manager/configFile round-trips through save and load") {
    TempDir root;
    {
        ConfigManager config(root.path(), configPath(root));
        config.load();
        CHECK(config.settings.configFile.empty());
        config.settings.configFile = "moved/claudex-config.json";
        std::string error;
        REQUIRE(config.save(error));
        CHECK(error.empty());
    }

    CHECK_EQ(readJson(configPath(root)).at("configFile").get<std::string>(),
             "moved/claudex-config.json");

    ConfigManager reloaded(root.path(), configPath(root));
    reloaded.load();
    CHECK_EQ(reloaded.settings.configFile, "moved/claudex-config.json");
}

TEST_CASE("manager/a missing configFile is repaired to empty") {
    TempDir root;
    json document = embeddedDefaultConfig();
    document.erase("configFile");
    writeFile(configPath(root), document.dump(2));

    ConfigManager config(root.path(), configPath(root));
    config.load();
    CHECK_FALSE(config.configIssues().empty());
    std::string error;
    REQUIRE(config.repair(error));
    CHECK_EQ(readJson(configPath(root)).at("configFile").get<std::string>(),
             "");
}

TEST_CASE("manager/save creates a nested config file and its folder") {
    TempDir root;
    const fs::path nested =
        root.path() / "deep" / "sub" / "claudex-config.json";
    ConfigManager config(root.path(), nested);
    config.load();
    REQUIRE(config.fileWasMissing());

    std::string error;
    REQUIRE(config.save(error));
    CHECK(error.empty());
    CHECK(fs::exists(nested));
    CHECK_EQ(readJson(nested).at("configFile").get<std::string>(), "");
}

TEST_CASE("manager/save writes through a temporary file") {
    TempDir root;
    ConfigManager config(root.path(), configPath(root));
    config.load();
    std::string error;
    REQUIRE(config.save(error));
    CHECK_FALSE(fs::exists(root.path() / "claudex-config.json.tmp"));
}

TEST_CASE("manager/invalid JSON is reported and backed up on save") {
    TempDir root;
    writeFile(configPath(root), "{ this is not json");

    ConfigManager config(root.path(), configPath(root));
    config.load();
    CHECK(config.loadHadError());
    CHECK_FALSE(config.canRepair());
    CHECK_FALSE(config.loadWarning().empty());
    // Defaults stay usable.
    CHECK_FALSE(config.settings.claudePath.empty());

    std::string error;
    REQUIRE(config.save(error));
    CHECK_EQ(readFile(root.path() / "claudex-config.json.invalid"),
             "{ this is not json");
    CHECK_FALSE(config.loadHadError());
    CHECK(readJson(configPath(root)).is_object());
}

TEST_CASE("manager/repair refuses to run on unreadable configs") {
    TempDir root;
    writeFile(configPath(root), "garbage");
    ConfigManager config(root.path(), configPath(root));
    config.load();
    std::string error;
    CHECK_FALSE(config.repair(error));
    CHECK_FALSE(error.empty());
}

TEST_CASE("manager/array and scalar roots are rejected") {
    for (const char* content : {"[]", "42", "\"text\""}) {
        TempDir root;
        writeFile(configPath(root), content);
        ConfigManager config(root.path(), configPath(root));
        config.load();
        CHECK(config.loadHadError());
        CHECK_FALSE(config.canRepair());
    }
}

TEST_CASE("manager/UTF-8 BOM is ignored") {
    TempDir root;
    writeFile(configPath(root),
              "\xEF\xBB\xBF{\"projects\":[{\"path\":\"bom-project\"}]}");
    ConfigManager config(root.path(), configPath(root));
    config.load();
    CHECK_FALSE(config.loadHadError());
    REQUIRE(config.projects.size() == 1);
    CHECK_EQ(config.projects[0].path, "bom-project");
}

TEST_CASE("manager/projects must be an array") {
    TempDir root;
    writeFile(configPath(root), "{\"projects\": 5}");
    ConfigManager config(root.path(), configPath(root));
    config.load();
    CHECK(config.loadHadError());
    CHECK(config.projects.empty());
}

TEST_CASE("manager/project list keeps repeated paths and drops invalid entries") {
    TempDir root;
    writeFile(configPath(root), R"({
        "projects": [
            {"path": "a"},
            {"path": "  a  "},
            {"path": ""},
            {"path": "   "},
            5,
            {"nopath": true},
            {"path": 7},
            {"path": "b", "lastUsed": "2026-05-06T07:08:09"},
            {"path": "./a"}
        ]
    })");
    ConfigManager config(root.path(), configPath(root));
    config.load();
    // Repeating a path is allowed: only unusable entries are dropped.
    REQUIRE(config.projects.size() == 4);
    CHECK_EQ(config.projects[0].path, "a");
    CHECK_EQ(config.projects[1].path, "a");
    CHECK_EQ(config.projects[2].path, "b");
    CHECK_EQ(config.projects[2].lastUsed, "2026-05-06T07:08:09");
    CHECK_EQ(config.projects[3].path, "./a");
}

TEST_CASE("manager/project names load, trim and save") {
    TempDir root;
    writeFile(configPath(root), R"({
        "projects": [
            {"path": "a", "name": "  <red>A</red>  "},
            {"path": "b"},
            {"path": "c", "name": ""},
            {"path": "d", "name": 7}
        ]
    })");
    ConfigManager config(root.path(), configPath(root));
    config.load();
    REQUIRE(config.projects.size() == 4);
    CHECK_EQ(config.projects[0].name, "<red>A</red>");
    CHECK(config.projects[1].name.empty());
    CHECK(config.projects[2].name.empty());
    CHECK(config.projects[3].name.empty());

    std::string error;
    REQUIRE(config.save(error));
    const json document = readJson(configPath(root));
    CHECK_EQ(document.at("projects").at(0).at("name").get<std::string>(),
             "<red>A</red>");
    // Empty names are not written at all.
    CHECK_FALSE(document.at("projects").at(1).contains("name"));
    CHECK_FALSE(document.at("projects").at(2).contains("name"));

    ConfigManager reloaded(root.path(), configPath(root));
    reloaded.load();
    CHECK_EQ(reloaded.projects[0].name, "<red>A</red>");
}

TEST_CASE("manager/findProject compares resolved paths") {
    TempDir root;
    fs::create_directories(root.path() / "a");
    fs::create_directories(root.path() / "b");
    ConfigManager config(root.path(), configPath(root));
    config.load();
    config.projects.push_back({"a", "", ""});
    config.projects.push_back({"{root}/b", "", ""});

    CHECK_EQ(config.findProject(root.path() / "a"), 0);
    CHECK_EQ(config.findProject(root.path() / "b"), 1);
    CHECK_EQ(config.findProject(root.path() / "a" / ".." / "b"), 1);
    CHECK_EQ(config.findProject(root.path() / "missing"), -1);
}

TEST_CASE("manager/resolveProjectPath expands templates") {
    TempDir root;
    ConfigManager config(root.path(), configPath(root));
    config.load();
    CHECK(pathsEqual(config.resolveProjectPath(std::string("{root}/x")),
                     root.path() / "x"));
    Project project{"y", "", ""};
    CHECK(pathsEqual(config.resolveProjectPath(project), root.path() / "y"));
}

TEST_CASE("manager/reads every setting") {
    TempDir root;
    writeFile(configPath(root), R"({
        "claudePath": "bin/claude",
        "claudeExclude": [],
        "launchCommand": "{claude} --model x",
        "claudeConfigDir": "cfg",
        "tempDir": "scratch",
        "logFile": "",
        "tabOnlySectionSwitch": true,
        "defaultProjectName": "<accent>Starter</accent>",
        "defaultFocus": { "section": "recent", "item": 2 }
    })");
    ConfigManager config(root.path(), configPath(root));
    config.load();

    CHECK_FALSE(config.loadHadError());
    CHECK_EQ(config.settings.claudePath, std::vector<std::string>{"bin/claude"});
    CHECK(config.settings.claudeExclude.empty());
    CHECK_EQ(config.settings.launchCommand.windows, "{claude} --model x");
    CHECK_EQ(config.settings.launchCommand.posix, "{claude} --model x");
    CHECK_EQ(config.settings.claudeConfigDir, "cfg");
    CHECK_EQ(config.settings.tempDir, "scratch");
    CHECK(config.settings.tabOnlySectionSwitch);
    CHECK_EQ(config.settings.defaultProjectName, "<accent>Starter</accent>");
    REQUIRE(config.settings.defaultFocusTargets.size() == 1);
    CHECK_EQ(config.settings.defaultFocusTargets.front().section, "recent");
    CHECK(config.settings.defaultFocusTargets.front().match.empty());
    CHECK_EQ(config.settings.defaultFocusTargets.front().item, 2);
    CHECK(config.logFilePath().empty());
    CHECK(pathsEqual(config.claudeConfigDir(), root.path() / "cfg"));
    CHECK(pathsEqual(config.tempDir(), root.path() / "scratch"));
}

TEST_CASE("manager/claudePathText reflects explicit configuration") {
    {
        TempDir root;
        writeFile(configPath(root), "{\"claudePath\": [\"a\", \"b\"]}");
        ConfigManager config(root.path(), configPath(root));
        config.load();
        CHECK_EQ(config.claudePathText(), "a ; b");
        CHECK_EQ(config.firstClaudePathText(), "a");
    }
    {
        // Spelling out the default value is not "configured".
        TempDir root;
        writeFile(configPath(root), "{\"claudePath\": [\"claude*\"]}");
        ConfigManager config(root.path(), configPath(root));
        config.load();
        CHECK_EQ(config.claudePathText(), "auto");
        CHECK_EQ(config.firstClaudePathText(), "auto");
    }
    {
        // Blank entries fall back to the default.
        TempDir root;
        writeFile(configPath(root), "{\"claudePath\": [\"  \"]}");
        ConfigManager config(root.path(), configPath(root));
        config.load();
        CHECK_EQ(config.claudePathText(), "auto");
        CHECK_FALSE(config.settings.claudePath.empty());
    }
}

TEST_CASE("manager/blank paths fall back to defaults") {
    TempDir root;
    writeFile(configPath(root),
              "{\"theme\": \" \", \"claudeConfigDir\": \"\", \"tempDir\": \" \"}");
    ConfigManager config(root.path(), configPath(root));
    config.load();
    const Settings defaults = settingsFromDefault(embeddedDefaultConfig());
    CHECK_EQ(config.settings.themeFile, defaults.themeFile);
    CHECK_EQ(config.settings.claudeConfigDir, defaults.claudeConfigDir);
    CHECK_EQ(config.settings.tempDir, defaults.tempDir);
}

TEST_CASE("manager/default config and temp folders live under the root") {
    TempDir root;
    ConfigManager config(root.path(), configPath(root));
    config.load();
    CHECK(pathsEqual(config.claudeConfigDir(), root.path() / "data"));
    CHECK(pathsEqual(config.tempDir(), root.path() / "data" / "tmp"));
}

// --- logging ---------------------------------------------------------------

TEST_CASE("manager/log appends timestamped single-line entries") {
    TempDir root;
    writeFile(configPath(root), "{\"logFile\": \"logs/test.log\"}");
    ConfigManager config(root.path(), configPath(root));
    config.load();

    config.log("INFO", "first\nsecond\r\nthird");
    config.log("ERROR", "again");

    const std::string content = readFile(root.path() / "logs" / "test.log");
    CHECK(contains(content, "INFO first second  third\n"));
    CHECK(contains(content, "ERROR again\n"));
    CHECK(content.front() == '[');
}

TEST_CASE("manager/an empty logFile disables logging") {
    TempDir root;
    writeFile(configPath(root), "{\"logFile\": \"\"}");
    ConfigManager config(root.path(), configPath(root));
    config.load();
    config.log("INFO", "nothing");
    CHECK_FALSE(fs::exists(root.path() / "claudex.log"));
}

TEST_CASE("manager/the log never overwrites the config file") {
    TempDir root;
    writeFile(configPath(root), "{\"logFile\": \"claudex-config.json\"}");
    const std::string before = readFile(configPath(root));
    ConfigManager config(root.path(), configPath(root));
    config.load();
    config.log("INFO", "must not be written");
    CHECK_EQ(readFile(configPath(root)), before);
}

// --- repair ----------------------------------------------------------------

TEST_CASE("manager/repair fills missing keys and keeps a backup") {
    TempDir root;
    writeFile(configPath(root), R"({"projects": [{"path": "keep-me"}]})");

    ConfigManager config(root.path(), configPath(root));
    config.load();
    REQUIRE(config.canRepair());
    CHECK(config.needsRepair());
    CHECK(hasIssue(config, "theme: missing"));
    CHECK(hasIssue(config, "launchCommand: missing"));

    std::string error;
    REQUIRE(config.repair(error));
    CHECK(error.empty());
    CHECK(fs::exists(root.path() / "claudex-config.json.before-fix"));
    CHECK(config.configIssues().empty());
    CHECK_FALSE(config.needsRepair());

    const json repaired = readJson(configPath(root));
    CHECK(repaired.contains("theme"));
    CHECK(repaired.contains("launchCommand"));
    CHECK_EQ(repaired.at("projects").at(0).at("path").get<std::string>(),
             "keep-me");

    ConfigManager reloaded(root.path(), configPath(root));
    reloaded.load();
    CHECK(reloaded.configIssues().empty());
    CHECK_FALSE(reloaded.needsRepair());
    REQUIRE(reloaded.projects.size() == 1);
}

TEST_CASE("manager/repair is a no-op for healthy configs") {
    TempDir root;
    ConfigManager config(root.path(), configPath(root));
    config.load();
    std::string error;
    REQUIRE(config.save(error));
    config.load();
    CHECK_FALSE(config.needsRepair());
    CHECK(config.repair(error));
    CHECK_FALSE(fs::exists(root.path() / "claudex-config.json.before-fix"));
}

TEST_CASE("manager/wrongly typed values are reported as issues") {
    TempDir root;
    json document = embeddedDefaultConfig();
    document["tabOnlySectionSwitch"] = "yes";
    document["defaultProjectName"] = 7;
    writeFile(configPath(root), document.dump(2));
    ConfigManager config(root.path(), configPath(root));
    config.load();
    CHECK(hasIssue(config, "tabOnlySectionSwitch: wrong type"));
    CHECK(hasIssue(config, "defaultProjectName: wrong type"));
    // The bad values are ignored while loading.
    CHECK_FALSE(config.settings.tabOnlySectionSwitch);
    CHECK(config.settings.defaultProjectName.empty());
}

TEST_CASE("manager/defaultProjectName round-trips through save and load") {
    TempDir root;
    {
        ConfigManager config(root.path(), configPath(root));
        config.load();
        config.settings.defaultProjectName = "<accent>Starter</accent>";
        std::string error;
        REQUIRE(config.save(error));
        CHECK(error.empty());
    }

    CHECK_EQ(readJson(configPath(root))
                 .at("defaultProjectName")
                 .get<std::string>(),
             "<accent>Starter</accent>");

    ConfigManager reloaded(root.path(), configPath(root));
    reloaded.load();
    CHECK_EQ(reloaded.settings.defaultProjectName,
             "<accent>Starter</accent>");
}

TEST_CASE("manager/a single defaultFocus target is saved as an object") {
    TempDir root;
    {
        ConfigManager config(root.path(), configPath(root));
        config.load();
        config.settings.defaultFocusTargets = {{"recent", "F:/work/app", -1}};
        std::string error;
        REQUIRE(config.save(error));
        CHECK(error.empty());
    }

    const json document = readJson(configPath(root));
    CHECK(document.at("defaultFocus").is_object());
    CHECK_EQ(document.at("defaultFocus").at("section").get<std::string>(),
             "recent");
    CHECK_EQ(document.at("defaultFocus").at("match").get<std::string>(),
             "F:/work/app");
    CHECK_EQ(document.at("defaultFocus").at("item").get<int>(), -1);
}

TEST_CASE("manager/defaultFocus targets round-trip through save and load") {
    TempDir root;
    {
        ConfigManager config(root.path(), configPath(root));
        config.load();
        config.settings.defaultFocusTargets = {
            {"recent", "F:/work/app", -1},
            {"actions", "edit", 0},
            {"path", "", 0},
        };
        std::string error;
        REQUIRE(config.save(error));
        CHECK(error.empty());
    }

    const json document = readJson(configPath(root));
    REQUIRE(document.at("defaultFocus").is_array());
    REQUIRE(document.at("defaultFocus").size() == 3);
    CHECK_EQ(document.at("defaultFocus").at(0).at("section").get<std::string>(),
             "recent");
    CHECK_EQ(document.at("defaultFocus").at(1).at("match").get<std::string>(),
             "edit");
    CHECK_EQ(document.at("defaultFocus").at(2).at("section").get<std::string>(),
             "path");

    ConfigManager reloaded(root.path(), configPath(root));
    reloaded.load();
    REQUIRE(reloaded.settings.defaultFocusTargets.size() == 3);
    CHECK_EQ(reloaded.settings.defaultFocusTargets[0].section, "recent");
    CHECK_EQ(reloaded.settings.defaultFocusTargets[0].match, "F:/work/app");
    CHECK_EQ(reloaded.settings.defaultFocusTargets[0].item, -1);
    CHECK_EQ(reloaded.settings.defaultFocusTargets[1].section, "actions");
    CHECK_EQ(reloaded.settings.defaultFocusTargets[1].match, "edit");
    CHECK_EQ(reloaded.settings.defaultFocusTargets[2].section, "path");
}

TEST_CASE("manager/a defaultFocus array keeps its shape and fills missing "
          "fields on save") {
    TempDir root;
    json document = embeddedDefaultConfig();
    const json recent = {{"section", "recent"}};
    const json actions = {{"section", "actions"}, {"match", "edit"}};
    document["defaultFocus"] = json::array({recent, actions});
    writeFile(configPath(root), document.dump(2));

    ConfigManager config(root.path(), configPath(root));
    config.load();
    // An array is a valid shape; the shipped default is also an array.
    CHECK(config.configIssues().empty());
    REQUIRE(config.settings.defaultFocusTargets.size() == 2);
    CHECK_EQ(config.settings.defaultFocusTargets[0].section, "recent");
    CHECK_EQ(config.settings.defaultFocusTargets[0].item, 0);
    CHECK_EQ(config.settings.defaultFocusTargets[1].match, "edit");

    std::string error;
    REQUIRE(config.save(error));
    const json saved = readJson(configPath(root));
    REQUIRE(saved.at("defaultFocus").is_array());
    REQUIRE(saved.at("defaultFocus").size() == 2);
    CHECK_EQ(saved.at("defaultFocus").at(0).at("match").get<std::string>(),
             "");
    CHECK_EQ(saved.at("defaultFocus").at(0).at("item").get<int>(), 0);
}

TEST_CASE("manager/a defaultFocus object without optional fields is "
          "accepted") {
    TempDir root;
    json document = embeddedDefaultConfig();
    document["defaultFocus"] = {{"section", "recent"}};
    writeFile(configPath(root), document.dump(2));

    ConfigManager config(root.path(), configPath(root));
    config.load();
    CHECK(config.configIssues().empty());
    REQUIRE(config.settings.defaultFocusTargets.size() == 1);
    CHECK_EQ(config.settings.defaultFocusTargets.front().section, "recent");
    CHECK_EQ(config.settings.defaultFocusTargets.front().item, 0);
}

TEST_CASE("manager/a missing defaultFocus is repaired to the default") {
    TempDir root;
    json document = embeddedDefaultConfig();
    document.erase("defaultFocus");
    writeFile(configPath(root), document.dump(2));

    ConfigManager config(root.path(), configPath(root));
    config.load();
    CHECK(hasIssue(config, "defaultFocus: missing"));
    std::string error;
    REQUIRE(config.repair(error));
    const json repaired = readJson(configPath(root)).at("defaultFocus");
    REQUIRE(repaired.is_array());
    REQUIRE(repaired.size() == 4);
    const std::vector<std::string> expectedSections = {
        "cwd", "projects", "recent", "quickInput"};
    for (std::size_t i = 0; i < expectedSections.size(); ++i) {
        CHECK_EQ(repaired.at(i).at("section").get<std::string>(),
                 expectedSections[i]);
        CHECK_EQ(repaired.at(i).at("match").get<std::string>(), "");
        CHECK_EQ(repaired.at(i).at("item").get<int>(), 0);
    }
}

TEST_CASE("manager/a missing defaultProjectName is repaired to empty") {
    TempDir root;
    json document = embeddedDefaultConfig();
    document.erase("defaultProjectName");
    writeFile(configPath(root), document.dump(2));

    ConfigManager config(root.path(), configPath(root));
    config.load();
    CHECK(hasIssue(config, "defaultProjectName: missing"));
    std::string error;
    REQUIRE(config.repair(error));
    CHECK_EQ(readJson(configPath(root))
                 .at("defaultProjectName")
                 .get<std::string>(),
             "");
}

// --- theme file --------------------------------------------------------------

TEST_CASE("manager/a custom theme path is created on demand") {
    TempDir root;
    writeFile(configPath(root), "{\"theme\": \"themes/mine.json\"}");
    ConfigManager config(root.path(), configPath(root));
    config.load();
    CHECK(pathsEqual(config.themeFile(), root.path() / "themes" / "mine.json"));
    CHECK(fs::exists(root.path() / "themes" / "mine.json"));
    CHECK(readJson(root.path() / "themes" / "mine.json").is_object());
}

TEST_CASE("manager/the theme file is merged with defaults and backed up") {
    TempDir root;
    writeFile(root.path() / "claudex-theme.json",
              "{\"addProjectLabel\": \"+ Custom\"}");
    ConfigManager config(root.path(), configPath(root));
    config.load();

    const MenuTheme defaults = menuThemeFromDocument(embeddedDefaultTheme());
    CHECK_EQ(config.menuTheme.addProjectLabel, "+ Custom");
    CHECK_EQ(config.menuTheme.titleFormat, defaults.titleFormat);
    CHECK_EQ(config.menuTheme.messages.size(), defaults.messages.size());

    CHECK(fs::exists(root.path() / "claudex-theme.json.before-fix"));
    const json written = readJson(root.path() / "claudex-theme.json");
    CHECK_EQ(written.at("addProjectLabel").get<std::string>(), "+ Custom");
    CHECK(written.contains("screens"));
    CHECK_FALSE(config.themeStatusIsError());
}

TEST_CASE("manager/an invalid theme falls back to defaults and is replaced") {
    TempDir root;
    writeFile(root.path() / "claudex-theme.json", "{ not json");
    ConfigManager config(root.path(), configPath(root));
    config.load();

    const MenuTheme defaults = menuThemeFromDocument(embeddedDefaultTheme());
    CHECK_EQ(config.menuTheme.addProjectLabel, defaults.addProjectLabel);
    CHECK_EQ(readFile(root.path() / "claudex-theme.json.invalid"),
             "{ not json");
    CHECK(readJson(root.path() / "claudex-theme.json").is_object());
}

TEST_CASE("manager/theme repair never writes the default definitions") {
    TempDir root;
    writeFile(root.path() / "claudex-theme.json",
              "{\"addProjectLabel\": \"+ Custom\"}");
    ConfigManager config(root.path(), configPath(root));
    config.load();

    const json written = readJson(root.path() / "claudex-theme.json");
    CHECK_FALSE(written.contains("variables"));
    // The palette still resolves at runtime through the default theme.
    CHECK_EQ(config.menuTheme.variables.count("red"), static_cast<std::size_t>(1));
    CHECK_EQ(config.menuTheme.variables.count("accent"),
             static_cast<std::size_t>(1));

    // A second load must not see the absent definitions as a missing key.
    const std::string before = readFile(root.path() / "claudex-theme.json");
    ConfigManager second(root.path(), configPath(root));
    second.load();
    CHECK_EQ(readFile(root.path() / "claudex-theme.json"), before);
}

TEST_CASE("manager/a complete theme is left untouched") {
    TempDir root;
    {
        ConfigManager first(root.path(), configPath(root));
        first.load();
    }
    const std::string before = readFile(root.path() / "claudex-theme.json");
    ConfigManager second(root.path(), configPath(root));
    second.load();
    CHECK_EQ(readFile(root.path() / "claudex-theme.json"), before);
    CHECK_FALSE(fs::exists(root.path() / "claudex-theme.json.before-fix"));
}

TEST_CASE("manager/path helpers delegate to the resolver") {
    TempDir root;
    ConfigManager config(root.path(), configPath(root));
    config.load();

    CHECK_EQ(config.expandPath("{root}/x"), pathToUtf8(root.path()) + "/x");
    CHECK(pathsEqual(config.normalizeAbsolutePath("a/../b"),
                     root.path() / "b"));
    CHECK_EQ(config.storeProjectPath("{root}/p", root.path() / "p"),
             "{root}/p");
    CHECK(pathsEqual(config.rootDir(), root.path()));
    CHECK(pathsEqual(config.configFile(), configPath(root)));

    PathResolution resolved = config.resolvePath(
        "nothing-*", PathKind::Any, MultiMatchPolicy::FirstSorted);
    CHECK(resolved.noMatch);
}
