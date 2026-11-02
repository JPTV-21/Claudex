#include "test_framework.hpp"

#include "platform.hpp"
#include "scoped_env.hpp"

using namespace claudex;
using testfw::ScopedEnv;

TEST_CASE("platform/utf8 path round trip keeps non-ASCII text") {
    const std::string text = "\xE9\xA1\xB9\xE7\x9B\xAE/\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E";
    CHECK_EQ(genericPathToUtf8(pathFromUtf8(text)), text);
}

TEST_CASE("platform/pathToUtf8 of an empty path is empty") {
    CHECK_EQ(pathToUtf8(fs::path()), "");
    CHECK(pathFromUtf8("").empty());
}

TEST_CASE("platform/environment get and set round trip") {
    ScopedEnv env("CLAUDEX_TEST_ROUNDTRIP", "value-1");
    auto value = getEnvUtf8("CLAUDEX_TEST_ROUNDTRIP");
    REQUIRE(value.has_value());
    CHECK_EQ(*value, "value-1");
}

TEST_CASE("platform/environment values keep non-ASCII text") {
    const std::string text = "\xE9\xA1\xB9\xE7\x9B\xAE";
    ScopedEnv env("CLAUDEX_TEST_UNICODE", text);
    auto value = getEnvUtf8("CLAUDEX_TEST_UNICODE");
    REQUIRE(value.has_value());
    CHECK_EQ(*value, text);
}

TEST_CASE("platform/getEnvUtf8 reports missing variables") {
    CHECK_FALSE(getEnvUtf8("CLAUDEX_TEST_DEFINITELY_NOT_SET_12345").has_value());
}

TEST_CASE("platform/expandEnvironmentVariables supported syntaxes") {
    ScopedEnv env("CLAUDEX_TEST_VAR", "value");
    CHECK_EQ(expandEnvironmentVariables("a/$CLAUDEX_TEST_VAR/b"), "a/value/b");
    CHECK_EQ(expandEnvironmentVariables("a/${CLAUDEX_TEST_VAR}x"), "a/valuex");
    CHECK_EQ(expandEnvironmentVariables("a/%CLAUDEX_TEST_VAR%/b"), "a/value/b");
}

TEST_CASE("platform/expandEnvironmentVariables leaves unknown text alone") {
    CHECK_EQ(expandEnvironmentVariables("$CLAUDEX_TEST_UNKNOWN_99"),
             "$CLAUDEX_TEST_UNKNOWN_99");
    CHECK_EQ(expandEnvironmentVariables("${CLAUDEX_TEST_UNKNOWN_99}"),
             "${CLAUDEX_TEST_UNKNOWN_99}");
    CHECK_EQ(expandEnvironmentVariables("%CLAUDEX_TEST_UNKNOWN_99%"),
             "%CLAUDEX_TEST_UNKNOWN_99%");
    CHECK_EQ(expandEnvironmentVariables("100%"), "100%");
    CHECK_EQ(expandEnvironmentVariables("a%%b"), "a%%b");
    CHECK_EQ(expandEnvironmentVariables("cost: $"), "cost: $");
    CHECK_EQ(expandEnvironmentVariables("$1abc"), "$1abc");
    CHECK_EQ(expandEnvironmentVariables(""), "");
}

TEST_CASE("platform/platformName and archName are known values") {
    const std::string platform = platformName();
    CHECK(platform == "windows" || platform == "macos" || platform == "linux");
    CHECK_FALSE(archName().empty());
}

TEST_CASE("platform/executableSuffixes match the platform") {
#ifdef _WIN32
    const auto& suffixes = executableSuffixes();
    CHECK_FALSE(suffixes.empty());
    for (const std::string& suffix : suffixes) {
        CHECK(!suffix.empty() && suffix.front() == '.');
        for (char c : suffix) {
            CHECK(!(c >= 'A' && c <= 'Z'));
        }
    }
#else
    CHECK(executableSuffixes().empty());
#endif
}

TEST_CASE("platform/pathsEqual compares normalized paths") {
    testfw::TempDir temp;
    fs::create_directories(temp.path() / "a" / "b");

    CHECK(pathsEqual(temp.path() / "a", temp.path() / "a" / "b" / ".."));
    CHECK(pathsEqual(temp.path() / "a", temp.path() / "a"));
    CHECK_FALSE(pathsEqual(temp.path() / "a", temp.path() / "a" / "b"));
    // Missing paths fall back to a lexical comparison.
    CHECK(pathsEqual(temp.path() / "missing" / "x",
                     temp.path() / "missing" / "y" / ".." / "x"));
    CHECK_FALSE(pathsEqual(temp.path() / "missing1", temp.path() / "missing2"));
}

TEST_CASE("platform/pathSortKey uses forward slashes") {
    const std::string key = pathSortKey(fs::path("a") / "b");
    CHECK_EQ(key, "a/b");
}

TEST_CASE("platform/timestamps have a fixed layout") {
    const std::string now = nowTimestamp();
    REQUIRE(now.size() == 19);
    CHECK_EQ(now[4], '-');
    CHECK_EQ(now[7], '-');
    CHECK_EQ(now[10], 'T');
    CHECK_EQ(now[13], ':');
    CHECK_EQ(now[16], ':');

    const std::string log = logTimestamp();
    REQUIRE(log.size() == 23);
    CHECK_EQ(log[10], ' ');
    CHECK_EQ(log[19], '.');
}

TEST_CASE("platform/executablePath returns an absolute existing location") {
    const fs::path path = executablePath(nullptr);
    CHECK(path.is_absolute());
    std::error_code ec;
    CHECK(fs::exists(path, ec));

    const fs::path directory = executableDirectory(nullptr);
    CHECK(fs::is_directory(directory, ec));
}

TEST_CASE("platform/executablePath falls back to argv0 when needed") {
    // On supported platforms the OS answer wins, so argv0 is only a fallback;
    // either way the result must be absolute.
    CHECK(executablePath("relative/claudex").is_absolute());
}
