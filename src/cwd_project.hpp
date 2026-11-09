#pragma once

#include "platform.hpp"

#include <string>
#include <system_error>

namespace claudex {

// Why the process working directory is (or is not) offered as a project.
enum class CwdProjectStatus {
    Visible,         // A real directory other than the launcher folder.
    SameAsLauncher,  // cwd == executableDir, so the section is hidden.
    Unavailable,     // current_path or the directory check failed.
};

struct CwdProjectInfo {
    fs::path path;  // Normalized absolute path; empty when Unavailable.
    CwdProjectStatus status = CwdProjectStatus::Unavailable;
    std::string detail;  // Diagnostic text for the log only.

    bool visible() const { return status == CwdProjectStatus::Visible; }
};

// Reads the process working directory and classifies it.
CwdProjectInfo detectCwdProject(const fs::path& executableDir);

// Classifies an explicit cwd. The error is the one returned by
// fs::current_path; passing it in keeps the detection testable.
CwdProjectInfo resolveCwdProject(const fs::path& executableDir,
                                 const fs::path& cwd,
                                 std::error_code cwdError = {});

}  // namespace claudex
