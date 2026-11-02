#include "test_framework.hpp"

#include "path_resolver.hpp"
#include "scoped_env.hpp"
#include "text_util.hpp"

using namespace claudex;
using testfw::ScopedEnv;
using testfw::TempDir;
using testfw::writeFile;

namespace {

std::string quoted(const fs::path& path) {
    return quoteShellArgument(pathToUtf8(path));
}

}  // namespace

// --- expandPathValue -------------------------------------------------------

TEST_CASE("path/expandPathValue substitutes template tokens") {
    TempDir temp;
    CHECK_EQ(expandPathValue("{root}/x", temp.path()),
             pathToUtf8(temp.path()) + "/x");
    CHECK_EQ(expandPathValue("{platform}-{arch}", temp.path()),
             platformName() + "-" + archName());
    CHECK_EQ(expandPathValue("{home}", temp.path()), homeDirectory());
}

TEST_CASE("path/expandPathValue {drive}") {
    TempDir temp;
#ifdef _WIN32
    CHECK_EQ(expandPathValue("{drive}/Projects", temp.path()),
             pathToUtf8(temp.path().root_name()) + "/Projects");
#else
    const std::string expanded = expandPathValue("{drive}", temp.path());
    CHECK(!expanded.empty());
    CHECK(expanded.front() == '/');
#endif
}

TEST_CASE("path/expandPathValue leaves unknown placeholders alone") {
    TempDir temp;
    CHECK_EQ(expandPathValue("{nope}/x", temp.path()), "{nope}/x");
    CHECK_EQ(expandPathValue("plain/relative", temp.path()), "plain/relative");
    CHECK_EQ(expandPathValue("", temp.path()), "");
}

TEST_CASE("path/expandPathValue expands a leading tilde only") {
    TempDir temp;
    ScopedEnv home("HOME", "/claudex/home");
    CHECK_EQ(expandPathValue("~", temp.path()), "/claudex/home");
    CHECK_EQ(expandPathValue("~/work", temp.path()), "/claudex/home/work");
    CHECK_EQ(expandPathValue("~\\work", temp.path()), "/claudex/home\\work");
    // ~name and an embedded ~ are not home references.
    CHECK_EQ(expandPathValue("~other/work", temp.path()), "~other/work");
    CHECK_EQ(expandPathValue("a/~/b", temp.path()), "a/~/b");
}

TEST_CASE("path/expandPathValue expands environment variables") {
    TempDir temp;
    ScopedEnv env("CLAUDEX_TEST_DIR", "from-env");
    CHECK_EQ(expandPathValue("$CLAUDEX_TEST_DIR/x", temp.path()),
             "from-env/x");
    CHECK_EQ(expandPathValue("%CLAUDEX_TEST_DIR%/x", temp.path()),
             "from-env/x");
}

// --- resolvePathSpec -------------------------------------------------------

TEST_CASE("path/resolve rejects empty input") {
    TempDir temp;
    PathResolution result = resolvePathSpec("", temp.path(), PathKind::Any,
                                            MultiMatchPolicy::FirstSorted);
    CHECK_FALSE(result.resolved);
}

TEST_CASE("path/resolve plain relative path uses the root") {
    TempDir temp;
    fs::create_directories(temp.path() / "sub");
    PathResolution result = resolvePathSpec("sub", temp.path(), PathKind::Any,
                                            MultiMatchPolicy::FirstSorted);
    CHECK(result.resolved);
    CHECK_FALSE(result.wildcard);
    CHECK(pathsEqual(result.path, temp.path() / "sub"));
}

TEST_CASE("path/resolve plain path does not need to exist") {
    TempDir temp;
    PathResolution result =
        resolvePathSpec("missing/dir", temp.path(), PathKind::Directory,
                        MultiMatchPolicy::FirstSorted);
    CHECK(result.resolved);
    CHECK(pathsEqual(result.path, temp.path() / "missing" / "dir"));
}

TEST_CASE("path/resolve strips quotes and expands templates") {
    TempDir temp;
    fs::create_directories(temp.path() / "q");
    PathResolution result =
        resolvePathSpec("\"{root}/q\"", temp.path(), PathKind::Any,
                        MultiMatchPolicy::FirstSorted);
    CHECK(result.resolved);
    CHECK(pathsEqual(result.path, temp.path() / "q"));
}

