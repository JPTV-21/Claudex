#include "claude_finder.hpp"

#include "config_manager.hpp"
#include "text_util.hpp"

#include <algorithm>
#include <fstream>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#endif

namespace claudex {

bool isRegularFile(const fs::path& path) {
    std::error_code ec;
    return fs::is_regular_file(path, ec) && !ec;
}

bool isExecutableFile(const fs::path& path) {
    if (!isRegularFile(path)) {
        return false;
    }
#ifndef _WIN32
    return ::access(path.c_str(), X_OK) == 0;
#else
    return true;
#endif
}

bool isBatchFile(const fs::path& path) {
#ifdef _WIN32
    const std::string extension = lowerAscii(path.extension().string());
    return extension == ".cmd" || extension == ".bat";
#else
    (void)path;
    return false;
#endif
}

std::vector<fs::path> executableNameVariants(const fs::path& name) {
    if (name.has_extension()) {
        return {name};
    }
    // Append with filesystem::path so non-ASCII characters in the native
    // (wide) path are preserved.
    std::vector<fs::path> variants;
    for (const std::string& suffix : executableSuffixes()) {
        fs::path candidate = name;
        candidate += suffix;
        variants.push_back(std::move(candidate));
    }
    variants.push_back(name);
    return variants;
}

fs::path canonicalPath(const fs::path& path) {
    std::error_code ec;
    fs::path canonical = fs::weakly_canonical(path, ec);
    return ec ? path : canonical;
}

// Used while scanning a directory. Unlike isExecutableFile, this rejects files
// that the platform would not run directly (for example a .txt on Windows).
bool isRunnableCandidate(const fs::path& path) {
    std::error_code ec;
    if (!fs::is_regular_file(path, ec) || ec) {
        return false;
    }
#ifndef _WIN32
    return ::access(path.c_str(), X_OK) == 0;
#else
    const std::string extension = lowerAscii(pathToUtf8(path.extension()));
    if (!extension.empty()) {
        const std::vector<std::string>& suffixes = executableSuffixes();
        return std::find(suffixes.begin(), suffixes.end(), extension) !=
               suffixes.end();
    }
    // Extensionless executable images on Windows are PE files.
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return false;
    }
    char header[2] = {};
    input.read(header, 2);
    return input.gcount() == 2 && header[0] == 'M' && header[1] == 'Z';
#endif
}

// Whether a wildcard match is worth reporting as "not runnable". On Windows a
// pattern such as "claude*" also matches claudex-config.json and claudex.log;
// those should fall through to "not found" instead of naming a config file.
bool hasExecutableName(const fs::path& path) {
#ifdef _WIN32
    const std::string extension = lowerAscii(pathToUtf8(path.extension()));
    if (extension.empty()) {
        return true;
    }
    const std::vector<std::string>& suffixes = executableSuffixes();
    return std::find(suffixes.begin(), suffixes.end(), extension) !=
           suffixes.end();
#else
    (void)path;
    return true;
#endif
}

namespace {

bool equalsIgnoreCase(std::string_view lhs, std::string_view rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    for (std::size_t i = 0; i < lhs.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(lhs[i])) !=
            std::tolower(static_cast<unsigned char>(rhs[i]))) {
            return false;
        }
    }
    return true;
}

bool filenamesEqual(std::string_view lhs, std::string_view rhs,
                    bool caseInsensitive) {
    return caseInsensitive ? equalsIgnoreCase(lhs, rhs) : lhs == rhs;
}

}  // namespace

bool filenameMatchesName(const fs::path& file, const std::string& name,
                         bool caseInsensitive) {
    if (name.empty()) {
        return true;
    }
    const std::string filename = pathToUtf8(file.filename());
    if (containsWildcard(name)) {
        return wildcardMatch(name, filename, caseInsensitive);
    }
    if (filenamesEqual(filename, name, caseInsensitive)) {
        return true;
    }
    const std::string stem = pathToUtf8(file.stem());
    return filenamesEqual(stem, name, caseInsensitive);
}

