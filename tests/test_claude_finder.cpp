#include "test_framework.hpp"

#include "claude_finder.hpp"
#include "config_manager.hpp"
#include "scoped_env.hpp"
#include "text_util.hpp"

using namespace claudex;
using testfw::ScopedEnv;
using testfw::TempDir;
using testfw::writeFile;

namespace {

#ifdef _WIN32
constexpr bool kCaseInsensitive = true;
constexpr char kPathSeparator = ';';
#else
constexpr bool kCaseInsensitive = false;
constexpr char kPathSeparator = ':';
#endif

// File name that the platform treats as an executable for `stem`.
std::string exeName(const std::string& stem) {
#ifdef _WIN32
    return stem + ".exe";
#else
    return stem;
#endif
}

// Creates a runnable file. Windows decides by extension, POSIX by mode bits.
fs::path makeExecutable(const fs::path& directory, const std::string& stem) {
    const fs::path file = directory / exeName(stem);
    writeFile(file, "#!/bin/sh\nexit 0\n");
#ifndef _WIN32
    std::error_code ec;
    fs::permissions(file,
                    fs::perms::owner_all | fs::perms::group_read |
                        fs::perms::group_exec | fs::perms::others_read |
                        fs::perms::others_exec,
                    ec);
#endif
    return file;
}

// A data file that is never runnable on any platform.
fs::path makeDataFile(const fs::path& directory, const std::string& name) {
    const fs::path file = directory / name;
    writeFile(file, "data");
#ifndef _WIN32
    std::error_code ec;
    fs::permissions(file, fs::perms::owner_read | fs::perms::owner_write,
                    fs::perm_options::replace, ec);
#endif
    return file;
}

// A config rooted in a temporary folder with the given JSON body.
struct Fixture {
    TempDir root;
    ConfigManager config;

    explicit Fixture(const std::string& configJson = "{}")
        : config(root.path(), root.path() / "claudex-config.json") {
        writeFile(root.path() / "claudex-config.json", configJson);
        config.load();
    }
};

}  // namespace

// --- file classification -----------------------------------------------------

TEST_CASE("finder/isRegularFile and isExecutableFile") {
    TempDir temp;
    const fs::path exe = makeExecutable(temp.path(), "tool");
    const fs::path data = makeDataFile(temp.path(), "notes.txt");
    CHECK(isRegularFile(exe));
    CHECK(isRegularFile(data));
    CHECK_FALSE(isRegularFile(temp.path()));
    CHECK_FALSE(isRegularFile(temp.path() / "missing"));
    CHECK(isExecutableFile(exe));
    CHECK_FALSE(isExecutableFile(temp.path()));
    CHECK_FALSE(isExecutableFile(temp.path() / "missing"));
#ifndef _WIN32
    CHECK_FALSE(isExecutableFile(data));
#endif
}

TEST_CASE("finder/isRunnableCandidate rejects data files") {
    TempDir temp;
    CHECK(isRunnableCandidate(makeExecutable(temp.path(), "tool")));
    CHECK_FALSE(isRunnableCandidate(makeDataFile(temp.path(), "claudex.log")));
    CHECK_FALSE(isRunnableCandidate(makeDataFile(temp.path(), "x.json")));
    CHECK_FALSE(isRunnableCandidate(temp.path()));
    CHECK_FALSE(isRunnableCandidate(temp.path() / "missing"));
}

TEST_CASE("finder/hasExecutableName") {
    CHECK(hasExecutableName(fs::path("claude")));
#ifdef _WIN32
    CHECK(hasExecutableName(fs::path("claude.exe")));
    CHECK_FALSE(hasExecutableName(fs::path("claudex-config.json")));
    CHECK_FALSE(hasExecutableName(fs::path("claudex.log")));
#else
    CHECK(hasExecutableName(fs::path("claudex.log")));
#endif
}

TEST_CASE("finder/isBatchFile") {
#ifdef _WIN32
    CHECK(isBatchFile("run.cmd"));
    CHECK(isBatchFile("RUN.BAT"));
    CHECK_FALSE(isBatchFile("run.exe"));
#else
    CHECK_FALSE(isBatchFile("run.cmd"));
#endif
}

