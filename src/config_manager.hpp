#pragma once

#include "config_types.hpp"
#include "path_resolver.hpp"

#include <optional>
#include <string>
#include <vector>

namespace claudex {

class ConfigManager {
public:
    ConfigManager(fs::path rootDir, fs::path configFile);

    void load();
    bool save(std::string& error);
    bool repair(std::string& error);

    fs::path rootDir() const { return rootDir_; }
    fs::path configFile() const { return configFile_; }

    std::string expandPath(std::string value) const {
        return pathResolver_.expand(std::move(value));
    }

    PathResolution resolvePath(const std::string& raw, PathKind kind,
                               MultiMatchPolicy policy) const {
        return pathResolver_.resolve(raw, kind, policy);
    }

    fs::path normalizeAbsolutePath(const std::string& raw) const {
        return pathResolver_.normalizeAbsolutePath(raw);
    }

    std::string storeProjectPath(const std::string& entered,
                                 const fs::path& absolutePath) const {
        return pathResolver_.storeProjectPath(entered, absolutePath);
    }

    std::string expandLaunchCommand(std::string command,
                                    const fs::path& configDir,
                                    const fs::path& tempDir) const {
        return pathResolver_.expandLaunchCommand(std::move(command), configDir,
                                                 tempDir);
    }

    const PathResolver& pathResolver() const { return pathResolver_; }
    fs::path themeFile() const { return themeFile_; }
    const std::string& themeStatus() const { return themeStatus_; }
    bool themeStatusIsError() const { return themeStatusIsError_; }
    bool loadHadError() const { return loadHadError_; }
    const std::string& loadWarning() const { return loadWarning_; }
    bool fileWasMissing() const { return fileWasMissing_; }
    bool canRepair() const { return canRepair_; }
    bool needsRepair() const;
    const std::vector<std::string>& configIssues() const {
        return configIssues_;
    }

    std::string claudePathText() const;
    // Command templates are expanded by the shell, so a list would be
    // ambiguous. Use the first entry there.
    std::string firstClaudePathText() const;

    fs::path resolveProjectPath(const Project& project) const {
        return pathResolver_.normalizeAbsolutePath(project.path);
    }
    fs::path resolveProjectPath(const std::string& path) const {
        return pathResolver_.normalizeAbsolutePath(path);
    }
    int findProject(const fs::path& absolutePath) const;

    fs::path claudeConfigDir() const;
    fs::path tempDir() const;
    fs::path logFilePath() const;
    void log(const std::string& level, const std::string& message) const;

    std::optional<fs::path> findClaudeExecutable(std::string& error) const;

    Settings settings;
    MenuTheme menuTheme;
    std::vector<Project> projects;

private:
    void loadSettings(const json& document);
    fs::path resolveThemePath() const;
    void setThemeStatus(std::string message, bool error);
    void loadTheme();
    bool writeThemeIfNeeded(std::string& error);
    void loadProjects(const json& array);
    void computeConfigIssues();
    bool writeDocument(const json& document, std::string& error);
    bool backupBrokenConfig(std::string& error);
    bool replaceFile(const fs::path& from, const fs::path& to,
                     std::string& error,
                     const char* messageKey = "configReplaceFailed");

    fs::path rootDir_;
    fs::path configFile_;
    PathResolver pathResolver_;
    json defaultDocument_;
    json defaultThemeDocument_;
    json themeDocument_;
    fs::path themeFile_;
    // How the last loadTheme() found the file. Only Loaded, Missing and
    // Invalid permit a write; Unreadable is left untouched so a transient
    // read failure never destroys an existing theme.
    enum class ThemeLoadState { Loaded, Missing, Invalid, Unreadable };
    ThemeLoadState themeLoadState_ = ThemeLoadState::Loaded;
    Settings defaultSettings_;
    MenuTheme defaultMenuTheme_;
    json userDocument_;
    std::vector<std::string> configIssues_;
    std::string loadWarning_;
    std::string themeStatus_;
    bool themeStatusIsError_ = false;
    bool themeNeedsWrite_ = false;
    bool loadHadError_ = false;
    bool fileWasMissing_ = false;
    bool backupAttempted_ = false;
    bool canRepair_ = false;
    bool claudePathConfigured_ = false;
};

}  // namespace claudex