ClaudeExcludeMatcher::ClaudeExcludeMatcher(std::vector<std::string> patterns,
                     const PathResolver& resolver)
    : resolver_(resolver) {
    for (std::string pattern : patterns) {
        pattern = stripMatchingQuotes(trim(std::move(pattern)));
        if (pattern.empty()) {
            continue;
        }

        // Expand first: a pattern such as "{root}" or "~" is a full path
        // even though the raw text has no separator. Name patterns keep
        // their raw text, so environment-variable syntax in a file name
        // is not expanded accidentally.
        const std::string expanded = resolver_.expand(pattern);
        bool pathPattern = pattern.find('/') != std::string::npos ||
                           expanded.find('/') != std::string::npos;
#ifdef _WIN32
        pathPattern = pathPattern ||
                      pattern.find('\\') != std::string::npos ||
                      expanded.find('\\') != std::string::npos;
#endif
        fs::path path = pathFromUtf8(expanded);
        pathPattern = pathPattern || path.is_absolute() ||
                      path.has_root_name() || path.has_root_directory();
        if (!pathPattern) {
            namePatterns_.push_back(std::move(pattern));
            continue;
        }

        if (path.is_relative()) {
            path = resolver_.rootDir() / path;
        }
        path = path.lexically_normal();
        PathPattern entry;
        entry.path = std::move(path);
        entry.text = genericPathToUtf8(entry.path);
        entry.wildcard = containsWildcard(entry.text);
        pathPatterns_.push_back(std::move(entry));
    }
}

bool ClaudeExcludeMatcher::matches(const fs::path& candidate) const {
    if (candidate.empty()) {
        return false;
    }
#ifdef _WIN32
    constexpr bool caseInsensitive = true;
#else
    constexpr bool caseInsensitive = false;
#endif
    for (const std::string& pattern : namePatterns_) {
        if (filenameMatchesName(candidate, pattern, caseInsensitive)) {
            return true;
        }
    }
    if (pathPatterns_.empty()) {
        return false;
    }

    const fs::path canonical = canonicalPath(candidate);
    const std::string generic = genericPathToUtf8(canonical);
    for (const PathPattern& pattern : pathPatterns_) {
        if (pattern.wildcard) {
            if (wildcardMatch(pattern.text, generic, caseInsensitive)) {
                return true;
            }
        } else if (pathsEqual(canonical, pattern.path)) {
            return true;
        }
    }
    return false;
}

std::vector<fs::path> pathDirectories() {
    std::vector<fs::path> directories;
    auto pathValue = getEnvUtf8("PATH");
    if (!pathValue || pathValue->empty()) {
        return directories;
    }

#ifdef _WIN32
    constexpr char separator = ';';
#else
    constexpr char separator = ':';
#endif

    std::size_t start = 0;
    while (start <= pathValue->size()) {
        std::size_t end = pathValue->find(separator, start);
        if (end == std::string::npos) {
            end = pathValue->size();
        }
        std::string entry = pathValue->substr(start, end - start);
        entry = stripMatchingQuotes(trim(std::move(entry)));

        fs::path directory;
        if (entry.empty()) {
            std::error_code ec;
            directory = fs::current_path(ec);
            if (ec) {
                directory = fs::path(".");
            }
        } else {
            directory = pathFromUtf8(entry);
            if (directory.is_relative()) {
                std::error_code ec;
                fs::path absolute = fs::absolute(directory, ec);
                if (!ec) {
                    directory = std::move(absolute);
                }
            }
        }
        directories.push_back(std::move(directory));

        if (end == pathValue->size()) {
            break;
        }
        start = end + 1;
    }
    return directories;
}

std::optional<fs::path> findExecutableInDirectory(const fs::path& directory,
                                                  const fs::path& name) {
    for (const fs::path& candidate : executableNameVariants(name)) {
        fs::path full = candidate.is_absolute() ? candidate
                                                : directory / candidate;
        if (isExecutableFile(full)) {
            return canonicalPath(full);
        }
    }
    return std::nullopt;
}

