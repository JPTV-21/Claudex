#include "launcher.hpp"

#include "claude_finder.hpp"
#include "text_util.hpp"

#include <csignal>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <vector>

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

bool ensureDirectory(const fs::path& directory, std::string& error,
                     const MenuTheme& theme) {
    std::error_code ec;
    if (fs::is_directory(directory, ec) && !ec) {
        return true;
    }
    fs::create_directories(directory, ec);
    if (ec) {
        error = messageText(theme, "createFolderFailed",
                            {{"path", pathToUtf8(directory)},
                             {"error", ec.message()}});
        return false;
    }
    if (!fs::is_directory(directory, ec) || ec) {
        error = messageText(theme, "notAFolder",
                            {{"path", pathToUtf8(directory)}});
        return false;
    }
    return true;
}

bool preparePortableDirectories(const ConfigManager& config,
                                fs::path& configDir, fs::path& tempDir,
                                std::string& error) {
    configDir = config.claudeConfigDir();
    tempDir = config.tempDir();

    if (configDir.empty()) {
        error = messageText(config.menuTheme, "pathPatternNoMatch",
                            {{"value", config.settings.claudeConfigDir}});
        return false;
    }
    if (tempDir.empty()) {
        error = messageText(config.menuTheme, "pathPatternNoMatch",
                            {{"value", config.settings.tempDir}});
        return false;
    }

    if (!ensureDirectory(configDir, error, config.menuTheme)) {
        return false;
    }
    if (!ensureDirectory(tempDir, error, config.menuTheme)) {
        return false;
    }
    return true;
}

// Custom project actions keep the old behavior: the launcher exports the
// portable environment for them. Claude launches get it from launchCommand.
bool preparePortableEnvironment(const ConfigManager& config,
                                std::string& error) {
    fs::path configDir;
    fs::path tempDir;
    if (!preparePortableDirectories(config, configDir, tempDir, error)) {
        return false;
    }

    auto setEnvironment = [&](const char* name, const fs::path& value) {
        std::string detail;
        if (!setEnvUtf8(name, pathToUtf8(value), detail)) {
            error = messageText(config.menuTheme, "environmentVariableFailed",
                                {{"name", name}, {"error", detail}});
            return false;
        }
        return true;
    };

    if (!setEnvironment("CLAUDE_CONFIG_DIR", configDir)) {
        return false;
    }
    if (!setEnvironment("TEMP", tempDir)) {
        return false;
    }
    if (!setEnvironment("TMP", tempDir)) {
        return false;
    }
    if (!setEnvironment("TMPDIR", tempDir)) {
        return false;
    }
    return true;
}

#ifdef _WIN32

namespace {

BOOL WINAPI childConsoleControlHandler(DWORD eventType) {
    // The child process has its own handlers. Keep a stray Ctrl+C from also
    // killing the launcher while it is waiting for Claude to exit.
    switch (eventType) {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
            return TRUE;
        default:
            return FALSE;
    }
}

}  // namespace

std::wstring quoteWindowsArgument(const std::wstring& argument) {
    if (argument.empty()) {
        return L"\"\"";
    }
    if (argument.find_first_of(L" \t\n\v\"") == std::wstring::npos) {
        return argument;
    }

    std::wstring result = L"\"";
    std::size_t backslashes = 0;
    for (wchar_t character : argument) {
        if (character == L'\\') {
            ++backslashes;
            continue;
        }
        if (character == L'"') {
            result.append(backslashes * 2 + 1, L'\\');
            result.push_back(L'"');
            backslashes = 0;
            continue;
        }
        result.append(backslashes, L'\\');
        backslashes = 0;
        result.push_back(character);
    }
    result.append(backslashes * 2, L'\\');
    result.push_back(L'"');
    return result;
}