TEST_CASE("finder/canonicalPath resolves dot segments and tolerates missing paths") {
    TempDir temp;
    fs::create_directories(temp.path() / "a");
    CHECK(pathsEqual(canonicalPath(temp.path() / "a" / ".." / "a"),
                     temp.path() / "a"));
    const fs::path missing = temp.path() / "nope";
    CHECK(pathsEqual(canonicalPath(missing), missing));
}

TEST_CASE("finder/executableNameVariants") {
    // A name with an extension is used as-is.
    const auto withExtension = executableNameVariants(fs::path("claude.sh"));
    REQUIRE(withExtension.size() == 1);
    CHECK_EQ(withExtension[0], fs::path("claude.sh"));

    const auto bare = executableNameVariants(fs::path("claude"));
    CHECK_FALSE(bare.empty());
    // The plain name is always the last resort.
    CHECK_EQ(bare.back(), fs::path("claude"));
#ifdef _WIN32
    CHECK(std::find(bare.begin(), bare.end(), fs::path("claude.exe")) !=
          bare.end());
#else
    CHECK_EQ(bare.size(), static_cast<std::size_t>(1));
#endif
}

TEST_CASE("finder/filenameMatchesName") {
    const fs::path file = fs::path("dir") / "Claude.exe";
    CHECK(filenameMatchesName(file, "", false));
    CHECK(filenameMatchesName(file, "Claude.exe", false));
    CHECK(filenameMatchesName(file, "Claude", false));  // stem
    CHECK_FALSE(filenameMatchesName(file, "claude", false));
    CHECK(filenameMatchesName(file, "claude", true));
    CHECK(filenameMatchesName(file, "Cl*", false));
    CHECK(filenameMatchesName(file, "cl*", true));
    CHECK_FALSE(filenameMatchesName(file, "cl*", false));
    CHECK_FALSE(filenameMatchesName(file, "other", true));
}

// --- ClaudeExcludeMatcher ------------------------------------------------------

TEST_CASE("finder/exclude matcher matches file names and stems") {
    TempDir temp;
    PathResolver resolver(temp.path());
    ClaudeExcludeMatcher matcher({"ClaudeX*", "skipme"}, resolver);

    CHECK(matcher.matches(temp.path() / "ClaudeX.exe"));
    CHECK(matcher.matches(temp.path() / "ClaudeX-linux"));
    CHECK(matcher.matches(temp.path() / "skipme"));
    CHECK(matcher.matches(temp.path() / "skipme.exe"));
    CHECK_FALSE(matcher.matches(temp.path() / "claude"));
    CHECK_FALSE(matcher.matches(temp.path() / "claude.exe"));
    CHECK_FALSE(matcher.matches(fs::path()));
}

TEST_CASE("finder/exclude matcher ignores blank patterns") {
    TempDir temp;
    PathResolver resolver(temp.path());
    ClaudeExcludeMatcher matcher({"", "   ", "\"\""}, resolver);
    CHECK_FALSE(matcher.matches(temp.path() / "anything"));
}

TEST_CASE("finder/exclude matcher handles absolute and relative paths") {
    TempDir temp;
    const fs::path blocked = makeExecutable(temp.path() / "blocked", "claude");
    const fs::path allowed = makeExecutable(temp.path() / "allowed", "claude");
    PathResolver resolver(temp.path());

    ClaudeExcludeMatcher absolute({pathToUtf8(blocked)}, resolver);
    CHECK(absolute.matches(blocked));
    CHECK_FALSE(absolute.matches(allowed));

    ClaudeExcludeMatcher relative({"blocked/" + exeName("claude")}, resolver);
    CHECK(relative.matches(blocked));
    CHECK_FALSE(relative.matches(allowed));

    ClaudeExcludeMatcher templated({"{root}/blocked/" + exeName("claude")},
                                   resolver);
    CHECK(templated.matches(blocked));
    CHECK_FALSE(templated.matches(allowed));
}

TEST_CASE("finder/exclude matcher supports path wildcards and folders") {
    TempDir temp;
    const fs::path blocked = makeExecutable(temp.path() / "vendor", "claude");
    const fs::path other = makeExecutable(temp.path() / "mine", "claude");
    PathResolver resolver(temp.path());

    ClaudeExcludeMatcher wildcard({"{root}/vendor/*"}, resolver);
    CHECK(wildcard.matches(blocked));
    CHECK_FALSE(wildcard.matches(other));

    // A bare template that expands to a path is a path pattern.
    ClaudeExcludeMatcher whole({"{root}"}, resolver);
    CHECK(whole.matches(temp.path()));
    CHECK_FALSE(whole.matches(blocked));
}

