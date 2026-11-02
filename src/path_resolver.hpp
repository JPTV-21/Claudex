#pragma once

#include "platform.hpp"

#include <string>
#include <utility>
#include <vector>

namespace claudex {

inline constexpr const char* kDefaultConfigFile = "claudex-config.json";
// A claudePath entry that names a directory is shorthand for looking for this
// pattern inside it. Explicit file names and globs always win.
inline constexpr const char* kDefaultClaudePattern = "claude*";

enum class PathKind { Any, File, Directory };
enum class MultiMatchPolicy { FirstSorted, RequireUnique };

struct PathResolution {
    bool resolved = false;
    bool wildcard = false;
    bool noMatch = false;
    bool ambiguous = false;
    std::string expanded;
    fs::path path;
    std::vector<fs::path> matches;
};

// Expands {root}, {platform}, {arch}, {home}, {drive}, environment variables
// and a leading ~.
std::string expandPathValue(std::string value, const fs::path& rootDir);

std::vector<fs::path> expandWildcardPath(const fs::path& pattern,
                                         PathKind kind);

PathResolution resolveExpandedPath(const std::string& value,
                                   const fs::path& root, PathKind kind,
                                   MultiMatchPolicy policy);
PathResolution resolvePathSpec(const std::string& raw, const fs::path& root,
                               PathKind kind, MultiMatchPolicy policy);

bool pathContainsParentTraversal(const fs::path& path);
std::string makeStoredPath(const fs::path& absolutePath,
                           const fs::path& rootDir);
std::string makeStoredProjectPath(const std::string& entered,
                                  const fs::path& absolutePath,
                                  const fs::path& rootDir);

// Centralizes path expansion and resolution for config values, project paths,
// Claude discovery, and launch commands.
class PathResolver {
public:
    explicit PathResolver(fs::path rootDir) : rootDir_(std::move(rootDir)) {}

    const fs::path& rootDir() const { return rootDir_; }

    std::string expand(std::string value) const {
        return expandPathValue(std::move(value), rootDir_);
    }

    PathResolution resolve(const std::string& raw, PathKind kind,
                           MultiMatchPolicy policy) const {
        return resolvePathSpec(raw, rootDir_, kind, policy);
    }

    // Expands placeholders against this resolver's root, but resolves a
    // relative path against `base`.
    PathResolution resolveWithBase(const std::string& raw, const fs::path& base,
                                   PathKind kind,
                                   MultiMatchPolicy policy) const;

    fs::path normalizeAbsolutePath(const std::string& raw) const {
        return resolve(raw, PathKind::Any, MultiMatchPolicy::FirstSorted).path;
    }

    std::string storeProjectPath(const std::string& entered,
                                 const fs::path& absolutePath) const {
        return makeStoredProjectPath(entered, absolutePath, rootDir_);
    }

    // Expands launch-command placeholders only. Environment variables and a
    // leading ~ are left to the shell.
    std::string expandLaunchCommand(std::string command,
                                    const fs::path& configDir,
                                    const fs::path& tempDir) const;

private:
    std::string resolveLaunchCommandPaths(std::string command) const;

    fs::path rootDir_;
};

}  // namespace claudex
