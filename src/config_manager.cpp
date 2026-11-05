#include "config_manager.hpp"

#include "config_json.hpp"
#include "default_focus.hpp"
#include "text_util.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fstream>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace claudex {

ConfigManager::ConfigManager(fs::path rootDir, fs::path configFile)
    : rootDir_(std::move(rootDir)),
      configFile_(std::move(configFile)),
      pathResolver_(rootDir_),
      defaultDocument_(embeddedDefaultConfig()),
      defaultThemeDocument_(embeddedDefaultTheme()),
      defaultSettings_(settingsFromDefault(defaultDocument_)),
      defaultMenuTheme_(
          menuThemeFromDocument(defaultThemeDocument_)) {
    settings = defaultSettings_;
    menuTheme = defaultMenuTheme_;
    themeDocument_ = defaultThemeDocument_;
    userDocument_ = defaultDocument_;
}

void ConfigManager::load() {
    loadHadError_ = false;
    loadWarning_.clear();
    fileWasMissing_ = false;
    canRepair_ = false;
    configIssues_.clear();
    settings = defaultSettings_;
    claudePathConfigured_ = false;
    menuTheme = defaultMenuTheme_;
    themeDocument_ = defaultThemeDocument_;
    themeFile_.clear();
    themeStatus_.clear();
    themeStatusIsError_ = false;
    themeLoadState_ = ThemeLoadState::Loaded;
    themeNeedsWrite_ = false;
    projects.clear();
    userDocument_ = defaultDocument_;

    std::error_code ec;
    const bool exists = fs::exists(configFile_, ec);
    if (ec) {
        loadHadError_ = true;
        loadWarning_ =
            messageText(menuTheme, "configInspectFailed",
                        {{"path", pathToUtf8(configFile_)},
                         {"error", ec.message()}});
        loadTheme();
        return;
    }
    if (!exists) {
        fileWasMissing_ = true;
        canRepair_ = true;
        loadTheme();
        return;
    }

    std::ifstream input(configFile_, std::ios::binary);
    if (!input) {
        loadHadError_ = true;
        loadWarning_ = messageText(
            menuTheme, "configOpenFailed",
            {{"path", pathToUtf8(configFile_)}});
        loadTheme();
        return;
    }
    std::string contents((std::istreambuf_iterator<char>(input)),
                         std::istreambuf_iterator<char>());
    if (contents.size() >= 3 &&
        static_cast<unsigned char>(contents[0]) == 0xEF &&
        static_cast<unsigned char>(contents[1]) == 0xBB &&
        static_cast<unsigned char>(contents[2]) == 0xBF) {
        contents.erase(0, 3);
    }

    try {
        json document = json::parse(contents);
        if (document.is_array()) {
            loadHadError_ = true;
            loadWarning_ =
                messageText(menuTheme, "configOldArrayFormat");
            loadTheme();
            return;
        }
        if (!document.is_object()) {
            loadHadError_ = true;
            loadWarning_ =
                messageText(menuTheme, "configUnsupportedFormat");
            loadTheme();
            return;
        }

        userDocument_ = document;
        canRepair_ = true;
        loadSettings(document);
        loadTheme();
        if (document.contains("projects")) {
            if (document["projects"].is_array()) {
                loadProjects(document["projects"]);
            } else {
                loadHadError_ = true;
                loadWarning_ =
                    messageText(menuTheme, "configProjectsNotArray");
            }
        }
        computeConfigIssues();
    } catch (const std::exception& exception) {
        settings = defaultSettings_;
        menuTheme = defaultMenuTheme_;
        projects.clear();
        userDocument_ = defaultDocument_;
        loadTheme();
        loadHadError_ = true;
        loadWarning_ =
            messageText(menuTheme, "configParseFailed",
                        {{"path", pathToUtf8(configFile_)},
                         {"error", exception.what()}});
    }
}