// --- directory search --------------------------------------------------------

TEST_CASE("finder/findClaudeInDirectory finds an exact name") {
    TempDir temp;
    PathResolver resolver(temp.path());
    const ClaudeExcludeMatcher none({}, resolver);
    const fs::path exe = makeExecutable(temp.path(), "claude");
    makeExecutable(temp.path(), "other");

    ClaudeDirectorySearch result =
        findClaudeInDirectory(temp.path(), "claude", false, none);
    CHECK(pathsEqual(result.selected, exe));
    CHECK_FALSE(result.ambiguous);
    CHECK_FALSE(result.usedSingleFallback);
}

TEST_CASE("finder/findClaudeInDirectory matches wildcard patterns") {
    TempDir temp;
    PathResolver resolver(temp.path());
    const ClaudeExcludeMatcher none({}, resolver);
    makeExecutable(temp.path(), "claude-2");
    const fs::path first = makeExecutable(temp.path(), "claude-1");
    makeExecutable(temp.path(), "unrelated");

    ClaudeDirectorySearch result =
        findClaudeInDirectory(temp.path(), "claude-*", false, none);
    CHECK(pathsEqual(result.selected, first));
    CHECK_EQ(result.rankedMatches.size(), static_cast<std::size_t>(2));
}

TEST_CASE("finder/findClaudeInDirectory skips excluded files") {
    TempDir temp;
    PathResolver resolver(temp.path());
    makeExecutable(temp.path(), "claude");
    const ClaudeExcludeMatcher exclude({"claude*"}, resolver);

    ClaudeDirectorySearch result =
        findClaudeInDirectory(temp.path(), "claude*", false, exclude);
    CHECK(result.selected.empty());
}

TEST_CASE("finder/findClaudeInDirectory ignores data files") {
    TempDir temp;
    PathResolver resolver(temp.path());
    const ClaudeExcludeMatcher none({}, resolver);
    makeDataFile(temp.path(), "claude.json");
    makeDataFile(temp.path(), "claude.log");

    ClaudeDirectorySearch result =
        findClaudeInDirectory(temp.path(), "claude*", true, none);
    CHECK(result.selected.empty());
    CHECK_FALSE(result.ambiguous);
}

TEST_CASE("finder/findClaudeInDirectory single-candidate fallback") {
    TempDir temp;
    PathResolver resolver(temp.path());
    const ClaudeExcludeMatcher none({}, resolver);
    const fs::path only = makeExecutable(temp.path(), "renamed-tool");

    ClaudeDirectorySearch refused =
        findClaudeInDirectory(temp.path(), "claude*", false, none);
    CHECK(refused.selected.empty());

    ClaudeDirectorySearch accepted =
        findClaudeInDirectory(temp.path(), "claude*", true, none);
    CHECK(pathsEqual(accepted.selected, only));
    CHECK(accepted.usedSingleFallback);
}

TEST_CASE("finder/findClaudeInDirectory reports ambiguous fallbacks") {
    TempDir temp;
    PathResolver resolver(temp.path());
    const ClaudeExcludeMatcher none({}, resolver);
    makeExecutable(temp.path(), "tool-a");
    makeExecutable(temp.path(), "tool-b");

    ClaudeDirectorySearch result =
        findClaudeInDirectory(temp.path(), "claude*", true, none);
    CHECK(result.selected.empty());
    CHECK(result.ambiguous);
    CHECK_EQ(result.candidates.size(), static_cast<std::size_t>(2));
}

TEST_CASE("finder/findClaudeInDirectory tolerates missing directories") {
    TempDir temp;
    PathResolver resolver(temp.path());
    const ClaudeExcludeMatcher none({}, resolver);
    ClaudeDirectorySearch result =
        findClaudeInDirectory(temp.path() / "nope", "claude*", true, none);
    CHECK(result.selected.empty());
    CHECK_FALSE(result.ambiguous);
}

TEST_CASE("finder/sortExecutableCandidates is deterministic") {
    std::vector<fs::path> paths = {fs::path("b") / "claude", fs::path("a") / "claude",
                                   fs::path("a") / "alpha"};
    sortExecutableCandidates(paths);
    CHECK_EQ(pathSortKey(paths[0]), pathSortKey(fs::path("a") / "alpha"));
    CHECK_EQ(pathSortKey(paths[1]), pathSortKey(fs::path("a") / "claude"));
    CHECK_EQ(pathSortKey(paths[2]), pathSortKey(fs::path("b") / "claude"));
}