std::optional<fs::path> findExecutableOnPath(const std::string& name) {
    if (name.empty()) {
        return std::nullopt;
    }
    for (const fs::path& directory : pathDirectories()) {
        if (auto found =
                findExecutableInDirectory(directory, pathFromUtf8(name))) {
            return found;
        }
    }
    return std::nullopt;
}

namespace {

int executableRank(const fs::path& path) {
#ifdef _WIN32
    const std::string extension = lowerAscii(pathToUtf8(path.extension()));
    const std::vector<std::string>& suffixes = executableSuffixes();
    for (std::size_t i = 0; i < suffixes.size(); ++i) {
        if (suffixes[i] == extension) {
            return static_cast<int>(i);
        }
    }
    return static_cast<int>(suffixes.size());
#else
    (void)path;
    return 0;
#endif
}

}  // namespace

void sortExecutableCandidates(std::vector<fs::path>& paths) {
    std::sort(paths.begin(), paths.end(),
              [](const fs::path& lhs, const fs::path& rhs) {
                  const int leftRank = executableRank(lhs);
                  const int rightRank = executableRank(rhs);
                  if (leftRank != rightRank) {
                      return leftRank < rightRank;
                  }
                  return pathSortKey(lhs) < pathSortKey(rhs);
              });
}

ClaudeDirectorySearch findClaudeInDirectory(
    const fs::path& directory, const std::string& exeName,
    bool allowSingleFallback, const ClaudeExcludeMatcher& exclude) {
    ClaudeDirectorySearch result;
    result.directory = directory;

    std::error_code ec;
    if (!fs::is_directory(directory, ec) || ec) {
        return result;
    }

#ifdef _WIN32
    constexpr bool caseInsensitive = true;
#else
    constexpr bool caseInsensitive = false;
#endif

    const bool wildcardName = containsWildcard(exeName);

    // Fast path: an exact name, or a simple "prefix*" pattern, can be tried
    // without enumerating the whole directory. This keeps PATH lookups cheap.
    std::string prefix;
    if (!wildcardName) {
        prefix = exeName;
    } else {
        const std::size_t wildcard = exeName.find_first_of("*?");
        if (wildcard == exeName.size() - 1 && exeName[wildcard] == '*') {
            prefix = exeName.substr(0, wildcard);
        }
    }

    if (!prefix.empty() && !containsWildcard(prefix)) {
        std::vector<fs::path> direct;
        for (const fs::path& variant :
             executableNameVariants(pathFromUtf8(prefix))) {
            fs::path full =
                variant.is_absolute() ? variant : directory / variant;
            if (!isExecutableFile(full)) {
                continue;
            }
            const fs::path canonical = canonicalPath(full);
            if (!exclude.matches(canonical)) {
                direct.push_back(canonical);
            }
        }
        if (!direct.empty()) {
            sortExecutableCandidates(direct);
            result.rankedMatches = std::move(direct);
            result.selected = result.rankedMatches.front();
            return result;
        }
        if (!wildcardName && !allowSingleFallback) {
            return result;
        }
    }

    std::vector<fs::path> matches;
    std::vector<fs::path> candidates;
    fs::directory_iterator iterator(directory, ec);
    if (ec) {
        return result;
    }
    const fs::directory_iterator end;
    for (; iterator != end; iterator.increment(ec)) {
        if (ec) {
            ec.clear();
            continue;
        }
        const fs::path candidate = iterator->path();
        if (!isRunnableCandidate(candidate)) {
            continue;
        }
        const fs::path canonical = canonicalPath(candidate);
        if (exclude.matches(canonical)) {
            continue;
        }
        candidates.push_back(canonical);
        if (!exeName.empty() &&
            !filenameMatchesName(candidate, exeName, caseInsensitive)) {
            continue;
        }
        matches.push_back(canonical);
    }

    result.candidates = std::move(candidates);
    if (!matches.empty()) {
        sortExecutableCandidates(matches);
        result.rankedMatches = std::move(matches);
        result.selected = result.rankedMatches.front();
        return result;
    }

    if (allowSingleFallback) {
        if (result.candidates.size() == 1) {
            result.selected = result.candidates.front();
            result.usedSingleFallback = true;
            return result;
        }
        if (result.candidates.size() > 1) {
            result.ambiguous = true;
        }
    }
    return result;
}

