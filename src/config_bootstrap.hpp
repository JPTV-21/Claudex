#pragma once

#include "platform.hpp"
#include "path_resolver.hpp"

#include <string>

namespace claudex {

struct ConfigBootstrapResult {
    fs::path configFile;
    bool redirected = false;
    bool targetMissing = false;
    // Optional status message key from resources/default-theme.json.
    std::string messageKey;
    std::string messageConfig;
};

// Reads the "configFile" field from |initialConfigFile| and returns the file
// the launcher should actually load. This mirrors how "theme" selects the
// theme file: the user copies the config themselves, and the launcher only
// follows the path. Nothing is created, copied, or deleted here.
ConfigBootstrapResult bootstrapConfigPath(
    const fs::path& rootDir, const fs::path& initialConfigFile,
    const PathResolver& resolver);

}  // namespace claudex