namespace {

std::wstring buildWindowsArguments(const std::vector<std::string>& arguments) {
    std::wstring result;
    for (const std::string& argument : arguments) {
        result += L' ';
        result += quoteWindowsArgument(utf8ToWide(argument));
    }
    return result;
}

std::wstring findCommandInterpreter() {
    if (auto comspec = getEnvUtf8("ComSpec");
        comspec && !comspec->empty()) {
        return utf8ToWide(*comspec);
    }
    if (auto systemRoot = getEnvUtf8("SystemRoot");
        systemRoot && !systemRoot->empty()) {
        fs::path candidate =
            pathFromUtf8(*systemRoot) / "System32" / "cmd.exe";
        if (isRegularFile(candidate)) {
            return candidate.wstring();
        }
    }
    if (auto found = findExecutableOnPath("cmd.exe")) {
        return found->wstring();
    }
    return L"cmd.exe";
}

LaunchResult runWindowsCommandLine(
    const std::wstring& applicationPath, const std::wstring& commandLine,
    const fs::path& workingDirectory, const MenuTheme& theme) {
    LaunchResult result;

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};

    std::wstring mutableCommandLine = commandLine;
    std::wstring workingDirectoryWide = workingDirectory.wstring();

    SetConsoleCtrlHandler(childConsoleControlHandler, TRUE);
    const BOOL created = CreateProcessW(
        applicationPath.c_str(), mutableCommandLine.data(), nullptr, nullptr,
        TRUE, 0, nullptr, workingDirectoryWide.c_str(), &startup, &process);
    const DWORD createError = created ? ERROR_SUCCESS : GetLastError();
    if (!created) {
        SetConsoleCtrlHandler(childConsoleControlHandler, FALSE);
        result.error = messageText(theme, "claudeStartFailed",
                                   {{"error", winErrorMessage(createError)}});
        return result;
    }

    result.started = true;
    const DWORD waitResult =
        WaitForSingleObject(process.hProcess, INFINITE);
    const DWORD waitError = waitResult == WAIT_FAILED ? GetLastError() : 0;
    SetConsoleCtrlHandler(childConsoleControlHandler, FALSE);

    DWORD exitCode = 0;
    if (waitResult == WAIT_FAILED || waitResult == WAIT_ABANDONED) {
        result.error = messageText(theme, "claudeWaitFailed",
                                   {{"error", winErrorMessage(waitError)}});
    } else if (GetExitCodeProcess(process.hProcess, &exitCode)) {
        result.exitCode = static_cast<int>(exitCode);
    } else {
        result.error = messageText(
            theme, "claudeExitCodeUnreadable",
            {{"error", winErrorMessage(GetLastError())}});
    }

    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return result;
}

[[maybe_unused]] LaunchResult runChild(
    const fs::path& executable,
    const std::vector<std::string>& arguments,
    const fs::path& workingDirectory, const MenuTheme& theme) {
    const fs::path applicationPath =
        isBatchFile(executable) ? fs::path(findCommandInterpreter())
                                : executable;
    std::wstring commandLine;
    if (isBatchFile(executable)) {
        commandLine = quoteWindowsArgument(applicationPath.wstring()) +
                      L" /d /s /c \"" +
                      quoteWindowsArgument(executable.wstring()) +
                      buildWindowsArguments(arguments) + L"\"";
    } else {
        commandLine = quoteWindowsArgument(executable.wstring()) +
                      buildWindowsArguments(arguments);
    }

    return runWindowsCommandLine(applicationPath.wstring(), commandLine,
                                 workingDirectory, theme);
}

}  // namespace

LaunchResult runShellCommand(const std::string& command,
                             const fs::path& workingDirectory,
                             const MenuTheme& theme) {
    const std::wstring interpreter = findCommandInterpreter();
    const std::wstring commandLine =
        quoteWindowsArgument(interpreter) + L" /d /s /c \"" +
        utf8ToWide(command) + L"\"";
    return runWindowsCommandLine(interpreter, commandLine, workingDirectory,
                                 theme);
}

#else

