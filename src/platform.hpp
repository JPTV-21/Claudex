#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace claudex {

namespace fs = std::filesystem;

#ifdef _WIN32
std::string wideToUtf8(const std::wstring& value);
std::wstring utf8ToWide(const std::string& value);
std::string winErrorMessage(unsigned long code);
#endif

fs::path pathFromUtf8(const std::string& value);
std::string pathToUtf8(const fs::path& value);
std::string genericPathToUtf8(const fs::path& value);

std::optional<std::string> getEnvUtf8(const std::string& name);
bool setEnvUtf8(const std::string& name, const std::string& value,
                std::string& detail);

std::string homeDirectory();
std::string platformName();
std::string archName();

// Windows reads PATHEXT; other platforms return an empty list.
const std::vector<std::string>& executableSuffixes();

// Expands %NAME%, $NAME and ${NAME}. Unknown variables are left untouched.
std::string expandEnvironmentVariables(std::string text);

#ifndef _WIN32
std::string volumeMountPoint(const fs::path& path);
#endif

std::string pathSortKey(const fs::path& path);
bool pathsEqual(const fs::path& lhs, const fs::path& rhs);

std::string nowTimestamp();
std::string logTimestamp();

fs::path executablePath(const char* argv0);
fs::path executableDirectory(const char* argv0);

}  // namespace claudex
