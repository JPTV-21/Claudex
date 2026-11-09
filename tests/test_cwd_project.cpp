#include "test_framework.hpp"

#include "cwd_project.hpp"
#include "platform.hpp"

#include <algorithm>
#include <cctype>

using namespace claudex;
using testfw::TempDir;

namespace {

// Restores the process working directory when the test ends, so a failing
// REQUIRE cannot leave later tests running from a removed TempDir.
class ScopedCurrentPath {
public:
    explicit ScopedCurrentPath(const fs::path& path) {
        std::error_code ec;
        previous_ = fs::current_path(ec);
        fs::current_path(path, ec);
    }
    ScopedCurrentPath(const ScopedCurrentPath&) = delete;
    ScopedCurrentPath& operator=(const ScopedCurrentPath&) = delete;
    ~ScopedCurrentPath() {
        if (previous_.empty()) {
            return;
        }
        std::error_code ec;
        fs::current_path(previous_, ec);
    }

private:
    fs::path previous_;
};

bool statusIs(const CwdProjectInfo& info, CwdProjectStatus status) {
    return info.status == status;
}

}  // namespace

TEST_CASE("cwd_project/hides the launcher folder") {
    TempDir root;
    const CwdProjectInfo info = resolveCwdProject(root.path(), root.path());
    CHECK(statusIs(info, CwdProjectStatus::SameAsLauncher));
    CHECK_FALSE(info.visible());
    CHECK(pathsEqual(info.path, root.path()));
}

TEST_CASE("cwd_project/shows a different existing directory") {
    TempDir root;
    const fs::path work = root.path() / "work";
    std::error_code ec;
    fs::create_directories(work, ec);
    REQUIRE(!ec);

    const CwdProjectInfo info = resolveCwdProject(root.path(), work);
    CHECK(statusIs(info, CwdProjectStatus::Visible));
    CHECK(info.visible());
    CHECK(pathsEqual(info.path, work));
}

TEST_CASE("cwd_project/hides missing paths and files") {
    TempDir root;

    const CwdProjectInfo missing =
        resolveCwdProject(root.path(), root.path() / "missing");
    CHECK(statusIs(missing, CwdProjectStatus::Unavailable));
    CHECK(missing.path.empty());
    CHECK_FALSE(missing.detail.empty());

    const fs::path file = root.path() / "file.txt";
    testfw::writeFile(file, "data");
    const CwdProjectInfo notFolder = resolveCwdProject(root.path(), file);
    CHECK(statusIs(notFolder, CwdProjectStatus::Unavailable));
    CHECK(notFolder.path.empty());
    CHECK_FALSE(notFolder.detail.empty());
}

TEST_CASE("cwd_project/treats a lexical alias of the launcher folder as the "
          "same directory") {
    TempDir root;
    const fs::path sub = root.path() / "sub";
    std::error_code ec;
    fs::create_directories(sub, ec);
    REQUIRE(!ec);

    const CwdProjectInfo info =
        resolveCwdProject(root.path(), sub / "..");
    CHECK(statusIs(info, CwdProjectStatus::SameAsLauncher));
    CHECK_FALSE(info.visible());
}

TEST_CASE("cwd_project/reports a current_path error") {
    TempDir root;
    const std::error_code error =
        std::make_error_code(std::errc::permission_denied);
    const CwdProjectInfo info =
        resolveCwdProject(root.path(), root.path(), error);
    CHECK(statusIs(info, CwdProjectStatus::Unavailable));
    CHECK_FALSE(info.detail.empty());
}

TEST_CASE("cwd_project/detectCwdProject reads the process directory") {
    TempDir launcher;
    TempDir work;
    {
        const ScopedCurrentPath cwd(work.path());
        const CwdProjectInfo info = detectCwdProject(launcher.path());
        CHECK(statusIs(info, CwdProjectStatus::Visible));
        CHECK(pathsEqual(info.path, work.path()));
    }
    {
        const ScopedCurrentPath cwd(launcher.path());
        const CwdProjectInfo info = detectCwdProject(launcher.path());
        CHECK(statusIs(info, CwdProjectStatus::SameAsLauncher));
    }
}

#ifdef _WIN32
TEST_CASE("cwd_project/matches the launcher folder across case differences") {
    TempDir root;
    const fs::path work = root.path() / "MixedCase";
    std::error_code ec;
    fs::create_directories(work, ec);
    REQUIRE(!ec);

    std::string text = genericPathToUtf8(work);
    const bool ascii = std::all_of(text.begin(), text.end(), [](char character) {
        return static_cast<unsigned char>(character) < 0x80;
    });
    if (!ascii) {
        return;  // Case folding non-ASCII text is locale-dependent.
    }
    std::transform(text.begin(), text.end(), text.begin(), [](char character) {
        return static_cast<char>(
            std::tolower(static_cast<unsigned char>(character)));
    });

    const CwdProjectInfo info =
        resolveCwdProject(work, pathFromUtf8(text));
    CHECK(statusIs(info, CwdProjectStatus::SameAsLauncher));
}
#endif
