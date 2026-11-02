#include "test_framework.hpp"

#include "config_json.hpp"
#include "config_manager.hpp"
#include "launcher.hpp"
#include "scoped_env.hpp"
#include "text_util.hpp"

using namespace claudex;
using testfw::ScopedEnv;
using testfw::TempDir;
using testfw::writeFile;

namespace {

// Shell snippet that exits with a given status on every platform.
std::string exitWith(int code) { return "exit " + std::to_string(code); }

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

// --- ensureDirectory / preparePortable* -------------------------------------------

TEST_CASE("launcher/ensureDirectory creates nested folders") {
    TempDir temp;
    const MenuTheme theme = menuThemeFromDocument(embeddedDefaultTheme());
    std::string error;
    const fs::path target = temp.path() / "a" / "b" / "c";
    CHECK(ensureDirectory(target, error, theme));
    CHECK(error.empty());
    CHECK(fs::is_directory(target));
    // An existing folder is fine.
    CHECK(ensureDirectory(target, error, theme));
}

TEST_CASE("launcher/ensureDirectory fails when a file is in the way") {
    TempDir temp;
    const MenuTheme theme = menuThemeFromDocument(embeddedDefaultTheme());
    writeFile(temp.path() / "file", "x");
    std::string error;
    CHECK_FALSE(ensureDirectory(temp.path() / "file", error, theme));
    CHECK_FALSE(error.empty());
    CHECK_FALSE(ensureDirectory(temp.path() / "file" / "child", error, theme));
    CHECK_FALSE(error.empty());
}

TEST_CASE("launcher/preparePortableDirectories creates config and temp folders") {
    Fixture fixture;
    fs::path configDir;
    fs::path tempDir;
    std::string error;
    REQUIRE(preparePortableDirectories(fixture.config, configDir, tempDir, error));
    CHECK(pathsEqual(configDir, fixture.root.path() / "data"));
    CHECK(pathsEqual(tempDir, fixture.root.path() / "data" / "tmp"));
    CHECK(fs::is_directory(configDir));
    CHECK(fs::is_directory(tempDir));
}

TEST_CASE("launcher/preparePortableDirectories reports blocked folders") {
    Fixture fixture(R"({"claudeConfigDir": "blocked", "tempDir": "t"})");
    writeFile(fixture.root.path() / "blocked", "i am a file");
    fs::path configDir;
    fs::path tempDir;
    std::string error;
    CHECK_FALSE(
        preparePortableDirectories(fixture.config, configDir, tempDir, error));
    CHECK_FALSE(error.empty());
}

TEST_CASE("launcher/preparePortableEnvironment exports the portable variables") {
    ScopedEnv configDirVar("CLAUDE_CONFIG_DIR", "old");
    ScopedEnv tempVar("TEMP", "old");
    ScopedEnv tmpVar("TMP", "old");
    ScopedEnv tmpdirVar("TMPDIR", "old");

    Fixture fixture;
    std::string error;
    REQUIRE(preparePortableEnvironment(fixture.config, error));

    const std::string expectedConfig = pathToUtf8(fixture.config.claudeConfigDir());
    const std::string expectedTemp = pathToUtf8(fixture.config.tempDir());
    CHECK_EQ(getEnvUtf8("CLAUDE_CONFIG_DIR").value_or(""), expectedConfig);
    CHECK_EQ(getEnvUtf8("TEMP").value_or(""), expectedTemp);
    CHECK_EQ(getEnvUtf8("TMP").value_or(""), expectedTemp);
    CHECK_EQ(getEnvUtf8("TMPDIR").value_or(""), expectedTemp);
}

// --- Windows argument quoting ---------------------------------------------------------

#ifdef _WIN32
TEST_CASE("launcher/quoteWindowsArgument follows CreateProcess rules") {
    CHECK(quoteWindowsArgument(L"") == L"\"\"");
    CHECK(quoteWindowsArgument(L"simple") == L"simple");
    CHECK(quoteWindowsArgument(L"two words") == L"\"two words\"");
    CHECK(quoteWindowsArgument(L"say \"hi\"") == L"\"say \\\"hi\\\"\"");
    // Backslashes only need escaping when they precede a quote.
    CHECK(quoteWindowsArgument(L"C:\\dir\\file") == L"C:\\dir\\file");
    CHECK(quoteWindowsArgument(L"C:\\my dir\\") == L"\"C:\\my dir\\\\\"");
    CHECK(quoteWindowsArgument(L"a\\\"b") == L"\"a\\\\\\\"b\"");
}
#endif

// --- running commands -----------------------------------------------------------------

TEST_CASE("launcher/runShellCommand reports the exit code") {
    TempDir temp;
    const MenuTheme theme = menuThemeFromDocument(embeddedDefaultTheme());

    LaunchResult ok = runShellCommand(exitWith(0), temp.path(), theme);
    CHECK(ok.started);
    CHECK(ok.error.empty());
    CHECK_EQ(ok.exitCode, 0);

    LaunchResult failed = runShellCommand(exitWith(7), temp.path(), theme);
    CHECK(failed.started);
    CHECK_EQ(failed.exitCode, 7);
}

TEST_CASE("launcher/runShellCommand uses the working directory") {
    TempDir temp;
    const MenuTheme theme = menuThemeFromDocument(embeddedDefaultTheme());
    writeFile(temp.path() / "marker.txt", "x");
    // Succeeds only when the shell starts inside the folder holding the marker.
#ifdef _WIN32
    const std::string command = "if exist marker.txt (exit 0) else (exit 3)";
#else
    const std::string command = "test -f marker.txt";
#endif
    LaunchResult result = runShellCommand(command, temp.path(), theme);
    CHECK(result.started);
    CHECK_EQ(result.exitCode, 0);
}

TEST_CASE("launcher/runShellCommand fails cleanly in a missing folder") {
    TempDir temp;
    const MenuTheme theme = menuThemeFromDocument(embeddedDefaultTheme());
    LaunchResult result =
        runShellCommand(exitWith(0), temp.path() / "missing", theme);
#ifdef _WIN32
    CHECK_FALSE(result.started);
    CHECK_FALSE(result.error.empty());
#else
    // The forked child cannot chdir and exits with 126.
    CHECK_EQ(result.exitCode, 126);
#endif
}

// --- launchClaude / runCustomProjectCommand ----------------------------------------------

TEST_CASE("launcher/launchClaude rejects a missing project folder") {
    Fixture fixture;
    LaunchResult result =
        launchClaude(fixture.config, fixture.root.path() / "no-such-project");
    CHECK_FALSE(result.started);
    CHECK_FALSE(result.error.empty());
    CHECK(result.error.find("no-such-project") != std::string::npos);
}

TEST_CASE("launcher/launchClaude runs the configured command in the project") {
    ScopedEnv configDirVar("CLAUDE_CONFIG_DIR", "old");
    ScopedEnv tempVar("TEMP", "old");
    ScopedEnv tmpVar("TMP", "old");
    ScopedEnv tmpdirVar("TMPDIR", "old");

    Fixture fixture("{\"launchCommand\": \"" + exitWith(5) + "\"}");
    const fs::path project = fixture.root.path() / "project";
    fs::create_directories(project);

    LaunchResult result = launchClaude(fixture.config, project);
    CHECK(result.started);
    CHECK(result.error.empty());
    CHECK_EQ(result.exitCode, 5);
    // The portable folders are prepared before the command runs.
    CHECK(fs::is_directory(fixture.root.path() / "data"));
    CHECK(fs::is_directory(fixture.root.path() / "data" / "tmp"));
}

TEST_CASE("launcher/launchClaude reports a missing Claude executable") {
    ScopedEnv path("PATH", "");
    Fixture fixture(R"({"launchCommand": "{claude}"})");
    const fs::path project = fixture.root.path() / "project";
    fs::create_directories(project);

    LaunchResult result = launchClaude(fixture.config, project);
    CHECK_FALSE(result.started);
    CHECK_EQ(result.error,
             messageText(fixture.config.menuTheme, "claudeNotFound", {}));
}

TEST_CASE("launcher/launchClaude writes the launch to the log") {
    Fixture fixture("{\"launchCommand\": \"" + exitWith(0) + "\"}");
    const fs::path project = fixture.root.path() / "project";
    fs::create_directories(project);
    launchClaude(fixture.config, project);

    const std::string log = testfw::readFile(fixture.root.path() / "claudex.log");
    CHECK(log.find("launch: cwd=") != std::string::npos);
    CHECK(log.find("exited with code 0") != std::string::npos);
}

TEST_CASE("launcher/runCustomProjectCommand exports the portable environment") {
    ScopedEnv configDirVar("CLAUDE_CONFIG_DIR", "old");
    ScopedEnv tempVar("TEMP", "old");
    ScopedEnv tmpVar("TMP", "old");
    ScopedEnv tmpdirVar("TMPDIR", "old");

    Fixture fixture;
    const fs::path project = fixture.root.path() / "project";
    fs::create_directories(project);

    LaunchResult result =
        runCustomProjectCommand(fixture.config, project, exitWith(4));
    CHECK(result.started);
    CHECK_EQ(result.exitCode, 4);
    CHECK_EQ(getEnvUtf8("CLAUDE_CONFIG_DIR").value_or(""),
             pathToUtf8(fixture.config.claudeConfigDir()));
}

TEST_CASE("launcher/runCustomProjectCommand rejects a missing folder") {
    Fixture fixture;
    LaunchResult result = runCustomProjectCommand(
        fixture.config, fixture.root.path() / "gone", exitWith(0));
    CHECK_FALSE(result.started);
    CHECK_FALSE(result.error.empty());
}
