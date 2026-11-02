#include "platform.hpp"

#include "text_util.hpp"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#endif

namespace claudex {

#ifdef _WIN32
std::string wideToUtf8(const std::wstring& value) {
    if (value.empty()) {
        return {};
    }
    int size = WideCharToMultiByte(CP_UTF8, 0, value.data(),
                                   static_cast<int>(value.size()), nullptr, 0,
                                   nullptr, nullptr);
    if (size <= 0) {
        return {};
    }
    std::string result(static_cast<std::size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(),
                        static_cast<int>(value.size()), result.data(), size,
                        nullptr, nullptr);
    return result;
}

std::wstring utf8ToWide(const std::string& value) {
    if (value.empty()) {
        return {};
    }
    int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
                                   static_cast<int>(value.size()), nullptr, 0);
    if (size <= 0) {
        // A few Windows tools still emit the active ANSI code page. Accept it
        // as a fallback instead of turning the path into an empty string.
        size = MultiByteToWideChar(CP_ACP, 0, value.data(),
                                   static_cast<int>(value.size()), nullptr, 0);
    }
    if (size <= 0) {
        return {};
    }
    std::wstring result(static_cast<std::size_t>(size), L'\0');
    UINT codePage = CP_UTF8;
    DWORD flags = MB_ERR_INVALID_CHARS;
    if (MultiByteToWideChar(codePage, flags, value.data(),
                            static_cast<int>(value.size()), result.data(),
                            size) <= 0) {
        codePage = CP_ACP;
        flags = 0;
        MultiByteToWideChar(codePage, flags, value.data(),
                            static_cast<int>(value.size()), result.data(), size);
    }
    return result;
}

fs::path pathFromUtf8(const std::string& value) {
    return fs::path(utf8ToWide(value));
}

std::string pathToUtf8(const fs::path& value) {
    return wideToUtf8(value.wstring());
}

std::string genericPathToUtf8(const fs::path& value) {
    return wideToUtf8(value.generic_wstring());
}

std::string winErrorMessage(DWORD code) {
    wchar_t* buffer = nullptr;
    DWORD size = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<wchar_t*>(&buffer), 0, nullptr);
    if (size == 0 || buffer == nullptr) {
        return "Windows error " + std::to_string(code);
    }
    std::wstring message(buffer, size);
    LocalFree(buffer);
    while (!message.empty() &&
           (message.back() == L'\r' || message.back() == L'\n' ||
            message.back() == L' ')) {
        message.pop_back();
    }
    return wideToUtf8(message);
}
#else
fs::path pathFromUtf8(const std::string& value) {
    return fs::path(value);
}

std::string pathToUtf8(const fs::path& value) {
    return value.string();
}

std::string genericPathToUtf8(const fs::path& value) {
    return value.generic_string();
}
#endif

std::optional<std::string> getEnvUtf8(const std::string& name) {
#ifdef _WIN32
    std::wstring wideName = utf8ToWide(name);
    DWORD needed = GetEnvironmentVariableW(wideName.c_str(), nullptr, 0);
    if (needed == 0) {
        return std::nullopt;
    }
    std::wstring value(needed, L'\0');
    DWORD written =
        GetEnvironmentVariableW(wideName.c_str(), value.data(), needed);
    if (written == 0) {
        return std::nullopt;
    }
    value.resize(written);
    return wideToUtf8(value);
#else
    const char* value = std::getenv(name.c_str());
    if (value == nullptr) {
        return std::nullopt;
    }
    return std::string(value);
#endif
}

bool setEnvUtf8(const std::string& name, const std::string& value,
                std::string& detail) {
#ifdef _WIN32
    if (!SetEnvironmentVariableW(utf8ToWide(name).c_str(),
                                 utf8ToWide(value).c_str())) {
        detail = winErrorMessage(GetLastError());
        return false;
    }
    return true;
#else
    if (setenv(name.c_str(), value.c_str(), 1) != 0) {
        detail = std::strerror(errno);
        return false;
    }
    return true;
#endif
}

std::string homeDirectory() {
    if (auto home = getEnvUtf8("HOME"); home && !home->empty()) {
        return *home;
    }
#ifdef _WIN32
    if (auto profile = getEnvUtf8("USERPROFILE");
        profile && !profile->empty()) {
        return *profile;
    }
    auto drive = getEnvUtf8("HOMEDRIVE");
    auto path = getEnvUtf8("HOMEPATH");
    if (drive && path) {
        return *drive + *path;
    }
#endif
    return {};
}