bool ConfigManager::save(std::string& error) {
    error.clear();

    std::string themeError;
    if (!writeThemeIfNeeded(themeError)) {
        log("ERROR", "theme: " + themeError);
    }

    json document =
        userDocument_.is_object() ? userDocument_ : defaultDocument_;
    document["version"] = defaultDocument_.at("version");
    document["theme"] = settings.themeFile;
    document["configFile"] = settings.configFile;
    document["claudePath"] = settings.claudePath;
    document["claudeExclude"] = settings.claudeExclude;
    json launchCommand;
    launchCommand["windows"] = settings.launchCommand.windows;
    launchCommand["posix"] = settings.launchCommand.posix;
    document["launchCommand"] = std::move(launchCommand);
    document["claudeConfigDir"] = settings.claudeConfigDir;
    document["tempDir"] = settings.tempDir;
    document["logFile"] = settings.logFile;
    document["tabOnlySectionSwitch"] = settings.tabOnlySectionSwitch;
    document["defaultProjectName"] = settings.defaultProjectName;
    document["defaultFocus"] =
        defaultFocusListToJson(settings.defaultFocusTargets);
    materializeConfigDefaults(document, defaultDocument_);

    json projectArray = json::array();
    for (const Project& project : projects) {
        json item;
        item["path"] = project.path;
        item["lastUsed"] = project.lastUsed;
        if (!project.name.empty()) {
            item["name"] = project.name;
        }
        projectArray.push_back(std::move(item));
    }
    document["projects"] = std::move(projectArray);

    if (!writeDocument(document, error)) {
        return false;
    }

    userDocument_ = document;
    computeConfigIssues();
    log("INFO", "config saved to " + pathToUtf8(configFile_));
    return true;
}

bool ConfigManager::repair(std::string& error) {
    error.clear();
    if (!canRepair_) {
        error = messageText(menuTheme, "configCannotRepair",
                            {{"warning", loadWarning_}});
        return false;
    }
    if (!needsRepair()) {
        return true;
    }

    json repaired =
        mergeConfigDefaults(defaultDocument_, userDocument_);
    materializeConfigDefaults(repaired, defaultDocument_);

    std::string themeError;
    if (!writeThemeIfNeeded(themeError)) {
        log("ERROR", "theme: " + themeError);
    }

    std::error_code ec;
    fs::path backup = configFile_;
    backup += ".before-fix";
    if (fs::exists(configFile_, ec) && !ec) {
        fs::copy_file(configFile_, backup,
                      fs::copy_options::overwrite_existing, ec);
        if (ec) {
            error = messageText(
                menuTheme, "configBackupFailed",
                {{"backup", pathToUtf8(backup)},
                 {"error", ec.message()}});
            return false;
        }
    }

    if (!writeDocument(repaired, error)) {
        return false;
    }

    userDocument_ = repaired;
    projects.clear();
    if (repaired.contains("projects") &&
        repaired["projects"].is_array()) {
        loadProjects(repaired["projects"]);
    }
    computeConfigIssues();
    log("INFO", "config repaired; backup=" + pathToUtf8(backup));
    return true;
}

bool ConfigManager::needsRepair() const {
    if (!configIssues_.empty() ||
        hasEmptyDefaultConfigValues(userDocument_)) {
        return true;
    }
    return themeLoadState_ == ThemeLoadState::Missing ||
           themeLoadState_ == ThemeLoadState::Invalid || themeNeedsWrite_;
}

std::string ConfigManager::claudePathText() const {
    if (!claudePathConfigured_ || settings.claudePath.empty()) {
        return "auto";
    }
    std::string text;
    for (const std::string& entry : settings.claudePath) {
        if (!text.empty()) {
            text += " ; ";
        }
        text += entry;
    }
    return text;
}

std::string ConfigManager::firstClaudePathText() const {
    if (!claudePathConfigured_ || settings.claudePath.empty()) {
        return "auto";
    }
    return settings.claudePath.front();
}

