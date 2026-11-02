#pragma once

#include "path_resolver.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace claudex {

bool isRegularFile(const fs::path& path);
bool isExecutableFile(const fs::path& path);
bool isBatchFile(const fs::path& path);
fs::path canonicalPath(const fs::path& path);

// "claude" -> claude.exe, claude.cmd, ... (PATHEXT on Windows), claude.
std::vector<fs::path> executableNameVariants(const fs::path& name);

// Used while scanning a directory. Unlike isExecutableFile, this rejects files
// that the platform would not run directly (for example a .txt on Windows).
bool isRunnableCandidate(const fs::path& path);
// Whether a wildcard match is worth reporting as "not runnable".
bool hasExecutableName(const fs::path& path);

// Matches a file name or stem against an exact name or a wildcard pattern.
bool filenameMatchesName(const fs::path& file, const std::string& name,
                         bool caseInsensitive);

// User-written exclusions for Claude executable detection. A pattern without
// a path separator matches the candidate file name (or stem); a pattern with a
// separator is expanded as a path and matched against the full path.
class ClaudeExcludeMatcher {
public:
    ClaudeExcludeMatcher(std::vector<std::string> patterns,
                         const PathResolver& resolver);

    bool matches(const fs::path& candidate) const;

private:
    struct PathPattern {
        fs::path path;
        std::string text;
        bool wildcard = false;
    };

    const PathResolver& resolver_;
    std::vector<std::string> namePatterns_;
    std::vector<PathPattern> pathPatterns_;
};

// Directories listed in PATH; an empty entry means the current directory.
std::vector<fs::path> pathDirectories();
std::optional<fs::path> findExecutableInDirectory(const fs::path& directory,
                                                  const fs::path& name);
std::optional<fs::path> findExecutableOnPath(const std::string& name);

struct ClaudeDirectorySearch {
    fs::path directory;
    fs::path selected;
    std::vector<fs::path> rankedMatches;
    std::vector<fs::path> candidates;
    bool usedSingleFallback = false;
    bool ambiguous = false;
};

void sortExecutableCandidates(std::vector<fs::path>& paths);

ClaudeDirectorySearch findClaudeInDirectory(const fs::path& directory,
                                            const std::string& exeName,
                                            bool allowSingleFallback,
                                            const ClaudeExcludeMatcher& exclude);
ClaudeDirectorySearch findClaudeOnPath(const std::string& exeName,
                                       const ClaudeExcludeMatcher& exclude);

}  // namespace claudex