std::string platformName() {
#ifdef _WIN32
    return "windows";
#elif defined(__APPLE__)
    return "macos";
#else
    return "linux";
#endif
}

std::string archName() {
#ifdef _WIN32
    SYSTEM_INFO info{};
    GetNativeSystemInfo(&info);
    switch (info.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_AMD64:
            return "x86_64";
        case PROCESSOR_ARCHITECTURE_ARM64:
            return "arm64";
        case PROCESSOR_ARCHITECTURE_INTEL:
            return "x86";
        default:
            break;
    }
#endif
#if defined(__x86_64__) || defined(_M_X64)
    return "x86_64";
#elif defined(__aarch64__) || defined(_M_ARM64) || defined(__arm64__)
    return "arm64";
#elif defined(__i386__) || defined(_M_IX86)
    return "x86";
#else
    return sizeof(void*) == 8 ? "64-bit" : "32-bit";
#endif
}

// Windows decides what counts as an executable through PATHEXT. Reading it
// instead of hardcoding .exe/.cmd/.bat keeps the launcher usable with custom
// or stripped-down environments.
const std::vector<std::string>& executableSuffixes() {
#ifdef _WIN32
    static const std::vector<std::string> suffixes = [] {
        std::vector<std::string> values;
        if (auto pathExt = getEnvUtf8("PATHEXT");
            pathExt && !pathExt->empty()) {
            std::size_t start = 0;
            while (start <= pathExt->size()) {
                std::size_t end = pathExt->find(';', start);
                if (end == std::string::npos) {
                    end = pathExt->size();
                }
                std::string suffix = trim(pathExt->substr(start, end - start));
                if (!suffix.empty()) {
                    if (suffix.front() != '.') {
                        suffix.insert(suffix.begin(), '.');
                    }
                    suffix = lowerAscii(std::move(suffix));
                    if (std::find(values.begin(), values.end(), suffix) ==
                        values.end()) {
                        values.push_back(std::move(suffix));
                    }
                }
                if (end == pathExt->size()) {
                    break;
                }
                start = end + 1;
            }
        }
        if (values.empty()) {
            // Fallback only for environments that do not define PATHEXT.
            values = {".com", ".exe", ".bat", ".cmd"};
        }
        return values;
    }();
    return suffixes;
#else
    static const std::vector<std::string> suffixes;
    return suffixes;
#endif
}

std::string expandEnvironmentVariables(std::string text) {
    std::string output;
    output.reserve(text.size());

    for (std::size_t i = 0; i < text.size();) {
        if (text[i] == '%') {
            std::size_t end = text.find('%', i + 1);
            if (end != std::string::npos && end > i + 1) {
                std::string name = text.substr(i + 1, end - i - 1);
                bool valid = !name.empty() &&
                             std::all_of(name.begin(), name.end(), [](char c) {
                                 return std::isalnum(static_cast<unsigned char>(c)) ||
                                        c == '_';
                             });
                if (valid) {
                    if (auto value = getEnvUtf8(name)) {
                        output += *value;
                        i = end + 1;
                        continue;
                    }
                }
            }
        } else if (text[i] == '$' && i + 1 < text.size()) {
            std::size_t nameStart = i + 1;
            std::size_t nameEnd = nameStart;
            if (text[nameStart] == '{') {
                std::size_t close = text.find('}', nameStart + 1);
                if (close != std::string::npos) {
                    std::string name =
                        text.substr(nameStart + 1, close - nameStart - 1);
                    if (!name.empty()) {
                        if (auto value = getEnvUtf8(name)) {
                            output += *value;
                            i = close + 1;
                            continue;
                        }
                    }
                }
            } else if (std::isalpha(static_cast<unsigned char>(text[nameStart])) ||
                       text[nameStart] == '_') {
                while (nameEnd < text.size() &&
                       (std::isalnum(static_cast<unsigned char>(text[nameEnd])) ||
                        text[nameEnd] == '_')) {
                    ++nameEnd;
                }
                std::string name = text.substr(nameStart, nameEnd - nameStart);
                if (auto value = getEnvUtf8(name)) {
                    output += *value;
                    i = nameEnd;
                    continue;
                }
            }
        }
        output += text[i];
        ++i;
    }
    return output;
}