TEST_CASE("path/resolve normalizes dot segments") {
    TempDir temp;
    fs::create_directories(temp.path() / "a" / "b");
    PathResolution result =
        resolvePathSpec("a/b/../b", temp.path(), PathKind::Any,
                        MultiMatchPolicy::FirstSorted);
    CHECK(result.resolved);
    CHECK(pathsEqual(result.path, temp.path() / "a" / "b"));
}

TEST_CASE("path/resolve wildcard picks the first sorted match") {
    TempDir temp;
    writeFile(temp.path() / "tool-b", "b");
    writeFile(temp.path() / "tool-a", "a");
    writeFile(temp.path() / "other", "o");

    PathResolution result =
        resolvePathSpec("tool-*", temp.path(), PathKind::File,
                        MultiMatchPolicy::FirstSorted);
    CHECK(result.wildcard);
    CHECK(result.resolved);
    CHECK(result.ambiguous);
    REQUIRE(result.matches.size() == 2);
    CHECK(pathsEqual(result.path, temp.path() / "tool-a"));
    CHECK(pathsEqual(result.matches[0], temp.path() / "tool-a"));
    CHECK(pathsEqual(result.matches[1], temp.path() / "tool-b"));
}

TEST_CASE("path/resolve RequireUnique refuses ambiguous wildcards") {
    TempDir temp;
    writeFile(temp.path() / "tool-a", "a");
    writeFile(temp.path() / "tool-b", "b");

    PathResolution result =
        resolvePathSpec("tool-*", temp.path(), PathKind::File,
                        MultiMatchPolicy::RequireUnique);
    CHECK_FALSE(result.resolved);
    CHECK(result.ambiguous);
    CHECK_EQ(result.matches.size(), static_cast<std::size_t>(2));
}

TEST_CASE("path/resolve RequireUnique accepts a single wildcard match") {
    TempDir temp;
    writeFile(temp.path() / "only-one", "x");

    PathResolution result =
        resolvePathSpec("only-*", temp.path(), PathKind::File,
                        MultiMatchPolicy::RequireUnique);
    CHECK(result.resolved);
    CHECK_FALSE(result.ambiguous);
    CHECK(pathsEqual(result.path, temp.path() / "only-one"));
}

TEST_CASE("path/resolve wildcard without matches reports noMatch") {
    TempDir temp;
    PathResolution result =
        resolvePathSpec("nothing-*", temp.path(), PathKind::Any,
                        MultiMatchPolicy::FirstSorted);
    CHECK(result.wildcard);
    CHECK(result.noMatch);
    CHECK_FALSE(result.resolved);
}

TEST_CASE("path/resolve wildcard honors the requested kind") {
    TempDir temp;
    fs::create_directories(temp.path() / "dir-x");
    writeFile(temp.path() / "file-x", "x");

    PathResolution dirs = resolvePathSpec("*-x", temp.path(), PathKind::Directory,
                                          MultiMatchPolicy::RequireUnique);
    CHECK(dirs.resolved);
    CHECK(pathsEqual(dirs.path, temp.path() / "dir-x"));

    PathResolution files = resolvePathSpec("*-x", temp.path(), PathKind::File,
                                           MultiMatchPolicy::RequireUnique);
    CHECK(files.resolved);
    CHECK(pathsEqual(files.path, temp.path() / "file-x"));

    PathResolution any = resolvePathSpec("*-x", temp.path(), PathKind::Any,
                                         MultiMatchPolicy::RequireUnique);
    CHECK_FALSE(any.resolved);
    CHECK(any.ambiguous);
}

TEST_CASE("path/resolve wildcards in directory components") {
    TempDir temp;
    writeFile(temp.path() / "pkg-2" / "bin" / "tool", "2");
    writeFile(temp.path() / "pkg-1" / "bin" / "tool", "1");
    writeFile(temp.path() / "pkg-3" / "lib" / "tool", "3");

    PathResolution result =
        resolvePathSpec("pkg-*/bin/tool", temp.path(), PathKind::File,
                        MultiMatchPolicy::FirstSorted);
    REQUIRE(result.matches.size() == 2);
    CHECK(pathsEqual(result.matches[0], temp.path() / "pkg-1" / "bin" / "tool"));
    CHECK(pathsEqual(result.matches[1], temp.path() / "pkg-2" / "bin" / "tool"));
}

TEST_CASE("path/expandWildcardPath of an empty pattern is empty") {
    CHECK(expandWildcardPath(fs::path(), PathKind::Any).empty());
}