int ConfigManager::findProject(const fs::path& absolutePath) const {
    for (std::size_t i = 0; i < projects.size(); ++i) {
        if (pathsEqual(resolveProjectPath(projects[i]), absolutePath)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

fs::path ConfigManager::claudeConfigDir() const {
    return pathResolver_
        .resolve(settings.claudeConfigDir, PathKind::Directory,
                 MultiMatchPolicy::FirstSorted)
        .path;
}

fs::path ConfigManager::tempDir() const {
    return pathResolver_
        .resolve(settings.tempDir, PathKind::Directory,
                 MultiMatchPolicy::FirstSorted)
        .path;
}

fs::path ConfigManager::logFilePath() const {
    const std::string value = trim(settings.logFile);
    if (value.empty()) {
        return {};
    }
    return pathResolver_
        .resolve(value, PathKind::File, MultiMatchPolicy::FirstSorted)
        .path;
}

void ConfigManager::log(const std::string& level, const std::string& message) const {
    const fs::path path = logFilePath();
    if (path.empty() || pathsEqual(path, configFile_)) {
        return;
    }

    std::error_code ec;
    if (!path.parent_path().empty()) {
        fs::create_directories(path.parent_path(), ec);
        if (ec) {
            return;
        }
    }

    std::ofstream output(path, std::ios::binary | std::ios::app);
    if (!output) {
        return;
    }

    std::string line = message;
    std::replace(line.begin(), line.end(), '\n', ' ');
    std::replace(line.begin(), line.end(), '\r', ' ');
    output << '[' << logTimestamp() << "] " << level << ' ' << line
           << '\n';
}

void ConfigManager::loadSettings(const json& document) {
    if (document.contains("theme") && document["theme"].is_string()) {
        std::string value = document["theme"].get<std::string>();
        settings.themeFile =
            trim(value).empty() ? defaultSettings_.themeFile
                                : std::move(value);
    }
    if (document.contains("configFile") &&
        document["configFile"].is_string()) {
        // Empty means "keep this file"; keep the value as written so a
        // relative target stays relative.
        settings.configFile = document["configFile"].get<std::string>();
    }
    if (document.contains("claudePath")) {
        const json& value = document["claudePath"];
        std::vector<std::string> entries;
        bool valid = false;
        if (value.is_string()) {
            valid = true;
            std::string entry = value.get<std::string>();
            if (!trim(entry).empty()) {
                entries.push_back(std::move(entry));
            }
        } else if (value.is_array()) {
            valid = true;
            for (const json& item : value) {
                if (!item.is_string()) {
                    valid = false;
                    entries.clear();
                    break;
                }
                std::string entry = item.get<std::string>();
                if (!trim(entry).empty()) {
                    entries.push_back(std::move(entry));
                }
            }
        }
        if (valid) {
            if (entries.empty()) {
                settings.claudePath = defaultSettings_.claudePath;
                claudePathConfigured_ = false;
            } else {
                settings.claudePath = std::move(entries);
                claudePathConfigured_ =
                    settings.claudePath != defaultSettings_.claudePath;
            }
        }
    }
    if (document.contains("claudeExclude")) {
        const json& value = document["claudeExclude"];
        std::vector<std::string> entries;
        bool valid = false;
        if (value.is_string()) {
            valid = true;
            std::string entry = value.get<std::string>();
            if (!trim(entry).empty()) {
                entries.push_back(std::move(entry));
            }
        } else if (value.is_array()) {
            valid = true;
            for (const json& item : value) {
                if (!item.is_string()) {
                    valid = false;
                    entries.clear();
                    break;
                }
                std::string entry = item.get<std::string>();
                if (!trim(entry).empty()) {
                    entries.push_back(std::move(entry));
                }
            }
        }
        if (valid) {
            // An empty list is meaningful: exclude nothing.
            settings.claudeExclude = std::move(entries);
        }
    }
    if (document.contains("launchCommand")) {
        updateLaunchCommand(document["launchCommand"],
                            settings.launchCommand);
    }
    if (document.contains("claudeConfigDir") &&
        document["claudeConfigDir"].is_string()) {
        std::string value =
            document["claudeConfigDir"].get<std::string>();
        settings.claudeConfigDir =
            trim(value).empty() ? defaultSettings_.claudeConfigDir
                                : std::move(value);
    }
    if (document.contains("tempDir") && document["tempDir"].is_string()) {
        std::string value = document["tempDir"].get<std::string>();
        settings.tempDir = trim(value).empty()
                               ? defaultSettings_.tempDir
                               : std::move(value);
    }
    if (document.contains("logFile") &&
        document["logFile"].is_string()) {
        settings.logFile = document["logFile"].get<std::string>();
    }
    if (document.contains("tabOnlySectionSwitch") &&
        document["tabOnlySectionSwitch"].is_boolean()) {
        settings.tabOnlySectionSwitch =
            document["tabOnlySectionSwitch"].get<bool>();
    }
    if (document.contains("defaultProjectName") &&
        document["defaultProjectName"].is_string()) {
        // An empty value is meaningful: do not prefill the name field.
        settings.defaultProjectName =
            document["defaultProjectName"].get<std::string>();
    }
    if (document.contains("defaultFocus")) {
        const json& value = document["defaultFocus"];
        if (value.is_object() || value.is_array()) {
            settings.defaultFocusTargets = defaultFocusListFromJson(value);
        }
    }
}

fs::path ConfigManager::resolveThemePath() const {
    const std::string value = trim(settings.themeFile);
    if (value.empty()) {
        return {};
    }
    return pathResolver_
        .resolve(value, PathKind::File, MultiMatchPolicy::FirstSorted)
        .path;
}

void ConfigManager::setThemeStatus(std::string message, bool error) {
    themeStatus_ = std::move(message);
    themeStatusIsError_ = error;
}

void ConfigManager::loadTheme() {
    themeStatus_.clear();
    themeStatusIsError_ = false;
    themeLoadState_ = ThemeLoadState::Loaded;
    themeNeedsWrite_ = false;

    themeFile_ = resolveThemePath();
    const std::string themePathText = pathToUtf8(themeFile_);
    if (themeFile_.empty()) {
        themeLoadState_ = ThemeLoadState::Unreadable;
        themeDocument_ = defaultThemeDocument_;
        menuTheme = defaultMenuTheme_;
        setThemeStatus(
            messageText(menuTheme, "themePathEmpty"), true);
        log("ERROR", "theme: " + themeStatus_);
        return;
    }

    json theme = defaultThemeDocument_;
    bool exists = false;
    std::error_code ec;
    exists = fs::exists(themeFile_, ec);
    if (ec) {
        themeLoadState_ = ThemeLoadState::Unreadable;
        setThemeStatus(
            messageText(defaultMenuTheme_, "themeInspectFailed",
                        {{"path", themePathText},
                         {"error", ec.message()}}),
            true);
    } else if (exists) {
        std::ifstream input(themeFile_, std::ios::binary);
        if (!input) {
            themeLoadState_ = ThemeLoadState::Unreadable;
            setThemeStatus(
                messageText(defaultMenuTheme_, "themeOpenFailed",
                            {{"path", themePathText}}),
                true);
        } else {
            std::string contents(
                (std::istreambuf_iterator<char>(input)),
                std::istreambuf_iterator<char>());
            if (contents.size() >= 3 &&
                static_cast<unsigned char>(contents[0]) == 0xEF &&
                static_cast<unsigned char>(contents[1]) == 0xBB &&
                static_cast<unsigned char>(contents[2]) == 0xBF) {
                contents.erase(0, 3);
            }
            try {
                json document = json::parse(contents);
                if (!document.is_object()) {
                    themeLoadState_ = ThemeLoadState::Invalid;
                    setThemeStatus(
                        messageText(defaultMenuTheme_,
                                    "themeUnsupportedFormat"),
                        true);
                } else {
                    theme = std::move(document);
                }
            } catch (const std::exception& exception) {
                themeLoadState_ = ThemeLoadState::Invalid;
                setThemeStatus(
                    messageText(defaultMenuTheme_, "themeParseFailed",
                                {{"path", themePathText},
                                 {"error", exception.what()}}),
                    true);
            }
        }
    }

    json merged = mergeThemeDefaults(defaultThemeDocument_, theme);
    if (merged != theme) {
        themeNeedsWrite_ = true;
    }

    themeDocument_ = merged;
    menuTheme = menuThemeFromDocument(merged, &defaultMenuTheme_);
    for (const std::string& warning : menuTheme.warnings) {
        log("WARN", "theme: " + warning);
    }

    if (!exists && themeLoadState_ == ThemeLoadState::Loaded) {
        themeLoadState_ = ThemeLoadState::Missing;
    }
    switch (themeLoadState_) {
        case ThemeLoadState::Loaded:
            break;
        case ThemeLoadState::Missing:
        case ThemeLoadState::Invalid:
            // Invalid JSON is recoverable: back it up to .invalid and write
            // the merged/default theme. A missing file is simply created.
            themeNeedsWrite_ = true;
            break;
        case ThemeLoadState::Unreadable:
            // Read/inspect failures are not recoverable: leave the existing
            // file untouched because it cannot be safely preserved.
            themeNeedsWrite_ = false;
            break;
    }

    if (themeStatus_.empty()) {
        log("INFO", "theme=" + themePathText);
    } else {
        log("ERROR", "theme: " + themeStatus_);
    }

    if (themeLoadState_ != ThemeLoadState::Unreadable) {
        std::string themeError;
        if (!writeThemeIfNeeded(themeError)) {
            log("ERROR", "theme: " + themeError);
        }
    }
}

bool ConfigManager::writeThemeIfNeeded(std::string& error) {
    error.clear();
    if (themeFile_.empty()) {
        error = messageText(menuTheme, "themePathEmpty");
        setThemeStatus(error, true);
        return false;
    }
    if (themeLoadState_ == ThemeLoadState::Unreadable) {
        // The existing file could not be read; leave it untouched.
        return true;
    }
    if (themeLoadState_ == ThemeLoadState::Loaded && !themeNeedsWrite_) {
        return true;
    }

    const bool invalidContent = themeLoadState_ == ThemeLoadState::Invalid;
    std::error_code ec;
    if (!themeFile_.parent_path().empty()) {
        fs::create_directories(themeFile_.parent_path(), ec);
        if (ec) {
            error = messageText(
                menuTheme, "themeFolderCreateFailed",
                {{"path", pathToUtf8(themeFile_.parent_path())},
                 {"error", ec.message()}});
            setThemeStatus(error, true);
            return false;
        }
    }

    const bool existed = fs::exists(themeFile_, ec) && !ec;
    if (ec) {
        error = messageText(
            menuTheme, "themeInspectFailed",
            {{"path", pathToUtf8(themeFile_)},
             {"error", ec.message()}});
        setThemeStatus(error, true);
        return false;
    }
    fs::path backup = themeFile_;
    backup += invalidContent ? ".invalid" : ".before-fix";
    if (existed) {
        fs::copy_file(themeFile_, backup,
                      fs::copy_options::overwrite_existing, ec);
        if (ec) {
            error = messageText(
                menuTheme,
                invalidContent ? "themeInvalidBackupFailed"
                               : "themeBackupFailed",
                {{"backup", pathToUtf8(backup)},
                 {"error", ec.message()}});
            setThemeStatus(error, true);
            return false;
        }
    }

    fs::path temporary = themeFile_;
    temporary += ".tmp";
    std::string serialized;
    try {
        serialized = themeDocument_.dump(2) + "\n";
    } catch (const std::exception& exception) {
        error = messageText(menuTheme, "themeSerializeFailed",
                            {{"error", exception.what()}});
        setThemeStatus(error, true);
        return false;
    }

    {
        std::ofstream output(temporary,
                             std::ios::binary | std::ios::trunc);
        if (!output) {
            error = messageText(menuTheme, "themeWriteFailed",
                                {{"path", pathToUtf8(temporary)}});
            setThemeStatus(error, true);
            return false;
        }
        output.write(serialized.data(),
                     static_cast<std::streamsize>(serialized.size()));
        output.flush();
        if (!output) {
            output.close();
            fs::remove(temporary, ec);
            error = messageText(menuTheme, "themeFinishWriteFailed",
                                {{"path", pathToUtf8(temporary)}});
            setThemeStatus(error, true);
            return false;
        }
    }

    if (!replaceFile(temporary, themeFile_, error,
                     "themeReplaceFailed")) {
        fs::remove(temporary, ec);
        setThemeStatus(error, true);
        return false;
    }

    if (existed) {
        setThemeStatus(
            messageText(menuTheme, "themeRepaired",
                        {{"backup", pathToUtf8(backup)}}),
            false);
    } else {
        setThemeStatus(
            messageText(menuTheme, "themeCreated",
                        {{"path", pathToUtf8(themeFile_)}}),
            false);
    }
    themeNeedsWrite_ = false;
    themeLoadState_ = ThemeLoadState::Loaded;
    log("INFO", "theme written to " + pathToUtf8(themeFile_));
    return true;
}

void ConfigManager::loadProjects(const json& array) {
    std::vector<Project> loaded;
    for (const json& item : array) {
        if (!item.is_object() || !item.contains("path") ||
            !item["path"].is_string()) {
            continue;
        }

        Project project;
        project.path = item["path"].get<std::string>();
        if (item.contains("lastUsed") && item["lastUsed"].is_string()) {
            project.lastUsed = item["lastUsed"].get<std::string>();
        }
        if (item.contains("name") && item["name"].is_string()) {
            project.name = item["name"].get<std::string>();
        }
        project.path = trim(std::move(project.path));
        project.name = trim(std::move(project.name));
        if (project.path.empty()) {
            continue;
        }
        // Repeating a path is the user's choice; entries are kept in order.
        loaded.push_back(std::move(project));
    }
    projects = std::move(loaded);
}

void ConfigManager::computeConfigIssues() {
    configIssues_.clear();
    if (userDocument_.is_object()) {
        collectConfigIssues(defaultDocument_, userDocument_, "",
                            configIssues_);
    }
}

bool ConfigManager::writeDocument(const json& document, std::string& error) {
    error.clear();

    std::error_code ec;
    if (!configFile_.parent_path().empty()) {
        fs::create_directories(configFile_.parent_path(), ec);
        if (ec) {
            error = messageText(
                menuTheme, "configFolderCreateFailed",
                {{"path", pathToUtf8(configFile_.parent_path())},
                 {"error", ec.message()}});
            return false;
        }
    }

    if (!backupBrokenConfig(error)) {
        return false;
    }

    fs::path temporary = configFile_;
    temporary += ".tmp";

    std::string serialized;
    try {
        serialized = document.dump(2) + "\n";
    } catch (const std::exception& exception) {
        error = messageText(menuTheme, "configSerializeFailed",
                            {{"error", exception.what()}});
        return false;
    }

    {
        std::ofstream output(temporary,
                             std::ios::binary | std::ios::trunc);
        if (!output) {
            error = messageText(menuTheme, "configWriteFailed",
                                {{"path", pathToUtf8(temporary)}});
            return false;
        }
        output.write(serialized.data(),
                     static_cast<std::streamsize>(serialized.size()));
        output.flush();
        if (!output) {
            output.close();
            fs::remove(temporary, ec);
            error = messageText(menuTheme, "configFinishWriteFailed",
                                {{"path", pathToUtf8(temporary)}});
            return false;
        }
    }

    if (!replaceFile(temporary, configFile_, error)) {
        fs::remove(temporary, ec);
        return false;
    }

    fileWasMissing_ = false;
    loadHadError_ = false;
    loadWarning_.clear();
    canRepair_ = true;
    return true;
}

bool ConfigManager::backupBrokenConfig(std::string& error) {
    if (!loadHadError_ || backupAttempted_) {
        return true;
    }

    std::error_code ec;
    if (!fs::exists(configFile_, ec) || ec) {
        return true;
    }

    fs::path backup = configFile_;
    backup += ".invalid";
    fs::copy_file(configFile_, backup, fs::copy_options::overwrite_existing,
                  ec);
    if (ec) {
        error = messageText(
            menuTheme, "configInvalidBackupFailed",
            {{"backup", pathToUtf8(backup)},
             {"error", ec.message()}});
        return false;
    }
    backupAttempted_ = true;
    return true;
}

bool ConfigManager::replaceFile(const fs::path& from, const fs::path& to,
                 std::string& error,
                 const char* messageKey) {
#ifdef _WIN32
    if (!MoveFileExW(from.c_str(), to.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        error = messageText(menuTheme, messageKey,
                            {{"path", pathToUtf8(to)},
                             {"error", winErrorMessage(GetLastError())}});
        return false;
    }
    return true;
#else
    if (std::rename(from.c_str(), to.c_str()) != 0) {
        error = messageText(menuTheme, messageKey,
                            {{"path", pathToUtf8(to)},
                             {"error", std::strerror(errno)}});
        return false;
    }
    return true;
#endif
}

}  // namespace claudex
