#include "test_framework.hpp"

#include "config_bootstrap.hpp"
#include "config_types.hpp"

using namespace claudex;
using testfw::TempDir;
using testfw::writeFile;

namespace {

fs::path configPath(const TempDir& root) {
    return root.path() / kDefaultConfigFile;
}

std::string configWithTarget(const json& target) {
    json document;
    document["configFile"] = target;
    return document.dump(2);
}

}  // namespace

TEST_CASE("bootstrap/missing config stays on the initial path") {
    TempDir root;

    const ConfigBootstrapResult result = bootstrapConfigPath(
        root.path(), configPath(root), PathResolver(root.path()));

    CHECK(pathsEqual(result.configFile, configPath(root)));
    CHECK_FALSE(result.redirected);
    CHECK_FALSE(result.targetMissing);
    CHECK(result.messageKey.empty());
}

TEST_CASE("bootstrap/missing or empty configFile does not redirect") {
    TempDir root;
    writeFile(configPath(root), "{\"version\": 1}");

    ConfigBootstrapResult result = bootstrapConfigPath(
        root.path(), configPath(root), PathResolver(root.path()));
    CHECK(pathsEqual(result.configFile, configPath(root)));
    CHECK_FALSE(result.redirected);

    writeFile(configPath(root), configWithTarget("   "));
    result = bootstrapConfigPath(root.path(), configPath(root),
                                 PathResolver(root.path()));
    CHECK(pathsEqual(result.configFile, configPath(root)));
    CHECK_FALSE(result.redirected);
}

TEST_CASE("bootstrap/a target equal to the initial file does nothing") {
    TempDir root;
    writeFile(configPath(root), configWithTarget("claudex-config.json"));

    const ConfigBootstrapResult result = bootstrapConfigPath(
        root.path(), configPath(root), PathResolver(root.path()));

    CHECK(pathsEqual(result.configFile, configPath(root)));
    CHECK_FALSE(result.redirected);
}

TEST_CASE("bootstrap/redirects to an existing config") {
    TempDir root;
    const fs::path moved = root.path() / "moved.json";
    writeFile(moved, "{}");
    writeFile(configPath(root), configWithTarget("moved.json"));

    const ConfigBootstrapResult result = bootstrapConfigPath(
        root.path(), configPath(root), PathResolver(root.path()));

    CHECK(result.redirected);
    CHECK(pathsEqual(result.configFile, moved));
    CHECK_FALSE(result.targetMissing);
    CHECK(result.messageKey.empty());
}

TEST_CASE("bootstrap/a missing target is reported but still selected") {
    TempDir root;
    writeFile(configPath(root), configWithTarget("moved.json"));

    const ConfigBootstrapResult result = bootstrapConfigPath(
        root.path(), configPath(root), PathResolver(root.path()));

    CHECK(result.redirected);
    CHECK(pathsEqual(result.configFile, root.path() / "moved.json"));
    CHECK(result.targetMissing);
    CHECK_EQ(result.messageKey, "configRedirectTargetMissing");
    // The launcher never creates the target itself.
    CHECK_FALSE(fs::exists(root.path() / "moved.json"));
}

TEST_CASE("bootstrap/relative targets resolve from the launcher folder") {
    TempDir root;
    writeFile(configPath(root), configWithTarget("sub/config.json"));

    const ConfigBootstrapResult result = bootstrapConfigPath(
        root.path(), configPath(root), PathResolver(root.path()));

    CHECK(result.redirected);
    CHECK(pathsEqual(result.configFile,
                     root.path() / "sub" / "config.json"));
}

TEST_CASE("bootstrap/targets support path templates") {
    TempDir root;
    writeFile(configPath(root), configWithTarget("{root}/moved/config.json"));

    const ConfigBootstrapResult result = bootstrapConfigPath(
        root.path(), configPath(root), PathResolver(root.path()));

    CHECK(result.redirected);
    CHECK(pathsEqual(result.configFile,
                     root.path() / "moved" / "config.json"));
}

TEST_CASE("bootstrap/an explicit config also follows its configFile") {
    TempDir root;
    const fs::path explicitFile = root.path() / "explicit.json";
    const fs::path moved = root.path() / "moved.json";
    writeFile(moved, "{}");
    writeFile(explicitFile, configWithTarget("moved.json"));

    const ConfigBootstrapResult result = bootstrapConfigPath(
        root.path(), explicitFile, PathResolver(root.path()));

    CHECK(result.redirected);
    CHECK(pathsEqual(result.configFile, moved));
}

TEST_CASE("bootstrap/a BOM before the JSON is accepted") {
    TempDir root;
    writeFile(configPath(root),
              std::string("\xEF\xBB\xBF") + configWithTarget("moved.json"));

    const ConfigBootstrapResult result = bootstrapConfigPath(
        root.path(), configPath(root), PathResolver(root.path()));

    CHECK(result.redirected);
}

TEST_CASE("bootstrap/unusable configs are left to ConfigManager") {
    TempDir root;

    writeFile(configPath(root), "{ not json");
    ConfigBootstrapResult result = bootstrapConfigPath(
        root.path(), configPath(root), PathResolver(root.path()));
    CHECK_FALSE(result.redirected);
    CHECK(pathsEqual(result.configFile, configPath(root)));

    writeFile(configPath(root), "[]");
    result = bootstrapConfigPath(root.path(), configPath(root),
                                 PathResolver(root.path()));
    CHECK_FALSE(result.redirected);

    writeFile(configPath(root), configWithTarget(json(123)));
    result = bootstrapConfigPath(root.path(), configPath(root),
                                 PathResolver(root.path()));
    CHECK_FALSE(result.redirected);
}