TEST_CASE("path/expandWildcardPath matches question marks") {
    TempDir temp;
    writeFile(temp.path() / "a1", "");
    writeFile(temp.path() / "a2", "");
    writeFile(temp.path() / "a10", "");
    auto matches = expandWildcardPath(temp.path() / "a?", PathKind::File);
    CHECK_EQ(matches.size(), static_cast<std::size_t>(2));
}

// --- stored paths ----------------------------------------------------------

TEST_CASE("path/pathContainsParentTraversal") {
    CHECK(pathContainsParentTraversal(fs::path("a") / ".." / "b"));
    CHECK(pathContainsParentTraversal(fs::path("..")));
    CHECK_FALSE(pathContainsParentTraversal(fs::path("a") / "b"));
    CHECK_FALSE(pathContainsParentTraversal(fs::path("a..b")));
}

TEST_CASE("path/makeStoredPath is relative inside the root") {
    TempDir temp;
    fs::create_directories(temp.path() / "sub" / "dir");
    CHECK_EQ(makeStoredPath(temp.path() / "sub" / "dir", temp.path()),
             "sub/dir");
}

TEST_CASE("path/makeStoredPath is absolute outside the root") {
    TempDir root;
    TempDir other;
    const std::string stored = makeStoredPath(other.path(), root.path());
    CHECK_EQ(stored, genericPathToUtf8(other.path()));
}

TEST_CASE("path/makeStoredProjectPath keeps templates") {
    TempDir temp;
    fs::create_directories(temp.path() / "proj");
    CHECK_EQ(makeStoredProjectPath("{root}/proj", temp.path() / "proj",
                                   temp.path()),
             "{root}/proj");
    CHECK_EQ(makeStoredProjectPath("  {root}/proj  ", temp.path() / "proj",
                                   temp.path()),
             "{root}/proj");
}

TEST_CASE("path/makeStoredProjectPath keeps environment variables") {
    TempDir temp;
    ScopedEnv env("CLAUDEX_TEST_PROJECTS", temp.path().generic_string());
    fs::create_directories(temp.path() / "proj");
    CHECK_EQ(makeStoredProjectPath("$CLAUDEX_TEST_PROJECTS/proj",
                                   temp.path() / "proj", temp.path()),
             "$CLAUDEX_TEST_PROJECTS/proj");
}

TEST_CASE("path/makeStoredProjectPath shortens relative input inside root") {
    TempDir temp;
    fs::create_directories(temp.path() / "proj");
    CHECK_EQ(makeStoredProjectPath("proj", temp.path() / "proj", temp.path()),
             "proj");
    CHECK_EQ(makeStoredProjectPath("./proj", temp.path() / "proj", temp.path()),
             "proj");
}

TEST_CASE("path/makeStoredProjectPath keeps relative input outside root") {
    TempDir temp;
    fs::path root = temp.path() / "launcher";
    fs::create_directories(root);
    fs::create_directories(temp.path() / "outside");
    CHECK_EQ(makeStoredProjectPath("../outside", temp.path() / "outside", root),
             "../outside");
}

TEST_CASE("path/makeStoredProjectPath relativizes absolute input") {
    TempDir temp;
    fs::create_directories(temp.path() / "proj");
    CHECK_EQ(makeStoredProjectPath(pathToUtf8(temp.path() / "proj"),
                                   temp.path() / "proj", temp.path()),
             "proj");

    TempDir other;
    CHECK_EQ(makeStoredProjectPath(pathToUtf8(other.path()), other.path(),
                                   temp.path()),
             genericPathToUtf8(other.path()));
}

TEST_CASE("path/makeStoredProjectPath stores the concrete wildcard match") {
    TempDir temp;
    fs::create_directories(temp.path() / "project-one");
    CHECK_EQ(makeStoredProjectPath("project-*", temp.path() / "project-one",
                                   temp.path()),
             "project-one");
}

// --- PathResolver ----------------------------------------------------------

TEST_CASE("path/PathResolver resolveWithBase resolves against the base") {
    TempDir root;
    TempDir base;
    fs::create_directories(base.path() / "sub");
    PathResolver resolver(root.path());

    PathResolution relative = resolver.resolveWithBase(
        "sub", base.path(), PathKind::Any, MultiMatchPolicy::FirstSorted);
    CHECK(relative.resolved);
    CHECK(pathsEqual(relative.path, base.path() / "sub"));

    // Templates still expand against the resolver root, not the base.
    fs::create_directories(root.path() / "tpl");
    PathResolution templated = resolver.resolveWithBase(
        "{root}/tpl", base.path(), PathKind::Any,
        MultiMatchPolicy::FirstSorted);
    CHECK(pathsEqual(templated.path, root.path() / "tpl"));
}