// --- PATH --------------------------------------------------------------------

TEST_CASE("finder/pathDirectories splits and cleans PATH") {
    TempDir temp;
    const std::string value = pathToUtf8(temp.path() / "one") + kPathSeparator +
                              "\"" + pathToUtf8(temp.path() / "two") + "\"" +
                              kPathSeparator + kPathSeparator +
                              pathToUtf8(temp.path() / "three");
    ScopedEnv env("PATH", value);

    const std::vector<fs::path> directories = pathDirectories();
    REQUIRE(directories.size() == 4);
    CHECK(pathsEqual(directories[0], temp.path() / "one"));
    CHECK(pathsEqual(directories[1], temp.path() / "two"));
    // An empty entry means the current directory.
    std::error_code ec;
    CHECK(pathsEqual(directories[2], fs::current_path(ec)));
    CHECK(pathsEqual(directories[3], temp.path() / "three"));
}

TEST_CASE("finder/pathDirectories is empty without PATH") {
    ScopedEnv env("PATH", "");
    CHECK(pathDirectories().empty());
}

TEST_CASE("finder/findExecutableOnPath") {
    TempDir temp;
    const fs::path exe = makeExecutable(temp.path() / "bin", "mytool");
    ScopedEnv env("PATH", pathToUtf8(temp.path() / "bin"));

    auto found = findExecutableOnPath("mytool");
    REQUIRE(found.has_value());
    CHECK(pathsEqual(*found, exe));
    CHECK_FALSE(findExecutableOnPath("not-here").has_value());
    CHECK_FALSE(findExecutableOnPath("").has_value());
}

TEST_CASE("finder/findExecutableInDirectory") {
    TempDir temp;
    const fs::path exe = makeExecutable(temp.path(), "mytool");
    auto found = findExecutableInDirectory(temp.path(), "mytool");
    REQUIRE(found.has_value());
    CHECK(pathsEqual(*found, exe));
    CHECK_FALSE(findExecutableInDirectory(temp.path(), "other").has_value());
}

TEST_CASE("finder/findClaudeOnPath returns the first directory with a match") {
    TempDir temp;
    PathResolver resolver(temp.path());
    const ClaudeExcludeMatcher none({}, resolver);
    fs::create_directories(temp.path() / "empty");
    const fs::path exe = makeExecutable(temp.path() / "second", "claude");
    ScopedEnv env("PATH", pathToUtf8(temp.path() / "empty") + kPathSeparator +
                              pathToUtf8(temp.path() / "second"));

    ClaudeDirectorySearch result = findClaudeOnPath("claude", none);
    CHECK(pathsEqual(result.selected, exe));
}

// --- ConfigManager::findClaudeExecutable ---------------------------------------

TEST_CASE("finder/findClaudeExecutable finds claude next to the launcher") {
    ScopedEnv env("PATH", "");
    Fixture fixture;
    const fs::path exe = makeExecutable(fixture.root.path(), "claude");

    std::string error;
    auto found = fixture.config.findClaudeExecutable(error);
    REQUIRE(found.has_value());
    CHECK(pathsEqual(*found, exe));
    CHECK(error.empty());
}

TEST_CASE("finder/findClaudeExecutable never picks the launcher itself") {
    ScopedEnv env("PATH", "");
    Fixture fixture;
    makeExecutable(fixture.root.path(), "ClaudeX");

    std::string error;
    CHECK_FALSE(fixture.config.findClaudeExecutable(error).has_value());
    CHECK_FALSE(error.empty());
}

TEST_CASE("finder/findClaudeExecutable reports a missing install") {
    ScopedEnv env("PATH", "");
    Fixture fixture;

    std::string error;
    CHECK_FALSE(fixture.config.findClaudeExecutable(error).has_value());
    CHECK_EQ(error, messageText(fixture.config.menuTheme, "claudeNotFound", {}));
}

TEST_CASE("finder/findClaudeExecutable falls back to PATH") {
    TempDir bin;
    const fs::path exe = makeExecutable(bin.path(), "claude");
    ScopedEnv env("PATH", pathToUtf8(bin.path()));
    Fixture fixture;

    std::string error;
    auto found = fixture.config.findClaudeExecutable(error);
    REQUIRE(found.has_value());
    CHECK(pathsEqual(*found, exe));
}