namespace {

class ScopedSignalIgnore {
public:
    ScopedSignalIgnore() {
        struct sigaction ignore {};
        ignore.sa_handler = SIG_IGN;
        sigemptyset(&ignore.sa_mask);
        ignore.sa_flags = 0;
        if (sigaction(SIGINT, &ignore, &oldInt_) == 0) {
            haveOldInt_ = true;
        }
        if (sigaction(SIGQUIT, &ignore, &oldQuit_) == 0) {
            haveOldQuit_ = true;
        }
    }

    ~ScopedSignalIgnore() {
        if (haveOldInt_) {
            sigaction(SIGINT, &oldInt_, nullptr);
        }
        if (haveOldQuit_) {
            sigaction(SIGQUIT, &oldQuit_, nullptr);
        }
    }

    ScopedSignalIgnore(const ScopedSignalIgnore&) = delete;
    ScopedSignalIgnore& operator=(const ScopedSignalIgnore&) = delete;

private:
    struct sigaction oldInt_ {};
    struct sigaction oldQuit_ {};
    bool haveOldInt_ = false;
    bool haveOldQuit_ = false;
};

LaunchResult runChild(const fs::path& executable,
                      const std::vector<std::string>& arguments,
                      const fs::path& workingDirectory,
                      const MenuTheme& theme) {
    LaunchResult result;

    std::vector<std::string> argumentStorage;
    argumentStorage.reserve(arguments.size() + 1);
    argumentStorage.push_back(pathToUtf8(executable));
    argumentStorage.insert(argumentStorage.end(), arguments.begin(),
                           arguments.end());

    std::vector<char*> argv;
    argv.reserve(argumentStorage.size() + 1);
    for (std::string& argument : argumentStorage) {
        argv.push_back(argument.data());
    }
    argv.push_back(nullptr);

    std::cout.flush();
    std::cerr.flush();

    // Ctrl+C belongs to Claude, not to the waiting launcher. Ignore it in the
    // parent before forking so there is no window in which it can kill us.
    ScopedSignalIgnore ignoreSignals;

    const pid_t pid = fork();
    if (pid < 0) {
        result.error = messageText(theme, "claudeStartFailed",
                                   {{"error", std::strerror(errno)}});
        return result;
    }
    if (pid == 0) {
        // Reset the inherited ignores so Claude can receive Ctrl+C/Ctrl+\.
        struct sigaction defaultAction {};
        defaultAction.sa_handler = SIG_DFL;
        sigemptyset(&defaultAction.sa_mask);
        sigaction(SIGINT, &defaultAction, nullptr);
        sigaction(SIGQUIT, &defaultAction, nullptr);

        if (chdir(workingDirectory.c_str()) != 0) {
            std::fprintf(stderr, "Could not enter %s: %s\n",
                         workingDirectory.c_str(), std::strerror(errno));
            _exit(126);
        }
        execv(executable.c_str(), argv.data());
        std::fprintf(stderr, "Could not execute %s: %s\n",
                     executable.c_str(), std::strerror(errno));
        _exit(127);
    }

    result.started = true;
    int status = 0;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno == EINTR) {
            continue;
        }
        result.error = messageText(theme, "claudeWaitFailed",
                                   {{"error", std::strerror(errno)}});
        break;
    }

    if (WIFEXITED(status)) {
        result.exitCode = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        result.exitCode = 128 + WTERMSIG(status);
    }
    return result;
}

}  // namespace

LaunchResult runShellCommand(const std::string& command,
                             const fs::path& workingDirectory,
                             const MenuTheme& theme) {
    return runChild(pathFromUtf8("/bin/sh"), {"-c", command},
                    workingDirectory, theme);
}

#endif