ClaudeDirectorySearch findClaudeOnPath(
    const std::string& exeName, const ClaudeExcludeMatcher& exclude) {
    ClaudeDirectorySearch result;
    for (const fs::path& directory : pathDirectories()) {
        ClaudeDirectorySearch found =
            findClaudeInDirectory(directory, exeName, false, exclude);
        if (!found.selected.empty()) {
            return found;
        }
    }
    return result;
}

std::optional<fs::path> ConfigManager::findClaudeExecutable(
    std::string& error) const {
    error.clear();
    const ClaudeExcludeMatcher exclude(settings.claudeExclude,
                                       pathResolver_);
    std::string notRunnablePath;
    std::string missingPath;
    ClaudeDirectorySearch ambiguity;

    auto accept = [&](const ClaudeDirectorySearch& found)
        -> std::optional<fs::path> {
        if (found.selected.empty()) {
            if (found.ambiguous) {
                ambiguity = found;
            }
            return std::nullopt;
        }
        if (found.rankedMatches.size() > 1) {
            log("WARN", "claude: multiple candidates in " +
                            pathToUtf8(found.directory) + "; using " +
                            pathToUtf8(found.selected));
        }
        if (found.usedSingleFallback) {
            log("INFO", "claude: only executable in " +
                            pathToUtf8(found.directory) + " is " +
                            pathToUtf8(found.selected));
        }
        return found.selected;
    };

    // A single unrelated executable in the launcher folder is often not
    // Claude (uninstallers, helpers), so only explicit subfolders get the
    // single-candidate fallback.
    auto allowFallbackFor = [&](const fs::path& directory) {
        return claudePathConfigured_ && !pathsEqual(directory, rootDir_);
    };

    for (const std::string& entry : settings.claudePath) {
        if (trim(entry).empty()) {
            continue;
        }
        const std::string value =
            pathResolver_.expand(stripMatchingQuotes(trim(entry)));
        if (value.empty()) {
            continue;
        }

        fs::path requested = pathFromUtf8(value);
        bool bare =
            !requested.has_parent_path() && !requested.is_absolute();
        if (bare) {
            // A single-segment value is a bare executable name unless it
            // names an existing file or folder next to the launcher.
            std::error_code localError;
            const fs::path local = rootDir_ / requested;
            if (fs::exists(local, localError) && !localError) {
                bare = false;
            }
        }

        if (bare) {
            // A bare entry is the executable name or pattern itself.
            ClaudeDirectorySearch rootSearch =
                findClaudeInDirectory(rootDir_, value, false, exclude);
            if (auto selected = accept(rootSearch)) {
                log("INFO", "claude: \"" + entry + "\" -> " +
                                pathToUtf8(*selected));
                return selected;
            }
            ClaudeDirectorySearch pathSearch =
                findClaudeOnPath(value, exclude);
            if (auto selected = accept(pathSearch)) {
                log("INFO", "claude: \"" + entry + "\" on PATH -> " +
                                pathToUtf8(*selected));
                return selected;
            }
            if (missingPath.empty()) {
                missingPath = value;
            }
            continue;
        }

        PathResolution resolution =
            pathResolver_.resolve(value, PathKind::Any,
                                  MultiMatchPolicy::FirstSorted);
        if (!resolution.resolved) {
            if (missingPath.empty()) {
                missingPath = resolution.expanded;
            }
            continue;
        }

        if (!resolution.wildcard) {
            std::error_code ec;
            if (fs::is_directory(resolution.path, ec) && !ec) {
                if (exclude.matches(resolution.path)) {
                    log("INFO", "claude: excluded by claudeExclude: " +
                                    pathToUtf8(resolution.path));
                    if (missingPath.empty()) {
                        missingPath = resolution.expanded;
                    }
                    continue;
                }
                if (auto selected = accept(findClaudeInDirectory(
                        resolution.path, kDefaultClaudePattern,
                        allowFallbackFor(resolution.path), exclude))) {
                    log("INFO", "claude: " + pathToUtf8(*selected));
                    return selected;
                }
                continue;
            }
            if (isRegularFile(resolution.path)) {
                if (exclude.matches(resolution.path)) {
                    log("INFO", "claude: excluded by claudeExclude: " +
                                    pathToUtf8(resolution.path));
                    if (missingPath.empty()) {
                        missingPath = resolution.expanded;
                    }
                    continue;
                }
                if (isRunnableCandidate(resolution.path)) {
                    log("INFO", "claude: " + pathToUtf8(resolution.path));
                    return canonicalPath(resolution.path);
                }
                if (notRunnablePath.empty()) {
                    notRunnablePath = pathToUtf8(resolution.path);
                }
                continue;
            }
            if (missingPath.empty()) {
                missingPath = resolution.expanded;
            }
            continue;
        }

        // A wildcard can match files and directories. Files win, then each
        // matched directory is searched in sorted order.
        std::vector<fs::path> files;
        std::vector<fs::path> directories;
        for (const fs::path& match : resolution.matches) {
            std::error_code matchError;
            if (fs::is_directory(match, matchError) && !matchError) {
                directories.push_back(match);
            } else if (isRegularFile(match)) {
                files.push_back(match);
            }
        }

        std::vector<fs::path> runnable;
        for (const fs::path& file : files) {
            if (!isRunnableCandidate(file)) {
                continue;
            }
            if (exclude.matches(file)) {
                log("INFO", "claude: excluded by claudeExclude: " +
                                pathToUtf8(file));
                continue;
            }
            runnable.push_back(canonicalPath(file));
        }
        if (!runnable.empty()) {
            sortExecutableCandidates(runnable);
            log("INFO", "claude: " + pathToUtf8(runnable.front()));
            return runnable.front();
        }
        for (const fs::path& directory : directories) {
            if (exclude.matches(directory)) {
                log("INFO", "claude: excluded by claudeExclude: " +
                                pathToUtf8(directory));
                continue;
            }
            if (auto selected = accept(findClaudeInDirectory(
                    directory, kDefaultClaudePattern,
                    allowFallbackFor(directory), exclude))) {
                log("INFO", "claude: " + pathToUtf8(*selected));
                return selected;
            }
        }
        if (!files.empty() && notRunnablePath.empty()) {
            for (const fs::path& file : files) {
                if (hasExecutableName(file) && !exclude.matches(file)) {
                    notRunnablePath = pathToUtf8(file);
                    break;
                }
            }
        }
        if (missingPath.empty()) {
            missingPath = resolution.expanded;
        }
    }

    if (!notRunnablePath.empty()) {
        error = messageText(menuTheme, "claudeNotRunnable",
                            {{"path", notRunnablePath}});
        return std::nullopt;
    }
    if (ambiguity.ambiguous && !ambiguity.candidates.empty()) {
        std::string candidates;
        for (const fs::path& candidate : ambiguity.candidates) {
            if (!candidates.empty()) {
                candidates += ", ";
            }
            candidates += pathToUtf8(candidate.filename());
        }
        error = messageText(menuTheme, "claudeAmbiguous",
                            {{"path", pathToUtf8(ambiguity.directory)},
                             {"candidates", candidates}});
        return std::nullopt;
    }
    if (claudePathConfigured_ && settings.claudePath.size() == 1 &&
        !missingPath.empty()) {
        error = messageText(menuTheme, "claudePathNotFound",
                            {{"path", missingPath}});
        return std::nullopt;
    }
    error = messageText(menuTheme, "claudeNotFound", {});
    return std::nullopt;
}

}  // namespace claudex
