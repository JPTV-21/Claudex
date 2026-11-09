#include "cwd_project.hpp"

namespace claudex {

namespace {

// Canonical when possible, otherwise an absolute lexical path. The caller only
// uses this for comparison and for launching, so a missing path is not an
// error here; is_directory() reports it separately.
fs::path normalizeDirectory(const fs::path& path) {
    std::error_code ec;
    fs::path canonical = fs::weakly_canonical(path, ec);
    if (!ec && !canonical.empty()) {
        return canonical.lexically_normal();
    }
    ec.clear();
    fs::path absolute = fs::absolute(path, ec);
    if (!ec && !absolute.empty()) {
        return absolute.lexically_normal();
    }
    return path.lexically_normal();
}

}  // namespace

CwdProjectInfo resolveCwdProject(const fs::path& executableDir,
                                 const fs::path& cwd,
                                 std::error_code cwdError) {
    CwdProjectInfo info;
    if (cwdError) {
        info.detail = cwdError.message();
        return info;
    }
    if (cwd.empty()) {
        info.detail = "working directory is empty";
        return info;
    }

    const fs::path resolved = normalizeDirectory(cwd);
    std::error_code ec;
    const bool directory = fs::is_directory(resolved, ec);
    if (ec) {
        info.detail = ec.message();
        return info;
    }
    if (!directory) {
        info.detail = "working directory is not a folder";
        return info;
    }

    info.path = resolved;
    const fs::path launcher = normalizeDirectory(executableDir);
    if (pathsEqual(resolved, launcher)) {
        info.status = CwdProjectStatus::SameAsLauncher;
        info.detail = "working directory equals the launcher folder";
    } else {
        info.status = CwdProjectStatus::Visible;
    }
    return info;
}

CwdProjectInfo detectCwdProject(const fs::path& executableDir) {
    std::error_code ec;
    const fs::path cwd = fs::current_path(ec);
    return resolveCwdProject(executableDir, cwd, ec);
}

}  // namespace claudex