#ifndef _WIN32
// Mount point of the filesystem that contains |path|. Walking up while the
// device id stays the same finds the volume root, for example /Volumes/Disk
// on macOS or /home on a separate Linux partition.
std::string volumeMountPoint(const fs::path& path) {
    std::error_code ec;
    fs::path current = fs::weakly_canonical(path, ec);
    if (ec) {
        current = path.lexically_normal();
    }

    struct stat info {};
    if (::stat(current.c_str(), &info) != 0) {
        return "/";
    }
    const dev_t device = info.st_dev;

    while (true) {
        const fs::path parent = current.parent_path();
        if (parent.empty() || parent == current) {
            return pathToUtf8(current);
        }

        struct stat parentInfo {};
        if (::stat(parent.c_str(), &parentInfo) != 0) {
            return pathToUtf8(current);
        }
        if (parentInfo.st_dev != device) {
            return pathToUtf8(current);
        }
        current = parent;
    }
}
#endif

std::string pathSortKey(const fs::path& path) {
    std::string key = genericPathToUtf8(path);
#ifdef _WIN32
    key = lowerAscii(std::move(key));
#endif
    return key;
}

bool pathsEqual(const fs::path& lhs, const fs::path& rhs) {
    std::error_code ec;
    if (fs::equivalent(lhs, rhs, ec) && !ec) {
        return true;
    }

    std::string left = genericPathToUtf8(lhs.lexically_normal());
    std::string right = genericPathToUtf8(rhs.lexically_normal());
#ifdef _WIN32
    left = lowerAscii(std::move(left));
    right = lowerAscii(std::move(right));
#endif
    return left == right;
}

std::string nowTimestamp() {
    const std::time_t now = std::time(nullptr);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    char buffer[32] = {};
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", &local);
    return std::string(buffer);
}

std::string logTimestamp() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t seconds =
        std::chrono::system_clock::to_time_t(now);
    const auto milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) %
        1000;

    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &seconds);
#else
    localtime_r(&seconds, &local);
#endif
    char buffer[32] = {};
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &local);

    char result[40] = {};
    std::snprintf(result, sizeof(result), "%s.%03lld", buffer,
                  static_cast<long long>(milliseconds.count()));
    return std::string(result);
}

fs::path executablePath(const char* argv0) {
    fs::path executable;
#ifdef _WIN32
    std::wstring buffer(1024, L'\0');
    for (;;) {
        DWORD length = GetModuleFileNameW(
            nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0) {
            break;
        }
        if (length < buffer.size() - 1) {
            buffer.resize(length);
            executable = fs::path(buffer);
            break;
        }
        buffer.resize(buffer.size() * 2);
    }
#elif defined(__APPLE__)
    std::vector<char> buffer(1024);
    for (;;) {
        uint32_t size = static_cast<uint32_t>(buffer.size());
        if (_NSGetExecutablePath(buffer.data(), &size) == 0) {
            executable = fs::path(std::string(buffer.data()));
            break;
        }
        buffer.resize(std::max<std::size_t>(size + 1, buffer.size() * 2));
    }
#else
    std::vector<char> buffer(1024);
    for (;;) {
        ssize_t length =
            readlink("/proc/self/exe", buffer.data(), buffer.size() - 1);
        if (length < 0) {
            break;
        }
        if (static_cast<std::size_t>(length) < buffer.size() - 1) {
            buffer[static_cast<std::size_t>(length)] = '\0';
            executable = fs::path(std::string(buffer.data()));
            break;
        }
        buffer.resize(buffer.size() * 2);
    }
#endif

    if (executable.empty() && argv0 != nullptr && argv0[0] != '\0') {
        executable = pathFromUtf8(argv0);
    }
    if (executable.empty()) {
        executable = fs::current_path();
    }

    std::error_code ec;
    if (executable.is_relative()) {
        executable = fs::absolute(executable, ec);
        if (ec) {
            executable = fs::current_path();
        }
    }
    fs::path canonical = fs::weakly_canonical(executable, ec);
    if (!ec) {
        executable = std::move(canonical);
    }
    return executable;
}

fs::path executableDirectory(const char* argv0) {
    const fs::path executable = executablePath(argv0);
    std::error_code ec;
    if (fs::is_directory(executable, ec) && !ec) {
        return executable;
    }
    fs::path parent = executable.parent_path();
    return parent.empty() ? fs::current_path() : parent;
}

}  // namespace claudex
