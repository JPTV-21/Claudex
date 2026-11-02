#include "path_resolver.hpp"

#include "text_util.hpp"

#include <algorithm>
#include <system_error>

namespace claudex {

std::string expandPathValue(std::string value, const fs::path& rootDir) {
    replaceAll(value, "{root}", pathToUtf8(rootDir));
    replaceAll(value, "{platform}", platformName());
    replaceAll(value, "{arch}", archName());
    replaceAll(value, "{home}", homeDirectory());
#ifdef _WIN32
    replaceAll(value, "{drive}", pathToUtf8(rootDir.root_name()));
#else
    replaceAll(value, "{drive}", volumeMountPoint(rootDir));
#endif
    value = expandEnvironmentVariables(std::move(value));

    if (!value.empty() && value.front() == '~' &&
        (value.size() == 1 || value[1] == '/' || value[1] == '\\')) {
        const std::string home = homeDirectory();
        if (!home.empty()) {
            value = home + value.substr(1);
        }
    }
    return value;
}

std::vector<fs::path> expandWildcardPath(const fs::path& pattern,
                                         PathKind kind) {
    const fs::path root = pattern.root_path();
    std::vector<fs::path> frontier;
    frontier.push_back(root.empty() ? fs::path(".") : root);

    std::vector<fs::path> components;
    for (const fs::path& component : pattern.relative_path()) {
        if (!component.empty()) {
            components.push_back(component);
        }
    }
    if (components.empty()) {
        return {};
    }

#ifdef _WIN32
    constexpr bool caseInsensitive = true;
#else
    constexpr bool caseInsensitive = false;
#endif

    for (std::size_t index = 0; index < components.size(); ++index) {
        const fs::path& component = components[index];
        const bool last = index + 1 == components.size();
        const std::string componentText = pathToUtf8(component);
        const bool wildcard = containsWildcard(componentText);

        std::vector<fs::path> next;
        for (const fs::path& directory : frontier) {
            if (!wildcard) {
                fs::path candidate = directory / component;
                std::error_code ec;
                if (last) {
                    next.push_back(std::move(candidate));
                } else if (fs::is_directory(candidate, ec) && !ec) {
                    next.push_back(std::move(candidate));
                }
                continue;
            }

            std::error_code ec;
            fs::directory_iterator iterator(directory, ec);
            if (ec) {
                continue;
            }
            const fs::directory_iterator end;
            for (; iterator != end; iterator.increment(ec)) {
                if (ec) {
                    ec.clear();
                    continue;
                }
                const fs::path name = iterator->path().filename();
                if (!wildcardMatch(componentText, pathToUtf8(name),
                                   caseInsensitive)) {
                    continue;
                }
                fs::path candidate = directory / name;
                if (last) {
                    next.push_back(std::move(candidate));
                } else if (fs::is_directory(candidate, ec) && !ec) {
                    next.push_back(std::move(candidate));
                }
            }
        }
        frontier = std::move(next);
        if (frontier.empty()) {
            return {};
        }
    }

    std::vector<fs::path> matches;
    for (fs::path& candidate : frontier) {
        std::error_code ec;
        bool matchesKind = false;
        if (kind == PathKind::Any) {
            matchesKind = fs::exists(candidate, ec) && !ec;
        } else if (kind == PathKind::File) {
            matchesKind = fs::is_regular_file(candidate, ec) && !ec;
        } else {
            matchesKind = fs::is_directory(candidate, ec) && !ec;
        }
        if (!matchesKind) {
            continue;
        }
        fs::path canonical = fs::weakly_canonical(candidate, ec);
        matches.push_back(ec ? candidate : std::move(canonical));
    }

    std::sort(matches.begin(), matches.end(),
              [](const fs::path& lhs, const fs::path& rhs) {
                  return pathSortKey(lhs) < pathSortKey(rhs);
              });
    matches.erase(std::unique(matches.begin(), matches.end(),
                              [](const fs::path& lhs, const fs::path& rhs) {
                                  return pathSortKey(lhs) == pathSortKey(rhs);
                              }),
                  matches.end());
    return matches;
}

PathResolution resolveExpandedPath(const std::string& value,
                                   const fs::path& root, PathKind kind,
                                   MultiMatchPolicy policy) {
    PathResolution result;
    result.expanded = value;
    if (value.empty()) {
        return result;
    }

    fs::path pattern = pathFromUtf8(value);
    if (pattern.is_relative()) {
        pattern = root / pattern;
    }
    pattern = pattern.lexically_normal();

    result.wildcard = containsWildcard(genericPathToUtf8(pattern));
    if (!result.wildcard) {
        std::error_code ec;
        fs::path canonical = fs::weakly_canonical(pattern, ec);
        result.path = ec ? std::move(pattern) : std::move(canonical);
        result.resolved = true;
        return result;
    }

    result.matches = expandWildcardPath(pattern, kind);
    if (result.matches.empty()) {
        result.noMatch = true;
        return result;
    }

    result.ambiguous = result.matches.size() > 1;
    if (result.ambiguous && policy == MultiMatchPolicy::RequireUnique) {
        return result;
    }
    result.path = result.matches.front();
    result.resolved = true;
    return result;
}

PathResolution resolvePathSpec(const std::string& raw, const fs::path& root,
                               PathKind kind, MultiMatchPolicy policy) {
    return resolveExpandedPath(
        expandPathValue(stripMatchingQuotes(raw), root), root, kind, policy);
}

bool pathContainsParentTraversal(const fs::path& path) {
    for (const fs::path& part : path) {
        if (part == "..") {
            return true;
        }
    }
    return false;
}

std::string makeStoredPath(const fs::path& absolutePath, const fs::path& rootDir) {
    std::error_code ec;
    fs::path canonical = fs::weakly_canonical(absolutePath, ec);
    if (ec) {
        canonical = absolutePath.lexically_normal();
    }
    fs::path canonicalRoot = fs::weakly_canonical(rootDir, ec);
    if (ec) {
        canonicalRoot = rootDir.lexically_normal();
    }

    fs::path relative = fs::relative(canonical, canonicalRoot, ec);
    if (!ec && !relative.empty() && !relative.is_absolute() &&
        !pathContainsParentTraversal(relative)) {
        return genericPathToUtf8(relative);
    }
    return genericPathToUtf8(canonical);
}

// Stores a project path without turning relative user input into an absolute
// path. The path is only resolved to validate it; relative input stays
// relative even when it points outside the launcher folder. Relative paths
// that resolve inside the launcher folder are shortened to a clean form.
// Templates, environment variables, and a leading ~ stay in the stored value
// so the project keeps working when the config moves to another drive or
// machine; they are expanded again on every resolve. Wildcards are still
// resolved to the one folder that was picked when the project was added.
std::string makeStoredProjectPath(const std::string& entered,
                                  const fs::path& absolutePath,
                                  const fs::path& rootDir) {
    const std::string inputText = stripMatchingQuotes(trim(entered));
    const std::string expanded = expandPathValue(inputText, rootDir);
    if (!containsWildcard(inputText) && expanded != inputText) {
        std::string stored = inputText;
#ifdef _WIN32
        std::replace(stored.begin(), stored.end(), '\\', '/');
#endif
        return stored;
    }

    const fs::path inputPath = pathFromUtf8(expanded);
    if (inputPath.empty() || !inputPath.is_relative() ||
        inputPath.has_root_name()) {
        return makeStoredPath(absolutePath, rootDir);
    }

    std::error_code ec;
    const fs::path canonical = fs::weakly_canonical(absolutePath, ec);
    const fs::path canonicalRoot = fs::weakly_canonical(rootDir, ec);
    if (!ec) {
        const fs::path relative = fs::relative(canonical, canonicalRoot, ec);
        if (!ec && !relative.empty() && !relative.is_absolute() &&
            !pathContainsParentTraversal(relative)) {
            return genericPathToUtf8(relative);
        }
    }

    return genericPathToUtf8(inputPath.lexically_normal());
}

namespace {

bool isCommandSeparator(char character) {
    return std::isspace(static_cast<unsigned char>(character)) != 0 ||
           character == '&' || character == '|' || character == ';' ||
           character == '<' || character == '>' || character == '(' ||
           character == ')';
}

std::string stripWordQuotes(const std::string& word) {
    if (word.size() >= 2 &&
        ((word.front() == '"' && word.back() == '"') ||
         (word.front() == '\'' && word.back() == '\''))) {
        return word.substr(1, word.size() - 2);
    }
    return word;
}

bool isAbsolutePathValue(const std::string& value) {
    const fs::path path = pathFromUtf8(value);
    return path.is_absolute() || path.has_root_name() ||
           path.has_root_directory() ||
           value.find("://") != std::string::npos;
}

bool isExplicitRelativePath(const std::string& value) {
    return value.rfind("./", 0) == 0 || value.rfind("../", 0) == 0 ||
           value.rfind(".\\", 0) == 0 || value.rfind("..\\", 0) == 0;
}

}  // namespace

PathResolution PathResolver::resolveWithBase(const std::string& raw,
                               const fs::path& base, PathKind kind,
                               MultiMatchPolicy policy) const {
    return resolveExpandedPath(
        expandPathValue(stripMatchingQuotes(raw), rootDir_), base, kind,
        policy);
}

// Expands launch-command placeholders only. Environment variables and a
// leading ~ are left to the shell.
std::string PathResolver::expandLaunchCommand(std::string command,
                                const fs::path& configDir,
                                const fs::path& tempDir) const {
    replaceAll(command, "{root}", pathToUtf8(rootDir_));
    replaceAll(command, "{platform}", platformName());
    replaceAll(command, "{arch}", archName());
    replaceAll(command, "{home}", homeDirectory());
#ifdef _WIN32
    replaceAll(command, "{drive}", pathToUtf8(rootDir_.root_name()));
#else
    replaceAll(command, "{drive}", volumeMountPoint(rootDir_));
#endif
    replaceAll(command, "{claudeConfigDir}", pathToUtf8(configDir));
    replaceAll(command, "{tempDir}", pathToUtf8(tempDir));
    return resolveLaunchCommandPaths(std::move(command));
}

// Resolves explicit ./ and ../ path words, plus a path-like first command
// word, against the launcher folder. Other arguments are left to the
// shell so flags, URLs, variables, and patterns keep their meaning.
std::string PathResolver::resolveLaunchCommandPaths(std::string command) const {
    std::string result;
    result.reserve(command.size());
    std::size_t index = 0;
    bool firstWord = true;
    while (index < command.size()) {
        const char character = command[index];
        if (isCommandSeparator(character)) {
            result += character;
            ++index;
            continue;
        }

        const std::size_t start = index;
        char quote = '\0';
        while (index < command.size()) {
            const char current = command[index];
            if (quote != '\0') {
                if (current == quote) {
                    quote = '\0';
                }
                ++index;
                continue;
            }
            if (current == '"' || current == '\'') {
                quote = current;
                ++index;
                continue;
            }
            if (isCommandSeparator(current)) {
                break;
            }
            ++index;
        }

        const std::string word = command.substr(start, index - start);
        const std::string value = stripWordQuotes(word);
        bool relative = isExplicitRelativePath(value);
        if (!relative && firstWord && !value.empty() &&
            value.find('=') == std::string::npos &&
            value.front() != '-' && value.front() != '%' &&
            value.front() != '$' && value.front() != '~' &&
            value.front() != '{' && !isAbsolutePathValue(value)) {
            if (value.find_first_of("/\\") != std::string::npos) {
                relative = true;
            } else {
                std::error_code ec;
                const fs::path candidate =
                    rootDir_ / pathFromUtf8(value);
                relative = fs::is_regular_file(candidate, ec) && !ec;
            }
        }
        if (relative) {
            const fs::path path = pathFromUtf8(value);
            if (path.is_relative()) {
                const fs::path resolved =
                    (rootDir_ / path).lexically_normal();
                result += quoteShellArgument(pathToUtf8(resolved));
            } else {
                result += word;
            }
        } else if (firstWord && !value.empty() &&
                   value.find('=') == std::string::npos &&
                   value.find("://") == std::string::npos &&
                   value.front() != '-' && value.front() != '%' &&
                   value.front() != '$' && value.front() != '~' &&
                   value.front() != '{' &&
                   (word.front() != '"' && word.front() != '\'') &&
                   isAbsolutePathValue(value) &&
                   value.find_first_of("/\\") != std::string::npos) {
            // An unquoted absolute first word, including one produced by
            // a {root}/{claudeConfigDir}/{tempDir} placeholder, may
            // contain spaces.
            result += quoteShellArgument(value);
        } else {
            result += word;
        }
        firstWord = false;
    }
    return result;
}

}  // namespace claudex
