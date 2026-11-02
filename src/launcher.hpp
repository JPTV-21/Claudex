#pragma once

#include "config_manager.hpp"

#include <string>

namespace claudex {

struct LaunchResult {
    bool started = false;
    int exitCode = -1;
    std::string error;
};

bool ensureDirectory(const fs::path& directory, std::string& error,
                     const MenuTheme& theme);
// Creates claudeConfigDir and tempDir and returns their resolved paths.
bool preparePortableDirectories(const ConfigManager& config,
                                fs::path& configDir, fs::path& tempDir,
                                std::string& error);
// Custom project actions keep the old behavior: the launcher exports the
// portable environment for them. Claude launches get it from launchCommand.
bool preparePortableEnvironment(const ConfigManager& config,
                                std::string& error);

#ifdef _WIN32
std::wstring quoteWindowsArgument(const std::wstring& argument);
#endif

LaunchResult runShellCommand(const std::string& command,
                             const fs::path& workingDirectory,
                             const MenuTheme& theme);

LaunchResult launchClaude(const ConfigManager& config,
                          const fs::path& projectDirectory);
// Runs a project action's custom shell command. Unlike Claude launches, custom
// actions still receive the portable environment from the launcher.
LaunchResult runCustomProjectCommand(const ConfigManager& config,
                                     const fs::path& projectDirectory,
                                     const std::string& command);

void clearScreen();

}  // namespace claudex