LaunchResult launchClaude(const ConfigManager& config,
                          const fs::path& projectDirectory) {
    LaunchResult result;

    std::error_code ec;
    if (!fs::is_directory(projectDirectory, ec) || ec) {
        result.error = messageText(
            config.menuTheme, "claudeProjectMissing",
            {{"path", pathToUtf8(projectDirectory)}});
        config.log("ERROR", "launch: " + result.error);
        return result;
    }

    std::string command = config.settings.launchCommand.current();

    fs::path configDir;
    fs::path tempDir;
    std::string environmentError;
    if (!preparePortableDirectories(config, configDir, tempDir,
                                    environmentError)) {
        result.error = environmentError;
        config.log("ERROR", "launch: " + result.error);
        return result;
    }

    command = config.expandLaunchCommand(std::move(command), configDir,
                                         tempDir);

    std::optional<fs::path> executable;
    if (command.find("{claude}") != std::string::npos) {
        std::string resolveError;
        executable = config.findClaudeExecutable(resolveError);
        if (!executable) {
            result.error = resolveError;
            config.log("ERROR", "launch: " + result.error);
            return result;
        }
        replaceAll(command, "{claude}",
                   quoteShellArgument(pathToUtf8(*executable)));
    }

    std::string executableLog;
    if (executable) {
        executableLog = " exe=" + pathToUtf8(*executable);
    }
    config.log("INFO", "launch: cwd=" + pathToUtf8(projectDirectory) +
                           " command=\"" + command + "\"" + executableLog +
                           " claudeConfigDir=" + pathToUtf8(configDir) +
                           " tempDir=" + pathToUtf8(tempDir));
    result = runShellCommand(command, projectDirectory, config.menuTheme);

    if (!result.started) {
        config.log("ERROR", "launch failed: " + result.error);
    } else if (!result.error.empty()) {
        config.log("ERROR", "launch process error: " + result.error);
    } else {
        config.log("INFO", "launch: Claude exited with code " +
                               std::to_string(result.exitCode));
    }
    return result;
}

// Runs a project action's custom shell command. Unlike Claude launches, custom
// actions still receive the portable environment from the launcher.
LaunchResult runCustomProjectCommand(const ConfigManager& config,
                                     const fs::path& projectDirectory,
                                     const std::string& command) {
    LaunchResult result;

    std::error_code ec;
    if (!fs::is_directory(projectDirectory, ec) || ec) {
        result.error = messageText(
            config.menuTheme, "claudeProjectMissing",
            {{"path", pathToUtf8(projectDirectory)}});
        config.log("ERROR", "custom action: " + result.error);
        return result;
    }

    std::string environmentError;
    if (!preparePortableEnvironment(config, environmentError)) {
        result.error = environmentError;
        config.log("ERROR", "custom action: " + result.error);
        return result;
    }

    config.log("INFO", "custom action: command=\"" + command + "\" cwd=" +
                           pathToUtf8(projectDirectory) +
                           " claudeConfigDir=" +
                           pathToUtf8(config.claudeConfigDir()) +
                           " tempDir=" + pathToUtf8(config.tempDir()));
    result = runShellCommand(command, projectDirectory, config.menuTheme);

    if (!result.started) {
        config.log("ERROR", "custom action failed: " + result.error);
    } else if (!result.error.empty()) {
        config.log("ERROR", "custom action process error: " + result.error);
    } else {
        config.log("INFO", "custom action: command exited with code " +
                               std::to_string(result.exitCode));
    }
    return result;
}

void clearScreen() {
#ifdef _WIN32
    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    if (output == nullptr || output == INVALID_HANDLE_VALUE) {
        return;
    }

    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (!GetConsoleScreenBufferInfo(output, &info)) {
        return;
    }

    const COORD home = {0, 0};
    const DWORD cellCount = static_cast<DWORD>(info.dwSize.X) *
                            static_cast<DWORD>(info.dwSize.Y);
    DWORD written = 0;
    FillConsoleOutputCharacterW(output, L' ', cellCount, home, &written);
    FillConsoleOutputAttribute(output, info.wAttributes, cellCount, home,
                               &written);
    SetConsoleCursorPosition(output, home);

    DWORD mode = 0;
    if (GetConsoleMode(output, &mode) &&
        (mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0) {
        // Also drop scrollback when the terminal supports it.
        std::cout << "\033[3J";
    }
    std::cout.flush();
#else
    if (!isatty(STDOUT_FILENO)) {
        return;
    }
    std::cout << "\033[H\033[2J\033[3J" << std::flush;
#endif
}

}  // namespace claudex