TEST_CASE("finder/findClaudeExecutable honors claudeExclude") {
    TempDir bin;
    makeExecutable(bin.path(), "claude");
    ScopedEnv env("PATH", pathToUtf8(bin.path()));
    Fixture fixture(R"({"claudeExclude": ["claude*"]})");

    std::string error;
    CHECK_FALSE(fixture.config.findClaudeExecutable(error).has_value());
}

TEST_CASE("finder/findClaudeExecutable searches an explicit folder") {
    ScopedEnv env("PATH", "");
    Fixture fixture(R"({"claudePath": "tools"})");
    // Any single runnable file in an explicit folder is accepted.
    const fs::path exe = makeExecutable(fixture.root.path() / "tools", "my-claude");

    std::string error;
    auto found = fixture.config.findClaudeExecutable(error);
    REQUIRE(found.has_value());
    CHECK(pathsEqual(*found, exe));
}

TEST_CASE("finder/findClaudeExecutable accepts an explicit file path") {
    ScopedEnv env("PATH", "");
    Fixture fixture("{\"claudePath\": \"tools/" + exeName("special") + "\"}");
    const fs::path exe = makeExecutable(fixture.root.path() / "tools", "special");

    std::string error;
    auto found = fixture.config.findClaudeExecutable(error);
    REQUIRE(found.has_value());
    CHECK(pathsEqual(*found, exe));
}

TEST_CASE("finder/findClaudeExecutable expands path wildcards") {
    ScopedEnv env("PATH", "");
    Fixture fixture(R"({"claudePath": "bin/*/claude*"})");
    makeExecutable(fixture.root.path() / "bin" / "v2", "claude");
    const fs::path first = makeExecutable(fixture.root.path() / "bin" / "v1", "claude");

    std::string error;
    auto found = fixture.config.findClaudeExecutable(error);
    REQUIRE(found.has_value());
    CHECK(pathsEqual(*found, first));
}

TEST_CASE("finder/findClaudeExecutable reports a missing explicit path") {
    ScopedEnv env("PATH", "");
    Fixture fixture(R"({"claudePath": "nowhere/claude"})");

    std::string error;
    CHECK_FALSE(fixture.config.findClaudeExecutable(error).has_value());
    CHECK(error.find("nowhere/claude") != std::string::npos);
}

TEST_CASE("finder/findClaudeExecutable reports files that cannot run") {
    ScopedEnv env("PATH", "");
    Fixture fixture(R"({"claudePath": "notes.txt"})");
    makeDataFile(fixture.root.path(), "notes.txt");

    std::string error;
    CHECK_FALSE(fixture.config.findClaudeExecutable(error).has_value());
    CHECK(error.find("notes.txt") != std::string::npos);
}

TEST_CASE("finder/findClaudeExecutable tries entries in order") {
    ScopedEnv env("PATH", "");
    Fixture fixture("{\"claudePath\": [\"missing/claude\", \"tools/" + exeName("real") + "\"]}");
    const fs::path exe = makeExecutable(fixture.root.path() / "tools", "real");

    std::string error;
    auto found = fixture.config.findClaudeExecutable(error);
    REQUIRE(found.has_value());
    CHECK(pathsEqual(*found, exe));
}

TEST_CASE("finder/findClaudeExecutable reports ambiguous folders") {
    ScopedEnv env("PATH", "");
    Fixture fixture(R"({"claudePath": "tools"})");
    makeExecutable(fixture.root.path() / "tools", "alpha");
    makeExecutable(fixture.root.path() / "tools", "beta");

    std::string error;
    CHECK_FALSE(fixture.config.findClaudeExecutable(error).has_value());
    CHECK(error.find("alpha") != std::string::npos);
    CHECK(error.find("beta") != std::string::npos);
}

TEST_CASE("finder/exclude patterns follow the platform's case rules") {
    TempDir temp;
    PathResolver resolver(temp.path());
    ClaudeExcludeMatcher matcher({"claudex*"}, resolver);
    CHECK_EQ(matcher.matches(temp.path() / "ClaudeX.exe"), kCaseInsensitive);
    CHECK(matcher.matches(temp.path() / "claudex.exe"));
}