TEST_CASE("path/PathResolver normalizeAbsolutePath and expand") {
    TempDir temp;
    PathResolver resolver(temp.path());
    CHECK(pathsEqual(resolver.rootDir(), temp.path()));
    CHECK_EQ(resolver.expand("{root}/a"), pathToUtf8(temp.path()) + "/a");
    CHECK(pathsEqual(resolver.normalizeAbsolutePath("x/../y"),
                     temp.path() / "y"));
}

TEST_CASE("path/launchCommand expands placeholders") {
    TempDir temp;
    PathResolver resolver(temp.path());
    const fs::path configDir = temp.path() / "data";
    const fs::path tempDir = temp.path() / "data" / "tmp";

    const std::string command = resolver.expandLaunchCommand(
        "run --cfg {claudeConfigDir} --tmp {tempDir} --os {platform}",
        configDir, tempDir);
    // Only the first word is treated as an executable path, so the flag
    // values pass through untouched.
    CHECK_EQ(command, "run --cfg " + pathToUtf8(configDir) + " --tmp " +
                          pathToUtf8(tempDir) + " --os " + platformName());
}

TEST_CASE("path/launchCommand leaves the claude placeholder to the caller") {
    TempDir temp;
    PathResolver resolver(temp.path());
    CHECK_EQ(resolver.expandLaunchCommand("{claude} --model sonnet", "c", "t"),
             "{claude} --model sonnet");
}

TEST_CASE("path/launchCommand quotes an absolute first word") {
    // A synthetic root without spaces: the expansion never touches the disk,
    // and the system temp folder may legitimately contain spaces.
#ifdef _WIN32
    const fs::path root = "C:/claudex-root";
#else
    const fs::path root = "/claudex-root";
#endif
    PathResolver resolver(root);
    const fs::path configDir = root / "data";
    const std::string command =
        resolver.expandLaunchCommand("{claudeConfigDir}/run.sh --x", configDir,
                                     configDir / "tmp");
    CHECK_EQ(command,
             quoteShellArgument(pathToUtf8(configDir) + "/run.sh") + " --x");
}

TEST_CASE("path/launchCommand rewrites explicit relative words") {
    TempDir temp;
    PathResolver resolver(temp.path());
    const std::string command = resolver.expandLaunchCommand(
        "./bin/tool --flag ../other", "c", "t");
    const fs::path first =
        (temp.path() / pathFromUtf8("bin/tool")).lexically_normal();
    const fs::path second =
        (temp.path() / pathFromUtf8("../other")).lexically_normal();
    CHECK_EQ(command, quoted(first) + " --flag " + quoted(second));
}

TEST_CASE("path/launchCommand resolves a first word that exists next to the launcher") {
    TempDir temp;
    writeFile(temp.path() / "wrapper.cmd", "echo hi");
    PathResolver resolver(temp.path());
    const std::string command =
        resolver.expandLaunchCommand("wrapper.cmd arg", "c", "t");
    CHECK_EQ(command, quoted((temp.path() / "wrapper.cmd").lexically_normal()) +
                          " arg");
}

TEST_CASE("path/launchCommand leaves shell words alone") {
    TempDir temp;
    PathResolver resolver(temp.path());
    CHECK_EQ(resolver.expandLaunchCommand("echo hello", "c", "t"),
             "echo hello");
    CHECK_EQ(resolver.expandLaunchCommand("wsl claude --model sonnet", "c", "t"),
             "wsl claude --model sonnet");
    CHECK_EQ(resolver.expandLaunchCommand("tool --config=./x", "c", "t"),
             "tool --config=./x");
    CHECK_EQ(resolver.expandLaunchCommand("https://example.com/x", "c", "t"),
             "https://example.com/x");
    CHECK_EQ(resolver.expandLaunchCommand("VAR=1 tool", "c", "t"),
             "VAR=1 tool");
    CHECK_EQ(resolver.expandLaunchCommand("$HOME/tool", "c", "t"),
             "$HOME/tool");
}

TEST_CASE("path/launchCommand keeps quoted words and separators") {
    TempDir temp;
    PathResolver resolver(temp.path());
    CHECK_EQ(resolver.expandLaunchCommand("echo \"a && b\" && echo done", "c",
                                          "t"),
             "echo \"a && b\" && echo done");
}
