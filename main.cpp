#include "claude_finder.hpp"
#include "config_bootstrap.hpp"
#include "config_json.hpp"
#include "config_manager.hpp"
#include "cwd_project.hpp"
#include "default_focus.hpp"
#include "dialog_buttons.hpp"
#include "focus_order.hpp"
#include "grid_menu.hpp"
#include "key_events.hpp"
#include "launcher.hpp"
#include "menu_markup.hpp"
#include "path_resolver.hpp"
#include "platform.hpp"
#include "recent_list.hpp"
#include "screens.hpp"
#include "scroll_bar.hpp"
#include "text_edit.hpp"
#include "text_util.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/terminal.hpp>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace ftxui;
using namespace claudex;

#if defined(__MINGW32__)
// MinGW's startup code can glob wildcard arguments before main(). The
// launcher expands wildcards itself, so keep argv exactly as typed.
int _dowildcard = 0;
#endif

int main(int argc, char** argv) {
    const fs::path executableDir = executableDirectory(
        argc > 0 ? argv[0] : nullptr);
    const PathResolver pathResolver(executableDir);
    auto resolveConfigPath = [&](const std::string& raw) {
        std::error_code ec;
        fs::path cwd = fs::current_path(ec);
        if (ec) {
            cwd = executableDir;
        }
        PathResolution resolution = pathResolver.resolveWithBase(
            raw, cwd, PathKind::File, MultiMatchPolicy::FirstSorted);
        if (resolution.resolved) {
            return resolution.path;
        }
        // Keep the literal pattern so the config load error names it.
        fs::path literal = pathFromUtf8(resolution.expanded);
        if (literal.is_relative()) {
            literal = fs::absolute(literal, ec);
        }
        return literal;
    };

    fs::path explicitConfigFile;
    bool whereClaude = false;
    if (auto environmentConfig = getEnvUtf8("CLAUDEX_CONFIG");
        environmentConfig && !environmentConfig->empty()) {
        explicitConfigFile = resolveConfigPath(*environmentConfig);
    }

    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--config" || argument == "-c") {
            if (i + 1 >= argc) {
                std::cerr << "Missing value after " << argument << ".\n";
                return 2;
            }
            explicitConfigFile = resolveConfigPath(argv[++i]);
        } else if (argument == "--where-claude") {
            whereClaude = true;
        } else if (argument == "--help" || argument == "-h") {
            std::cout
                << "ClaudeX - portable Claude Code launcher\n\n"
                << "Usage: " << (argc > 0 ? argv[0] : "ClaudeX")
                << " [--config PATH] [--where-claude]\n\n"
                << "Controls (defaults; rebind in claudex-theme.json):\n"
                << "  Up/Down     move inside a section; at its edge, switch "
                   "sections\n"
                << "  Enter       launch in the selected project, or start in "
                   "the typed quick-launch folder once\n"
                << "  E / F2      open the selected project's action menu\n"
                << "  Delete      remove the selected project\n"
                << "  A           add a project\n"
                << "  Tab         switch sections in layout order\n"
                << "  Esc         leave the quick-launch input\n"
                << "  Q / Esc     quit\n"
                << "  Mouse       hover highlights, left click chooses, "
                   "right click opens the project menu\n\n"
                << "Options:\n"
                << "  --config PATH   Use an alternate config file.\n"
                << "  --where-claude  Print the resolved Claude executable "
                   "and exit.\n\n"
                << "The default config is ./claudex-config.json and the "
                   "default theme is ./claudex-theme.json next to the "
                   "executable.\n"
                << "Missing or invalid config and theme keys are repaired "
                   "automatically at startup.\n"
                << "CLAUDEX_CONFIG may also specify a config path.\n";
            return 0;
        } else {
            std::cerr << "Unknown argument: " << argument
                      << "\nUse --help for usage.\n";
            return 2;
        }
    }

    fs::path configFile = explicitConfigFile.empty()
                              ? executableDir / kDefaultConfigFile
                              : explicitConfigFile;
    const ConfigBootstrapResult bootstrap =
        bootstrapConfigPath(executableDir, configFile, pathResolver);
    configFile = bootstrap.configFile;
    if (configFile.is_relative()) {
        std::error_code ec;
        fs::path absolute = fs::absolute(configFile, ec);
        if (!ec) {
            configFile = std::move(absolute);
        }
    }

    ConfigManager config(executableDir, configFile);
    config.load();
    config.log("INFO", "launcher starting; root=" + pathToUtf8(executableDir) +
                           " config=" + pathToUtf8(config.configFile()) +
                           " theme=" + pathToUtf8(config.themeFile()));
    if (config.loadHadError()) {
        config.log("ERROR", "config load: " + config.loadWarning());
    }
    if (!config.configIssues().empty()) {
        config.log("WARN", "config has " +
                               std::to_string(config.configIssues().size()) +
                               " key issue(s)");
    }
    if (!bootstrap.messageKey.empty()) {
        const std::string text = messageText(
            config.menuTheme, bootstrap.messageKey.c_str(),
            {{"config", bootstrap.messageConfig}});
        config.log("WARN", "bootstrap: " + text);
    }

    if (whereClaude) {
        std::string error;
        const std::optional<fs::path> executable =
            config.findClaudeExecutable(error);
        if (executable) {
            std::cout << pathToUtf8(*executable) << "\n";
            return 0;
        }
        std::cerr << error << "\n";
        return 1;
    }

    // The process cwd is offered as a one-off project only when it is a real
    // folder different from the launcher folder. Detection never fails the
    // launch; an unreadable cwd simply hides the section.
    const CwdProjectInfo cwdProject = detectCwdProject(executableDir);
    const bool cwdVisible = cwdProject.visible();
    const std::string cwdPathText =
        cwdVisible ? pathToUtf8(cwdProject.path) : std::string();
    if (cwdProject.status == CwdProjectStatus::Unavailable &&
        !cwdProject.detail.empty()) {
        config.log("WARN", "cwd section hidden: " + cwdProject.detail);
    } else if (cwdVisible) {
        config.log("INFO", "cwd section: " + cwdPathText);
    } else {
        config.log("INFO", "cwd section hidden: " + cwdProject.detail);
    }

    auto screen = ScreenInteractive::TerminalOutput();
    enum class Page {
        Main = 0,
        Path,
        ConfirmCreate,
        ProjectActions,
        ConfirmRemove
    };
    Page page = Page::Main;
    int pageIndex = static_cast<int>(Page::Main);

    std::vector<std::string> menuEntries;
    int mainSelected = 0;
    std::string statusMessage;
    bool statusIsError = false;
    std::string pathInput;
    std::string pathInputPlaceholder;
    std::string projectName;
    std::string projectNamePlaceholder;
    std::string pathError;
    int pathCursor = 0;
    int nameCursor = 0;
    // Index into pathFieldOrder: the name and path inputs in the order the
    // theme draws them.
    int pathFocus = 0;
    enum class PathMode { Add, Edit };
    PathMode pathMode = PathMode::Add;
    int pathEditIndex = -1;
    fs::path pendingPath;

    // The main page has up to three focus targets. mainFocus is an index into
    // mainFocusOrder, which is built from the authored block order below.
    int mainFocus = 0;
    std::vector<std::string> recentMenuEntries;
    std::vector<int> recentProjectIndices;
    int recentSelected = 0;

    // Hit boxes refreshed by reflect() on every draw. They let the root event
    // handler turn mouse position into a section and row before the menus run.
    // Start them empty so a stray event before the first draw matches nothing.
    std::vector<Box> projectRowBoxes;
    std::vector<Box> recentRowBoxes;
    Box projectListBox = kEmptyHitBox;
    Box recentListBox = kEmptyHitBox;
    Box quickInputBox = kEmptyHitBox;
    Box cwdListBox = kEmptyHitBox;
    Box cwdRowBox = kEmptyHitBox;
    // Path screen input boxes, refreshed on every draw so focus movement uses
    // the positions the theme actually produced.
    Box nameFieldBox = kEmptyHitBox;
    Box pathFieldBox = kEmptyHitBox;
    std::vector<std::string> cwdMenuEntries;
    int cwdSelected = 0;
    if (cwdVisible) {
        cwdMenuEntries.push_back(cwdPathText);
    }
    std::string quickInput;
    int quickInputCursor = 0;
    fs::path pendingLaunchPath;

    std::vector<std::string> projectActionEntries;
    std::vector<Box> actionRowBoxes;
    int projectActionSelected = 0;

    // Shared clickable Back/Confirm bar for the path and confirmation pages.
    // Page transitions install the labels and callbacks; Esc/Enter keep working
    // through the regular key bindings.
    DialogButtonBar dialogBar;
    std::function<void()> pathContinueAction;
    std::function<void()> pathCancelAction;
    std::function<void()> createConfirmAction;
    std::function<void()> createBackAction;
    std::function<void()> removeConfirmAction;
    std::function<void()> removeCancelAction;
    int removeIndex = -1;
    bool launchRequested = false;
    int pendingLaunchIndex = -1;
    std::string pendingCommand;

    // The main project list and the action list are ordinary components, but
    // their decoration and add-row behavior come from their screen blocks.
    const json emptyBlockList = json::array();
    auto screenBlocks = [&](const char* name) -> const json& {
        auto item = config.menuTheme.screens.find(name);
        return item == config.menuTheme.screens.end() ? emptyBlockList
                                                      : item->second.blocks;
    };

    // Focus order follows the actual top-to-bottom block order, so Up/Down
    // boundary jumps and Tab agree with what is drawn on screen.
    std::vector<int> mainFocusOrder;
    collectMainFocusOrder(screenBlocks("main"), mainFocusOrder);

    std::vector<PathField> pathFieldOrder;
    collectPathFieldOrder(screenBlocks("path"), pathFieldOrder);
    if (pathFieldOrder.empty()) {
        pathFieldOrder.push_back(PathField::Path);
    }
    auto pathFieldIndex = [&](PathField field) -> int {
        auto found = std::find(pathFieldOrder.begin(), pathFieldOrder.end(),
                               field);
        return found == pathFieldOrder.end()
                   ? -1
                   : static_cast<int>(found - pathFieldOrder.begin());
    };

    auto focusIndexOf = [&](int node) -> int {
        return claudex::focusIndexOf(mainFocusOrder, node);
    };
    auto currentFocusNode = [&]() -> int {
        if (mainFocus < 0 ||
            mainFocus >= static_cast<int>(mainFocusOrder.size())) {
            return -1;
        }
        return mainFocusOrder[static_cast<std::size_t>(mainFocus)];
    };
    auto focusIs = [&](int node) { return currentFocusNode() == node; };
    auto setFocusNode = [&](int node) -> bool {
        const int index = focusIndexOf(node);
        if (index < 0) {
            return false;
        }
        mainFocus = index;
        return true;
    };

    const int initialFocus = focusIndexOf(kMainFocusProjects);
    mainFocus = initialFocus >= 0 ? initialFocus : 0;

    const json* mainListBlock =
        findBlockByType(screenBlocks("main"), "projectList");
    const json* actionListBlock =
        findBlockByType(screenBlocks("projectActions"), "actionList");

    const std::string listEntryFormat =
        mainListBlock
            ? jsonStringField(*mainListBlock, "entryFormat",
                              config.menuTheme.projectEntryFormat)
            : config.menuTheme.projectEntryFormat;
    const std::string listActivePrefix =
        mainListBlock ? jsonStringField(*mainListBlock, "activePrefix",
                                        config.menuTheme.activePrefix)
                      : config.menuTheme.activePrefix;
    const std::string listInactivePrefix =
        mainListBlock ? jsonStringField(*mainListBlock, "inactivePrefix",
                                        config.menuTheme.inactivePrefix)
                      : config.menuTheme.inactivePrefix;
    const bool showAddProject =
        mainListBlock ? jsonBoolField(*mainListBlock, "addProject", true)
                      : true;
    const bool listInverseHighlight =
        mainListBlock
            ? jsonBoolField(*mainListBlock, "inverseHighlight", true)
            : true;
    const int listHeight =
        mainListBlock ? jsonIntFieldResolved(*mainListBlock, "height", 10,
                                 &config.menuTheme.variables) : 10;
    const bool listScroll =
        mainListBlock ? jsonBoolField(*mainListBlock, "scroll", true) : true;
    const ScrollbarStyle listScrollbar =
        parseScrollbarStyle(mainListBlock, &config.menuTheme.variables);
    const bool listFrame =
        mainListBlock ? jsonBoolField(*mainListBlock, "frame", true) : true;

    const json* recentListBlock =
        findBlockByType(screenBlocks("main"), "recentList");
    const int recentLimit =
        std::max(0, recentListBlock
                        ? jsonIntFieldResolved(*recentListBlock, "limit",
                                       config.menuTheme.recentLimit,
                                       &config.menuTheme.variables)
                        : config.menuTheme.recentLimit);
    const std::string recentEntryFormat =
        recentListBlock
            ? jsonStringField(*recentListBlock, "entryFormat",
                              config.menuTheme.recentEntryFormat)
            : config.menuTheme.recentEntryFormat;
    // Multi-line entry formats. An empty list means "one line from
    // entryFormat"; entryLines takes priority when the block provides it.
    const std::vector<std::string> projectEntryLines =
        mainListBlock != nullptr
            ? stringListField(*mainListBlock, "entryLines", {})
            : std::vector<std::string>{};
    const std::vector<std::string> recentEntryLines =
        recentListBlock != nullptr
            ? stringListField(*recentListBlock, "entryLines", {})
            : std::vector<std::string>{};
    // Optional extra line sets for the row that currently has keyboard focus.
    // They fall back to entryLines when absent; see entryLinesFor.
    const std::vector<std::string> projectFocusedEntryLines =
        mainListBlock != nullptr
            ? stringListField(*mainListBlock, "focusedEntryLines", {})
            : std::vector<std::string>{};
    const std::vector<std::string> recentFocusedEntryLines =
        recentListBlock != nullptr
            ? stringListField(*recentListBlock, "focusedEntryLines", {})
            : std::vector<std::string>{};

    const json* cwdListBlock =
        findBlockByType(screenBlocks("main"), "cwdList");
    const std::string cwdEntryFormat =
        cwdListBlock ? jsonStringField(*cwdListBlock, "entryFormat",
                                       config.menuTheme.cwdEntryFormat)
                     : config.menuTheme.cwdEntryFormat;
    const std::vector<std::string> cwdEntryLines =
        cwdListBlock != nullptr
            ? stringListField(*cwdListBlock, "entryLines", {})
            : std::vector<std::string>{};
    const std::vector<std::string> cwdFocusedEntryLines =
        cwdListBlock != nullptr
            ? stringListField(*cwdListBlock, "focusedEntryLines", {})
            : std::vector<std::string>{};

    // projectNameDisplay controls where the optional name line appears.
    // Passing an empty name makes the name placeholders expand as unnamed,
    // just like a project that has no name of its own.
    const ProjectNameDisplay nameDisplay =
        config.menuTheme.projectNameDisplay;
    const bool pinnedNamesShown = nameDisplay == ProjectNameDisplay::Both ||
                                  nameDisplay == ProjectNameDisplay::Pinned;
    const bool recentNamesShown = nameDisplay == ProjectNameDisplay::Both ||
                                  nameDisplay == ProjectNameDisplay::Recent;

    auto projectLinesFor = [&](int index, const std::string& raw,
                               bool focusedRow) -> std::vector<std::string> {
        if (index < 0 || index >= static_cast<int>(config.projects.size())) {
            return {raw};
        }
        const Project& project =
            config.projects[static_cast<std::size_t>(index)];
        return entryLinesFor(
            projectFocusedEntryLines, projectEntryLines, listEntryFormat,
            focusedRow, pinnedNamesShown ? project.name : std::string(),
            project.path, project.lastUsed, &config.menuTheme.variables);
    };

    auto recentLinesFor =
        [&](int index, bool focusedRow) -> std::vector<std::string> {
        if (index < 0 ||
            index >= static_cast<int>(recentProjectIndices.size())) {
            return {};
        }
        const Project& project = config.projects[static_cast<std::size_t>(
            recentProjectIndices[static_cast<std::size_t>(index)])];
        return entryLinesFor(
            recentFocusedEntryLines, recentEntryLines, recentEntryFormat,
            focusedRow, recentNamesShown ? project.name : std::string(),
            project.path, project.lastUsed, &config.menuTheme.variables);
    };

    auto cwdLinesFor = [&](bool focusedRow) -> std::vector<std::string> {
        if (!cwdVisible) {
            return {};
        }
        return entryLinesFor(cwdFocusedEntryLines, cwdEntryLines,
                             cwdEntryFormat, focusedRow, std::string(),
                             cwdPathText, std::string(),
                             &config.menuTheme.variables);
    };

    const std::string cwdActivePrefix =
        cwdListBlock ? jsonStringField(*cwdListBlock, "activePrefix",
                                       config.menuTheme.activePrefix)
                     : config.menuTheme.activePrefix;
    const std::string cwdInactivePrefix =
        cwdListBlock ? jsonStringField(*cwdListBlock, "inactivePrefix",
                                       config.menuTheme.inactivePrefix)
                     : config.menuTheme.inactivePrefix;
    const bool cwdInverseHighlight =
        cwdListBlock ? jsonBoolField(*cwdListBlock, "inverseHighlight", true)
                     : true;

    const std::string recentActivePrefix =
        recentListBlock
            ? jsonStringField(*recentListBlock, "activePrefix",
                              config.menuTheme.activePrefix)
            : config.menuTheme.activePrefix;
    const std::string recentInactivePrefix =
        recentListBlock
            ? jsonStringField(*recentListBlock, "inactivePrefix",
                              config.menuTheme.inactivePrefix)
            : config.menuTheme.inactivePrefix;
    const bool recentInverseHighlight =
        recentListBlock
            ? jsonBoolField(*recentListBlock, "inverseHighlight", true)
            : true;
    const int recentHeight =
        recentListBlock ? jsonIntFieldResolved(*recentListBlock, "height", 0,
                                  &config.menuTheme.variables) : 0;
    // Scrolling is theme-controlled: the block's "scroll" and "frame" keys
    // decide it, and the shipped default theme enables them for every list.
    // When enabled, visibility follows the rendered lines (a two-line entry
    // takes two rows), so an overflowing list gets a scrollbar instead of
    // being clipped.
    const bool recentScroll =
        recentListBlock ? jsonBoolField(*recentListBlock, "scroll", false)
                        : false;
    const ScrollbarStyle recentScrollbar =
        parseScrollbarStyle(recentListBlock, &config.menuTheme.variables);
    const bool recentFrame =
        recentListBlock ? jsonBoolField(*recentListBlock, "frame", false)
                        : false;
    std::string quickInputPlaceholder =
        config.expandPath(config.menuTheme.quickInputPlaceholder);

    const int actionHeight =
        actionListBlock ? jsonIntFieldResolved(*actionListBlock, "height", 0,
                                  &config.menuTheme.variables) : 0;
    const bool actionScroll =
        actionListBlock ? jsonBoolField(*actionListBlock, "scroll", false)
                        : false;
    const ScrollbarStyle actionScrollbar =
        parseScrollbarStyle(actionListBlock, &config.menuTheme.variables);
    const bool actionFrame =
        actionListBlock ? jsonBoolField(*actionListBlock, "frame", false)
                        : false;
    const bool actionInverseHighlight =
        actionListBlock
            ? jsonBoolField(*actionListBlock, "inverseHighlight", true)
            : true;

    // A list becomes a grid when its block asks for columns: a number fixes
    // the column count, "auto" fills the available width.
    auto gridSpecFor = [&](const json* block, int& columns,
                           bool& autoColumns) {
        columns = 0;
        autoColumns = false;
        if (block == nullptr || !block->contains("columns")) {
            return false;
        }
        const json& value = (*block)["columns"];
        if (value.is_string() &&
            normalizeMenuToken(value.get<std::string>()) == "auto") {
            autoColumns = true;
            return true;
        }
        if (const auto resolved =
                resolveThemeInt(value, &config.menuTheme.variables)) {
            columns = *resolved;
            return columns > 1;
        }
        return false;
    };

    auto makeGridOptions = [&](const json* block, int columns,
                               bool autoColumns, const std::string& active,
                               const std::string& inactive, bool inverse) {
        GridMenuOptions options;
        options.columns = columns;
        options.autoColumns = autoColumns;
        options.activePrefix = active;
        options.inactivePrefix = inactive;
        options.inverseHighlight = inverse;
        options.variables = &config.menuTheme.variables;
        if (block != nullptr) {
            options.cellWidth = jsonIntFieldResolved(
                *block, "cellWidth", 0, &config.menuTheme.variables);
            options.columnGap = jsonIntFieldResolved(
                *block, "columnGap", 2, &config.menuTheme.variables);
            options.pageRows = jsonIntFieldResolved(
                *block, "pageRows", 5, &config.menuTheme.variables);
        }
        return options;
    };

    auto setStatus = [&](std::string message, bool error) {
        statusMessage = std::move(message);
        statusIsError = error;
    };

    auto updatePathPlaceholder = [&]() {
        pathInputPlaceholder =
            config.expandPath(config.menuTheme.pathPlaceholder);
        projectNamePlaceholder =
            config.expandPath(config.menuTheme.projectNamePlaceholder);
    };

    auto refreshRecent = [&]() {
        const std::vector<int> previousIndices = recentProjectIndices;
        recentMenuEntries.clear();
        recentProjectIndices =
            claudex::recentProjectIndices(config.projects, recentLimit);
        for (int index : recentProjectIndices) {
            // The entry text is built at render time from the project, so the
            // name is available too.
            recentMenuEntries.push_back(
                config.projects[static_cast<std::size_t>(index)].path);
        }
        recentRowBoxes.assign(recentMenuEntries.size(), kEmptyHitBox);
        recentSelected = keptSelectionIndex(previousIndices, recentSelected,
                                            recentProjectIndices);
        if (focusIs(kMainFocusRecent) && recentMenuEntries.empty()) {
            if (!setFocusNode(kMainFocusProjects)) {
                mainFocus = 0;
            }
        }
    };

    auto refreshMenu = [&]() {
        menuEntries.clear();
        for (const Project& project : config.projects) {
            menuEntries.push_back(project.path);
        }
        if (showAddProject) {
            menuEntries.push_back(config.menuTheme.addProjectLabel);
        }

        projectRowBoxes.assign(menuEntries.size(), kEmptyHitBox);
        if (menuEntries.empty()) {
            mainSelected = 0;
        } else {
            mainSelected =
                std::clamp(mainSelected, 0, static_cast<int>(menuEntries.size()) - 1);
        }
        refreshRecent();
    };

    auto refreshActions = [&]() {
        projectActionEntries.clear();
        for (const MenuAction& action : config.menuTheme.actions) {
            projectActionEntries.push_back(action.label);
        }
        actionRowBoxes.assign(projectActionEntries.size(), kEmptyHitBox);
        if (projectActionEntries.empty()) {
            projectActionSelected = 0;
        } else {
            projectActionSelected =
                std::clamp(projectActionSelected, 0,
                           static_cast<int>(projectActionEntries.size()) - 1);
        }
    };

    refreshActions();

    auto clearProjectStatus = [&]() {
        if (statusIsError) {
            statusMessage.clear();
            statusIsError = false;
        }
    };

    // Every page transition installs the buttons that belong to that page, so
    // the same clickable Back/Confirm bar serves the path and confirm screens.
    auto switchPage = [&](Page next) {
        page = next;
        pageIndex = static_cast<int>(next);
        if (next == Page::Path) {
            dialogBar.set(
                {config.menuTheme.pathContinueLabel,
                 config.menuTheme.pathCancelLabel},
                [&](int index) {
                    if (index == 0) {
                        if (pathContinueAction) {
                            pathContinueAction();
                        }
                    } else if (pathCancelAction) {
                        pathCancelAction();
                    }
                });
        } else if (next == Page::ConfirmCreate) {
            dialogBar.set(
                {config.menuTheme.confirmCreateLabel,
                 config.menuTheme.confirmCreateBackLabel},
                [&](int index) {
                    if (index == 0) {
                        if (createConfirmAction) {
                            createConfirmAction();
                        }
                    } else if (createBackAction) {
                        createBackAction();
                    }
                });
        } else if (next == Page::ConfirmRemove) {
            dialogBar.set(
                {config.menuTheme.confirmRemoveLabel,
                 config.menuTheme.confirmRemoveBackLabel},
                [&](int index) {
                    if (index == 0) {
                        if (removeConfirmAction) {
                            removeConfirmAction();
                        }
                    } else if (removeCancelAction) {
                        removeCancelAction();
                    }
                });
        } else {
            dialogBar.clear();
        }
    };

    // The path screen can be the configured focus target: "name" and "path"
    // select the field the theme drew. It is applied when the screen opens,
    // not when returning from the confirm page, so the field the user chose
    // is kept in that round trip.
    auto applyConfiguredPathFocus = [&]() {
        const DefaultFocus* target =
            pathFocusTarget(config.settings.defaultFocusTargets);
        if (target == nullptr) {
            return;
        }
        const FocusSection focusSection =
            focusSectionFromText(target->section);
        const int index = pathFieldIndex(
            focusSection == FocusSection::PathName ? PathField::Name
                                                   : PathField::Path);
        if (index >= 0) {
            pathFocus = index;
        }
    };

    auto beginAddProject = [&]() {
        pathMode = PathMode::Add;
        pathEditIndex = -1;
        pathInput.clear();
        // Prefill the optional name field from the config. The value follows
        // the same trim rule as a name typed by hand.
        projectName = trim(config.settings.defaultProjectName);
        pathCursor = 0;
        nameCursor = static_cast<int>(projectName.size());
        pathFocus = std::max(0, pathFieldIndex(PathField::Path));
        applyConfiguredPathFocus();
        pathError.clear();
        pendingPath.clear();
        updatePathPlaceholder();
        switchPage(Page::Path);
    };

    auto beginProjectActions = [&]() {
        if (mainSelected < 0 ||
            mainSelected >= static_cast<int>(config.projects.size())) {
            setStatus(messageText(config.menuTheme, "selectProjectFirst"),
                      false);
            return;
        }
        projectActionSelected = 0;
        if (const DefaultFocus* target =
                actionFocusTarget(config.settings.defaultFocusTargets)) {
            projectActionSelected =
                actionFocusIndex(config.menuTheme.actions, *target);
        }
        switchPage(Page::ProjectActions);
    };

    auto beginEditProject = [&](PathMode mode) {
        if (config.projects.empty() ||
            mainSelected < 0 ||
            mainSelected >= static_cast<int>(config.projects.size())) {
            setStatus(messageText(config.menuTheme, "selectProjectFirst"),
                      false);
            return;
        }
        const Project& project =
            config.projects[static_cast<std::size_t>(mainSelected)];
        pathMode = mode;
        pathEditIndex = mainSelected;
        pathInput = project.path;
        projectName = project.name;
        pathCursor = static_cast<int>(pathInput.size());
        nameCursor = static_cast<int>(projectName.size());
        pathFocus = std::max(0, pathFieldIndex(PathField::Path));
        applyConfiguredPathFocus();
        pathError.clear();
        pendingPath.clear();
        updatePathPlaceholder();
        switchPage(Page::Path);
    };

    auto saveConfigWithStatus = [&](const std::string& successMessage) {
        std::string error;
        if (!config.save(error)) {
            setStatus(error, true);
            return false;
        }
        setStatus(successMessage, false);
        return true;
    };

    auto moveProject = [&](int step, bool toEnd) {
        if (config.projects.empty() || mainSelected < 0 ||
            mainSelected >= static_cast<int>(config.projects.size())) {
            setStatus(messageText(config.menuTheme, "selectProjectFirst"),
                      false);
            switchPage(Page::Main);
            return;
        }

        const int index = mainSelected;
        const int last = static_cast<int>(config.projects.size()) - 1;
        const int target = toEnd ? (step < 0 ? 0 : last)
                                 : std::clamp(index + step, 0, last);
        if (target == index) {
            setStatus(messageText(config.menuTheme,
                                  step < 0 ? "projectAlreadyFirst"
                                           : "projectAlreadyLast"),
                      false);
            switchPage(Page::Main);
            return;
        }

        Project moved = std::move(config.projects[static_cast<std::size_t>(
            index)]);
        config.projects.erase(config.projects.begin() + index);
        config.projects.insert(config.projects.begin() + target,
                               std::move(moved));
        refreshMenu();
        mainSelected = target;
        saveConfigWithStatus(messageText(config.menuTheme, "projectMoved"));
        switchPage(Page::Main);
    };

    auto commitPath = [&](const fs::path& absolutePath) {
        const std::string storedPath =
            config.storeProjectPath(pathInput, absolutePath);

        if (pathMode == PathMode::Edit) {
            if (pathEditIndex < 0 ||
                pathEditIndex >= static_cast<int>(config.projects.size())) {
                pathError = messageText(config.menuTheme,
                                        "projectNoLongerListed");
                switchPage(Page::Path);
                return;
            }
            config.projects[static_cast<std::size_t>(pathEditIndex)].path =
                storedPath;
            config.projects[static_cast<std::size_t>(pathEditIndex)].name =
                trim(projectName);
            if (!saveConfigWithStatus(messageText(
                    config.menuTheme, "projectPathUpdated"))) {
                pathError = statusMessage;
                switchPage(Page::Path);
                return;
            }
            refreshMenu();
            mainSelected = pathEditIndex;
            switchPage(Page::Main);
            return;
        }

        // A newly added project has not been launched yet, so it does not
        // appear in the recent list until it is actually used. Repeating a
        // path is allowed.
        config.projects.push_back(
            Project{storedPath, "", trim(projectName)});
        const int addedIndex = static_cast<int>(config.projects.size()) - 1;
        const bool saved =
            saveConfigWithStatus(messageText(config.menuTheme, "projectAdded"));
        std::string saveFailure;
        if (!saved) {
            saveFailure = statusMessage;
        }
        refreshMenu();
        mainSelected = addedIndex;
        switchPage(Page::Main);
        if (!saved) {
            statusMessage = messageText(config.menuTheme,
                                        "projectAddedSaveFailed",
                                        {{"error", saveFailure}});
            statusIsError = true;
        }
    };

    auto submitPath = [&]() {
        pathError.clear();
        const std::string entered = trim(pathInput);
        if (entered.empty()) {
            pathError = messageText(config.menuTheme, "enterFolderPath");
            return;
        }

        const PathResolution resolution = config.resolvePath(
            entered, PathKind::Directory, MultiMatchPolicy::RequireUnique);
        if (resolution.wildcard && resolution.noMatch) {
            pathError = messageText(config.menuTheme, "pathPatternNoMatch",
                                    {{"value", resolution.expanded}});
            return;
        }
        if (resolution.wildcard && resolution.ambiguous) {
            std::string candidates;
            for (const fs::path& match : resolution.matches) {
                if (!candidates.empty()) {
                    candidates += ", ";
                }
                candidates += pathToUtf8(match);
            }
            pathError = messageText(config.menuTheme, "pathPatternAmbiguous",
                                    {{"value", resolution.expanded},
                                     {"candidates", candidates}});
            return;
        }
        const fs::path absolute = resolution.path;
        if (!resolution.resolved || absolute.empty()) {
            pathError = messageText(config.menuTheme, "invalidFolderPath");
            return;
        }

        std::error_code ec;
        if (fs::is_directory(absolute, ec) && !ec) {
            commitPath(absolute);
            return;
        }
        if (fs::exists(absolute, ec) && !ec) {
            pathError = messageText(config.menuTheme, "pathNotFolder",
                                    {{"path", pathToUtf8(absolute)}});
            return;
        }
        if (ec && ec != std::errc::no_such_file_or_directory) {
            pathError = messageText(
                config.menuTheme, "pathInspectFailed",
                {{"path", pathToUtf8(absolute)}, {"error", ec.message()}});
            return;
        }

        pendingPath = absolute;
        switchPage(Page::ConfirmCreate);
    };

    auto requestLaunchIndex = [&](int index) {
        if (index < 0 ||
            index >= static_cast<int>(config.projects.size())) {
            return;
        }

        pendingLaunchIndex = index;
        pendingLaunchPath.clear();
        pendingCommand.clear();
        launchRequested = true;
        switchPage(Page::Main);
        screen.Exit();
    };

    auto requestLaunch = [&]() { requestLaunchIndex(mainSelected); };

    // One-off launch used by quick launch and the cwd section. An already
    // listed folder counts as a normal launch so lastUsed is refreshed.
    auto requestOneOffLaunch = [&](const fs::path& path) {
        const int existing = config.findProject(path);
        if (existing >= 0) {
            requestLaunchIndex(existing);
            return;
        }
        pendingLaunchIndex = -1;
        pendingLaunchPath = path;
        pendingCommand.clear();
        launchRequested = true;
        switchPage(Page::Main);
        screen.Exit();
    };

    auto requestCwdLaunch = [&]() {
        if (cwdVisible) {
            requestOneOffLaunch(cwdProject.path);
        }
    };

    // Quick launch validates the typed folder but never writes it to the
    // project list. When the folder is already listed, this counts as a normal
    // launch so its lastUsed timestamp is refreshed.
    auto submitQuickLaunch = [&]() {
        const std::string entered = trim(quickInput);
        if (entered.empty()) {
            setStatus(messageText(config.menuTheme, "quickPathEmpty"), true);
            return;
        }

        const PathResolution resolution = config.resolvePath(
            entered, PathKind::Directory, MultiMatchPolicy::RequireUnique);
        if (resolution.wildcard && resolution.noMatch) {
            setStatus(messageText(config.menuTheme, "pathPatternNoMatch",
                                  {{"value", resolution.expanded}}),
                      true);
            return;
        }
        if (resolution.wildcard && resolution.ambiguous) {
            std::string candidates;
            for (const fs::path& match : resolution.matches) {
                if (!candidates.empty()) {
                    candidates += ", ";
                }
                candidates += pathToUtf8(match);
            }
            setStatus(messageText(config.menuTheme, "pathPatternAmbiguous",
                                  {{"value", resolution.expanded},
                                   {"candidates", candidates}}),
                      true);
            return;
        }
        const fs::path absolute = resolution.path;
        if (!resolution.resolved || absolute.empty()) {
            setStatus(messageText(config.menuTheme, "invalidFolderPath"),
                      true);
            return;
        }

        std::error_code ec;
        if (!fs::is_directory(absolute, ec)) {
            if (ec && ec != std::errc::no_such_file_or_directory) {
                setStatus(
                    messageText(config.menuTheme, "pathInspectFailed",
                                {{"path", pathToUtf8(absolute)},
                                 {"error", ec.message()}}),
                    true);
                return;
            }
            ec.clear();
            if (fs::exists(absolute, ec) && !ec) {
                setStatus(
                    messageText(config.menuTheme, "pathNotFolder",
                                {{"path", pathToUtf8(absolute)}}),
                    true);
            } else if (ec) {
                setStatus(
                    messageText(config.menuTheme, "pathInspectFailed",
                                {{"path", pathToUtf8(absolute)},
                                 {"error", ec.message()}}),
                    true);
            } else {
                setStatus(
                    messageText(config.menuTheme, "quickPathNotFound",
                                {{"path", pathToUtf8(absolute)}}),
                    true);
            }
            return;
        }

        quickInput.clear();
        quickInputCursor = 0;
        requestOneOffLaunch(absolute);
    };

    auto beginRemove = [&]() {
        if (config.projects.empty() ||
            mainSelected < 0 ||
            mainSelected >= static_cast<int>(config.projects.size())) {
            setStatus(messageText(config.menuTheme, "selectProjectFirst"),
                      false);
            return;
        }
        removeIndex = mainSelected;
        switchPage(Page::ConfirmRemove);
    };

    // The visible buttons and the keyboard shortcuts share these actions.
    pathContinueAction = submitPath;
    pathCancelAction = [&]() {
        pathError.clear();
        switchPage(Page::Main);
    };
    createConfirmAction = [&]() {
        std::error_code ec;
        fs::create_directories(pendingPath, ec);
        if (!ec) {
            const bool isFolder = fs::is_directory(pendingPath, ec);
            if (!isFolder && !ec) {
                pathError = messageText(
                    config.menuTheme, "pathNotFolder",
                    {{"path", pathToUtf8(pendingPath)}});
                switchPage(Page::Path);
                return;
            }
        }
        if (ec) {
            pathError = messageText(
                config.menuTheme, "pathCreateFailed",
                {{"path", pathToUtf8(pendingPath)},
                 {"error", ec.message()}});
            switchPage(Page::Path);
            return;
        }
        commitPath(pendingPath);
    };
    createBackAction = [&]() { switchPage(Page::Path); };
    removeConfirmAction = [&]() {
        if (removeIndex >= 0 &&
            removeIndex < static_cast<int>(config.projects.size())) {
            config.projects.erase(config.projects.begin() + removeIndex);
            refreshMenu();
            saveConfigWithStatus(
                messageText(config.menuTheme, "projectRemoved"));
        }
        removeIndex = -1;
        switchPage(Page::Main);
    };
    removeCancelAction = [&]() {
        removeIndex = -1;
        switchPage(Page::Main);
    };

    MenuOption menuOptions = MenuOption::Vertical();
    menuOptions.entries_option.transform = [&](const EntryState& state) {
        const bool focusedRow =
            state.active && focusIs(kMainFocusProjects);
        Element element = styleMenuEntryLines(
            state, projectLinesFor(state.index, state.label, focusedRow),
            true, focusIs(kMainFocusProjects), listActivePrefix,
            listInactivePrefix, listInverseHighlight,
            &config.menuTheme.variables);
        if (state.index >= 0 &&
            state.index < static_cast<int>(projectRowBoxes.size())) {
            element |= reflect(
                projectRowBoxes[static_cast<std::size_t>(state.index)]);
        }
        return element;
    };
    menuOptions.on_enter = [&]() {
        if (mainSelected >= 0 &&
            mainSelected < static_cast<int>(config.projects.size())) {
            requestLaunch();
        } else if (showAddProject) {
            beginAddProject();
        }
    };

    refreshMenu();
    updatePathPlaceholder();

    // A section is usable when the theme authored it and it currently has
    // something to focus. The cwd section is hidden when the process started
    // from the launcher folder itself.
    auto sectionAvailable = [&](int node) {
        if (node == kMainFocusCwd) {
            return cwdVisible;
        }
        if (node == kMainFocusRecent) {
            return !recentMenuEntries.empty();
        }
        return true;
    };

    // Apply the configured startup focus once the lists are populated.
    // defaultFocus is a priority list: the first available main-screen target
    // wins, and the theme's authored block order is the fallback.
    {
        const FocusChoice focusChoice = resolveDefaultFocus(
            config.settings.defaultFocusTargets, mainFocusOrder,
            static_cast<int>(menuEntries.size()),
            static_cast<int>(recentMenuEntries.size()), cwdVisible,
            config.projects, recentProjectIndices);
        if (focusChoice.node >= 0) {
            const int focusIndex = focusIndexOf(focusChoice.node);
            if (focusIndex >= 0) {
                mainFocus = focusIndex;
            }
            if (focusChoice.node == kMainFocusProjects) {
                mainSelected = focusChoice.item;
            } else if (focusChoice.node == kMainFocusRecent) {
                recentSelected = focusChoice.item;
            }
        }
    }

    auto mainMenu = Menu(&menuEntries, &mainSelected, menuOptions);

    int projectGridColumns = 0;
    bool projectGridAuto = false;
    const bool projectUsesGrid =
        gridSpecFor(mainListBlock, projectGridColumns, projectGridAuto);
    std::shared_ptr<GridMenuComponent> projectGrid;
    if (projectUsesGrid) {
        GridMenuOptions options = makeGridOptions(
            mainListBlock, projectGridColumns, projectGridAuto,
            listActivePrefix, listInactivePrefix, listInverseHighlight);
        options.format_lines = [&](int index, const std::string& label) {
            return projectLinesFor(index, label, false);
        };
        options.focused_format_lines =
            [&](int index, const std::string& label) {
                return projectLinesFor(index, label, true);
            };
        options.on_change = clearProjectStatus;
        options.on_enter = [&](int index) {
            mainSelected = index;
            if (index >= 0 &&
                index < static_cast<int>(config.projects.size())) {
                requestLaunch();
            } else if (showAddProject) {
                beginAddProject();
            }
        };
        options.on_right_click = [&](int index) {
            mainSelected = index;
            beginProjectActions();
        };
        projectGrid = std::make_shared<GridMenuComponent>(
            &menuEntries, &mainSelected, options);
    }
    const Component projectListComponent =
        projectGrid ? Component(projectGrid) : Component(mainMenu);

    MenuOption recentMenuOptions = MenuOption::Vertical();
    recentMenuOptions.entries_option.transform = [&](const EntryState& state) {
        const bool focusedRow = state.active && focusIs(kMainFocusRecent);
        Element element = styleMenuEntryLines(
            state, recentLinesFor(state.index, focusedRow), true,
            focusIs(kMainFocusRecent), recentActivePrefix,
            recentInactivePrefix, recentInverseHighlight,
            &config.menuTheme.variables);
        if (state.index >= 0 &&
            state.index < static_cast<int>(recentRowBoxes.size())) {
            element |= reflect(
                recentRowBoxes[static_cast<std::size_t>(state.index)]);
        }
        return element;
    };
    recentMenuOptions.on_change = clearProjectStatus;
    recentMenuOptions.on_enter = [&]() {
        if (recentSelected >= 0 &&
            recentSelected <
                static_cast<int>(recentProjectIndices.size())) {
            requestLaunchIndex(
                recentProjectIndices[static_cast<std::size_t>(
                    recentSelected)]);
        }
    };
    auto recentMenu =
        Menu(&recentMenuEntries, &recentSelected, recentMenuOptions);

    int recentGridColumns = 0;
    bool recentGridAuto = false;
    const bool recentUsesGrid = gridSpecFor(
        recentListBlock, recentGridColumns, recentGridAuto);
    std::shared_ptr<GridMenuComponent> recentGrid;
    if (recentUsesGrid) {
        GridMenuOptions options = makeGridOptions(
            recentListBlock, recentGridColumns, recentGridAuto,
            recentActivePrefix, recentInactivePrefix, recentInverseHighlight);
        options.format_lines = [&](int index, const std::string&) {
            return recentLinesFor(index, false);
        };
        options.focused_format_lines =
            [&](int index, const std::string&) {
                return recentLinesFor(index, true);
            };
        options.on_change = clearProjectStatus;
        options.on_enter = [&](int index) {
            if (index >= 0 &&
                index < static_cast<int>(recentProjectIndices.size())) {
                requestLaunchIndex(
                    recentProjectIndices[static_cast<std::size_t>(index)]);
            }
        };
        options.on_right_click = [&](int index) {
            if (index >= 0 &&
                index < static_cast<int>(recentProjectIndices.size())) {
                mainSelected =
                    recentProjectIndices[static_cast<std::size_t>(index)];
                beginProjectActions();
            }
        };
        recentGrid = std::make_shared<GridMenuComponent>(
            &recentMenuEntries, &recentSelected, options);
    }
    const Component recentListComponent =
        recentGrid ? Component(recentGrid) : Component(recentMenu);

    // The cwd section is a one-row list. It stays in the component tree even
    // when hidden so the mainFocus index continues to match mainFocusOrder;
    // an empty menu is not focusable, and sectionAvailable masks it.
    MenuOption cwdMenuOptions = MenuOption::Vertical();
    cwdMenuOptions.entries_option.transform = [&](const EntryState& state) {
        const bool focusedRow = state.active && focusIs(kMainFocusCwd);
        Element element = styleMenuEntryLines(
            state, cwdLinesFor(focusedRow), true, focusIs(kMainFocusCwd),
            cwdActivePrefix, cwdInactivePrefix, cwdInverseHighlight,
            &config.menuTheme.variables);
        element |= reflect(cwdRowBox);
        return element;
    };
    cwdMenuOptions.on_enter = [&]() { requestCwdLaunch(); };
    auto cwdMenu = Menu(&cwdMenuEntries, &cwdSelected, cwdMenuOptions);
    const Component cwdListComponent = cwdMenu;

    MenuOption actionMenuOptions = MenuOption::Vertical();
    actionMenuOptions.entries_option.transform = [&](const EntryState& state) {
        Element element =
            styleMenuEntry(state, state.label, true, true,
                           config.menuTheme.activePrefix,
                           config.menuTheme.inactivePrefix,
                           actionInverseHighlight,
                           &config.menuTheme.variables);
        if (state.index >= 0 &&
            state.index < static_cast<int>(actionRowBoxes.size())) {
            element |= reflect(
                actionRowBoxes[static_cast<std::size_t>(state.index)]);
        }
        return element;
    };
    auto projectActionsMenu = Menu(&projectActionEntries,
                                   &projectActionSelected, actionMenuOptions);

    // activateProjectAction is defined further down, so the grid calls it
    // through this placeholder, which is bound before the first draw.
    std::function<void()> activateActionCallback;
    int actionGridColumns = 0;
    bool actionGridAuto = false;
    const bool actionUsesGrid = gridSpecFor(
        actionListBlock, actionGridColumns, actionGridAuto);
    std::shared_ptr<GridMenuComponent> actionGrid;
    if (actionUsesGrid) {
        GridMenuOptions options = makeGridOptions(
            actionListBlock, actionGridColumns, actionGridAuto,
            config.menuTheme.activePrefix, config.menuTheme.inactivePrefix,
            actionInverseHighlight);
        options.on_enter = [&](int index) {
            projectActionSelected = index;
            if (activateActionCallback) {
                activateActionCallback();
            }
        };
        actionGrid = std::make_shared<GridMenuComponent>(
            &projectActionEntries, &projectActionSelected, options);
    }
    const Component actionListComponent =
        actionGrid ? Component(actionGrid) : Component(projectActionsMenu);

    InputOption pathInputOptions;
    pathInputOptions.multiline = false;
    pathInputOptions.cursor_position = &pathCursor;
    applyThemePlaceholder(pathInputOptions, config.menuTheme,
                          &pathInputPlaceholder);
    auto pathInputComponent =
        Input(&pathInput, &pathInputPlaceholder, pathInputOptions);

    InputOption nameInputOptions;
    nameInputOptions.multiline = false;
    nameInputOptions.cursor_position = &nameCursor;
    applyThemePlaceholder(nameInputOptions, config.menuTheme,
                          &projectNamePlaceholder);
    auto nameInputComponent =
        Input(&projectName, &projectNamePlaceholder, nameInputOptions);

    // Both fields share the path page. Build the container in the authored
    // block order; arrows use the reflected boxes below, so the movement
    // follows the positions the theme actually drew.
    Components pathFieldComponents;
    for (const PathField field : pathFieldOrder) {
        pathFieldComponents.push_back(field == PathField::Name
                                          ? nameInputComponent
                                          : pathInputComponent);
    }
    auto pathFields =
        Container::Vertical(pathFieldComponents, &pathFocus);

    InputOption quickInputOptions;
    quickInputOptions.multiline = false;
    quickInputOptions.cursor_position = &quickInputCursor;
    quickInputOptions.on_enter = submitQuickLaunch;
    applyThemePlaceholder(quickInputOptions, config.menuTheme,
                          &quickInputPlaceholder);
    auto quickInputComponent =
        Input(&quickInput, &quickInputPlaceholder, quickInputOptions);

    // Build the container in the same order as the blocks, so arrow-key
    // boundary jumps and Tab always move to the visually adjacent section.
    Components mainChildren;
    for (int node : mainFocusOrder) {
        if (node == kMainFocusProjects) {
            mainChildren.push_back(projectListComponent);
        } else if (node == kMainFocusRecent) {
            mainChildren.push_back(recentListComponent);
        } else if (node == kMainFocusQuickInput) {
            mainChildren.push_back(quickInputComponent);
        } else if (node == kMainFocusCwd) {
            mainChildren.push_back(cwdListComponent);
        }
    }
    if (mainChildren.empty()) {
        mainChildren.push_back(projectListComponent);
    }
    auto mainContent = Container::Vertical(mainChildren, &mainFocus);
    auto mainPage = Renderer(mainContent, [&] {
        int focusedIndex = mainSelected;
        if (focusIs(kMainFocusRecent) && recentSelected >= 0 &&
            recentSelected <
                static_cast<int>(recentProjectIndices.size())) {
            focusedIndex = recentProjectIndices[static_cast<std::size_t>(
                recentSelected)];
        }

        std::string selectedProject;
        std::string selectedName;
        if (focusedIndex >= 0 &&
            focusedIndex < static_cast<int>(config.projects.size())) {
            const Project& project =
                config.projects[static_cast<std::size_t>(focusedIndex)];
            selectedProject = pathToUtf8(config.resolveProjectPath(project));
            selectedName = project.name;
        }

        BlockRenderContext context;
        context.config = &config;
        context.theme = &config.menuTheme;
        context.projectPath = selectedProject;
        context.projectName = selectedName;
        context.pathText = selectedProject;
        context.statusText = statusMessage;
        context.statusIsError = statusIsError;
        context.showAddProject = showAddProject;
        context.hasSelection = !selectedProject.empty();
        context.recentCount =
            static_cast<int>(recentMenuEntries.size());

        Element list = projectListComponent->Render();
        if (listScroll) {
            list = addScrollbar(std::move(list), listScrollbar);
        }
        if (listFrame) {
            list = list | frame;
        }
        if (listHeight > 0) {
            list = list | size(HEIGHT, LESS_THAN, listHeight);
        }
        context.projectList =
            std::move(list) | reflect(projectListBox);
        context.hasProjectList = true;

        if (!recentMenuEntries.empty()) {
            Element recent = recentListComponent->Render();
            if (recentScroll) {
                recent = addScrollbar(std::move(recent), recentScrollbar);
            }
            if (recentFrame) {
                recent = recent | frame;
            }
            if (recentHeight > 0) {
                recent = recent | size(HEIGHT, LESS_THAN, recentHeight);
            }
            context.recentList =
                std::move(recent) | reflect(recentListBox);
            context.hasRecentList = true;
        } else {
            // The section is hidden; make sure its old hit box cannot keep
            // receiving mouse events.
            recentListBox = kEmptyHitBox;
        }

        context.cwdPath = cwdPathText;
        if (cwdVisible) {
            context.cwdList =
                cwdListComponent->Render() | reflect(cwdListBox);
            context.hasCwdList = true;
        } else {
            context.hasCwdList = false;
            cwdListBox = kEmptyHitBox;
            cwdRowBox = kEmptyHitBox;
        }

        context.quickInput =
            quickInputComponent->Render() | reflect(quickInputBox);
        context.hasQuickInput = true;

        return renderScreen(screenThemeFor(config.menuTheme, "main"),
                            context);
    });

    auto pathPage = Renderer(pathFields, [&] {
        const ScreenTheme& screen =
            screenThemeFor(config.menuTheme, "path");

        BlockRenderContext context;
        context.config = &config;
        context.theme = &config.menuTheme;
        context.modeText = pathMode == PathMode::Add ? screen.addTitle
                                                     : screen.editTitle;
        context.pathText = pathInput;
        context.projectName = projectName;
        context.errorText = pathError;
        context.pathInput =
            hbox({text(" "), pathInputComponent->Render() | flex}) |
            reflect(pathFieldBox);
        context.hasPathInput = true;
        context.projectNameInput =
            hbox({text(" "), nameInputComponent->Render() | flex}) |
            reflect(nameFieldBox);
        context.hasProjectNameInput = true;
        context.dialogButtons = dialogBar.render(config.menuTheme);
        context.hasDialogButtons = true;

        return renderScreen(screen, context);
    });

    auto confirmCreatePage = Renderer([&] {
        BlockRenderContext context;
        context.config = &config;
        context.theme = &config.menuTheme;
        context.pathText = pathToUtf8(pendingPath);
        context.projectPath = context.pathText;
        context.errorText = pathError;
        context.dialogButtons = dialogBar.render(config.menuTheme);
        context.hasDialogButtons = true;
        return renderScreen(
            screenThemeFor(config.menuTheme, "confirmCreate"), context);
    });

    auto projectActionsPage = Renderer(actionListComponent, [&] {
        std::string projectPath;
        std::string projectName;
        if (mainSelected >= 0 &&
            mainSelected < static_cast<int>(config.projects.size())) {
            const Project& project =
                config.projects[static_cast<std::size_t>(mainSelected)];
            projectPath = pathToUtf8(config.resolveProjectPath(project));
            projectName = project.name;
        }

        BlockRenderContext context;
        context.config = &config;
        context.theme = &config.menuTheme;
        context.projectPath = projectPath;
        context.projectName = projectName;
        context.pathText = projectPath;
        context.statusText = statusMessage;
        context.statusIsError = statusIsError;
        context.hasSelection = !projectPath.empty();

        Element actions = actionGrid ? actionGrid->Render()
                                       : projectActionsMenu->Render();
        if (actionScroll) {
            actions = addScrollbar(std::move(actions), actionScrollbar);
        }
        if (actionFrame) {
            actions = actions | frame;
        }
        if (actionHeight > 0) {
            actions = actions | size(HEIGHT, LESS_THAN, actionHeight);
        }
        context.actionList = std::move(actions);
        context.hasActionList = true;

        return renderScreen(
            screenThemeFor(config.menuTheme, "projectActions"), context);
    });

    auto confirmRemovePage = Renderer([&] {
        std::string projectPath;
        if (removeIndex >= 0 &&
            removeIndex < static_cast<int>(config.projects.size())) {
            projectPath = pathToUtf8(
                config.resolveProjectPath(config.projects[removeIndex]));
        }

        BlockRenderContext context;
        context.config = &config;
        context.theme = &config.menuTheme;
        context.pathText = projectPath;
        context.projectPath = projectPath;
        context.hasSelection = !projectPath.empty();
        context.dialogButtons = dialogBar.render(config.menuTheme);
        context.hasDialogButtons = true;
        return renderScreen(
            screenThemeFor(config.menuTheme, "confirmRemove"), context);
    });

    auto pages = Container::Tab(
        {mainPage, pathPage, confirmCreatePage, projectActionsPage,
         confirmRemovePage},
        &pageIndex);

    const bool tabOnlySectionSwitch =
        config.settings.tabOnlySectionSwitch;

    // Tab is handled before the page components so a Menu cannot swallow it
    // and keep focus inside the project list. The order is the authored block
    // order, with unavailable sections skipped because their Menus are not
    // focusable.
    auto moveMainFocus = [&](int step, bool wrap) {
        std::vector<bool> enabled(mainFocusOrder.size(), true);
        for (int i = 0; i < static_cast<int>(mainFocusOrder.size()); ++i) {
            enabled[static_cast<std::size_t>(i)] =
                sectionAvailable(mainFocusOrder[static_cast<std::size_t>(i)]);
        }
        const int next = moveEnabledIndex(enabled, mainFocus, step, wrap);
        if (next >= 0) {
            mainFocus = next;
        }
    };

    // Arrow keys move to the section that is actually in that direction. The
    // boxes come from the last draw; when there is no candidate in that
    // direction (Left/Right in a single stacked column) the authored order is
    // the fallback.
    auto sectionBox = [&](int node) -> Box {
        if (node == kMainFocusProjects) {
            return projectListBox;
        }
        if (node == kMainFocusRecent) {
            return recentListBox;
        }
        if (node == kMainFocusQuickInput) {
            return quickInputBox;
        }
        if (node == kMainFocusCwd) {
            return cwdListBox;
        }
        return kEmptyHitBox;
    };

    auto spatialNeighbor = [&](NavDir direction) -> int {
        std::vector<Box> boxes(mainFocusOrder.size(), kEmptyHitBox);
        for (int i = 0; i < static_cast<int>(mainFocusOrder.size()); ++i) {
            const int node = mainFocusOrder[static_cast<std::size_t>(i)];
            if (sectionAvailable(node)) {
                boxes[static_cast<std::size_t>(i)] = sectionBox(node);
            }
        }
        const int index = nearestBoxInDirection(boxes, mainFocus, direction);
        return index < 0
                   ? -1
                   : mainFocusOrder[static_cast<std::size_t>(index)];
    };

    // Returns true when the focus actually moved.
    auto focusDirection = [&](NavDir direction) -> bool {
        const int target = spatialNeighbor(direction);
        if (target >= 0) {
            if (!setFocusNode(target)) {
                return false;
            }
            return true;
        }
        const int before = mainFocus;
        const int step =
            (direction == NavDir::Left || direction == NavDir::Up) ? -1 : 1;
        moveMainFocus(step, false);
        return mainFocus != before;
    };

    // Actions such as E, Del, and move use mainSelected. When the recent list
    // owns the focus, mirror its selection into mainSelected first.
    auto syncSelectionFromFocus = [&]() {
        if (focusIs(kMainFocusRecent) && recentSelected >= 0 &&
            recentSelected <
                static_cast<int>(recentProjectIndices.size())) {
            mainSelected =
                recentProjectIndices[static_cast<std::size_t>(recentSelected)];
        }
    };

    // Mouse support on the main page. Hover focuses the section and the row
    // under the pointer, a left click activates that row like Enter, and a
    // right click opens the project action menu.
    auto handleMainMouse = [&](Event& event) -> bool {
        const Mouse& mouse = event.mouse();
        const int projectRow = mouseRowAt(projectRowBoxes, mouse);
        const int recentRow = mouseRowAt(recentRowBoxes, mouse);
        const bool overProjectList = mouseInside(projectListBox, mouse);
        const bool overRecentList = mouseInside(recentListBox, mouse);
        const bool overQuickInput = mouseInside(quickInputBox, mouse);
        const bool overCwdList = mouseInside(cwdListBox, mouse);
        const bool overCwdRow = mouseInside(cwdRowBox, mouse);

        // A grid owns its own cells: hovering must be able to focus it before
        // it has the keyboard focus, which the component tree cannot do.
        if (projectGrid && overProjectList &&
            projectGrid->OnEvent(event)) {
            return true;
        }
        if (recentGrid && overRecentList && !recentMenuEntries.empty() &&
            recentGrid->OnEvent(event)) {
            return true;
        }

        auto focusProjects = [&]() {
            if (!setFocusNode(kMainFocusProjects)) {
                mainFocus = 0;
            }
        };
        auto focusRecent = [&]() {
            if (!setFocusNode(kMainFocusRecent)) {
                focusProjects();
            }
        };
        auto focusCwd = [&]() {
            if (!setFocusNode(kMainFocusCwd)) {
                focusProjects();
            }
        };

        if (mouse.button == Mouse::WheelUp ||
            mouse.button == Mouse::WheelDown) {
            // Move the focus to the wheeled section and let the Menu itself
            // move the row, so its scrolling frame follows the selection.
            if (projectRow >= 0) {
                focusProjects();
                clearProjectStatus();
                return false;
            }
            if (recentRow >= 0) {
                focusRecent();
                clearProjectStatus();
                return false;
            }
            if (overCwdRow) {
                focusCwd();
                clearProjectStatus();
                return true;
            }
            if (overProjectList) {
                focusProjects();
                return true;
            }
            if (overRecentList && !recentMenuEntries.empty()) {
                focusRecent();
                return true;
            }
            if (overCwdList) {
                focusCwd();
                return true;
            }
            return false;
        }

        if (mouse.button == Mouse::None && mouse.motion == Mouse::Moved) {
            if (projectRow >= 0) {
                focusProjects();
                if (mainSelected != projectRow) {
                    mainSelected = projectRow;
                    clearProjectStatus();
                }
                return true;
            }
            if (recentRow >= 0) {
                focusRecent();
                if (recentSelected != recentRow) {
                    recentSelected = recentRow;
                    clearProjectStatus();
                }
                return true;
            }
            if (overCwdRow) {
                focusCwd();
                clearProjectStatus();
                return true;
            }
            if (overQuickInput) {
                if (!setFocusNode(kMainFocusQuickInput)) {
                    focusProjects();
                }
                return true;
            }
            if (overProjectList) {
                focusProjects();
                return true;
            }
            if (overRecentList && !recentMenuEntries.empty()) {
                focusRecent();
                return true;
            }
            if (overCwdList) {
                focusCwd();
                return true;
            }
            return false;
        }

        if (mouse.button == Mouse::Left && mouse.motion == Mouse::Pressed) {
            if (projectRow >= 0) {
                focusProjects();
                clearProjectStatus();
                if (projectRow < static_cast<int>(config.projects.size())) {
                    mainSelected = projectRow;
                    requestLaunch();
                } else if (showAddProject) {
                    beginAddProject();
                }
                return true;
            }
            if (recentRow >= 0 &&
                recentRow < static_cast<int>(recentProjectIndices.size())) {
                focusRecent();
                recentSelected = recentRow;
                clearProjectStatus();
                requestLaunchIndex(
                    recentProjectIndices[static_cast<std::size_t>(
                        recentRow)]);
                return true;
            }
            if (overCwdRow) {
                focusCwd();
                clearProjectStatus();
                requestCwdLaunch();
                return true;
            }
            if (overProjectList) {
                focusProjects();
                return true;
            }
            if (overRecentList && !recentMenuEntries.empty()) {
                focusRecent();
                return true;
            }
            if (overCwdList) {
                focusCwd();
                return true;
            }
            // Clicks on the quick-launch field belong to the Input component,
            // which moves its text cursor there.
            return false;
        }

        if (mouse.button == Mouse::Right && mouse.motion == Mouse::Pressed) {
            if (projectRow >= 0 &&
                projectRow < static_cast<int>(config.projects.size())) {
                focusProjects();
                mainSelected = projectRow;
                clearProjectStatus();
                beginProjectActions();
                return true;
            }
            if (recentRow >= 0 &&
                recentRow < static_cast<int>(recentProjectIndices.size())) {
                focusRecent();
                recentSelected = recentRow;
                mainSelected =
                    recentProjectIndices[static_cast<std::size_t>(
                        recentRow)];
                clearProjectStatus();
                beginProjectActions();
                return true;
            }
            return false;
        }

        return false;
    };

    // Run the project action at projectActionSelected. Enter and a mouse click
    // both use this path.
    auto activateProjectAction = [&]() {
        if (projectActionSelected < 0 ||
            projectActionSelected >=
                static_cast<int>(config.menuTheme.actions.size())) {
            return;
        }
        const MenuAction& action =
            config.menuTheme.actions[static_cast<std::size_t>(
                projectActionSelected)];
        if (!trim(action.command).empty()) {
            std::string projectPath;
            if (mainSelected >= 0 &&
                mainSelected < static_cast<int>(config.projects.size())) {
                projectPath = pathToUtf8(
                    config.resolveProjectPath(
                        config.projects[static_cast<std::size_t>(
                            mainSelected)]));
            }
            pendingLaunchIndex = mainSelected;
            pendingCommand = expandCommandText(trim(action.command), config,
                                               projectPath);
            launchRequested = true;
            switchPage(Page::Main);
            screen.Exit();
        } else if (action.id == "launch") {
            requestLaunch();
        } else if (action.id == "edit") {
            beginEditProject(PathMode::Edit);
        } else if (action.id == "moveup") {
            moveProject(-1, false);
        } else if (action.id == "movedown") {
            moveProject(1, false);
        } else if (action.id == "movetop") {
            moveProject(-1, true);
        } else if (action.id == "movebottom") {
            moveProject(1, true);
        } else if (action.id == "remove") {
            beginRemove();
        } else if (action.id == "back") {
            switchPage(Page::Main);
        }
    };

    activateActionCallback = activateProjectAction;

    auto pathFieldFocused = [&]() -> PathField {
        const int index = std::clamp(
            pathFocus, 0, static_cast<int>(pathFieldOrder.size()) - 1);
        return pathFieldOrder[static_cast<std::size_t>(index)];
    };
    // Moves the path-screen focus one field in the authored layout order.
    auto movePathField = [&](int step, bool wrap) -> bool {
        const std::vector<bool> enabled(pathFieldOrder.size(), true);
        const int next =
            moveEnabledIndex(enabled, pathFocus, step, wrap);
        if (next < 0) {
            return false;
        }
        pathFocus = next;
        return true;
    };
    // Moves the path-screen focus to the input the theme actually drew in
    // that direction. Using the reflected boxes keeps the arrows correct for
    // any block order, nesting, or horizontal arrangement.
    auto movePathFieldInDirection = [&](NavDir direction) -> bool {
        const int count = static_cast<int>(pathFieldOrder.size());
        if (count == 0) {
            return false;
        }
        std::vector<Box> boxes(static_cast<std::size_t>(count),
                               kEmptyHitBox);
        for (int i = 0; i < count; ++i) {
            boxes[static_cast<std::size_t>(i)] =
                pathFieldOrder[static_cast<std::size_t>(i)] == PathField::Name
                    ? nameFieldBox
                    : pathFieldBox;
        }
        const int current = std::clamp(pathFocus, 0, count - 1);
        const int target =
            nearestBoxInDirection(boxes, current, direction);
        if (target < 0) {
            return false;
        }
        pathFocus = target;
        return true;
    };
    // Page key routing. Each handler owns one page; root() dispatches after
    // the cross-page Shift+Enter handling.

    auto handleMainEvent = [&](Event event) -> bool {
        // Mouse-wheel focus can land on a section that just became empty;
        // pull focus back to the project list.
        if ((focusIs(kMainFocusRecent) && recentMenuEntries.empty()) ||
            (focusIs(kMainFocusCwd) && !cwdVisible)) {
            if (!setFocusNode(kMainFocusProjects)) {
                mainFocus = 0;
            }
        }
        if (event.is_mouse() && handleMainMouse(event)) {
            return true;
        }
        if (event == Event::Tab) {
            moveMainFocus(+1, true);
            return true;
        }
        if (event == Event::TabReverse) {
            moveMainFocus(-1, true);
            return true;
        }

        const bool quickFocused = focusIs(kMainFocusQuickInput);
        if (quickFocused) {
            // Up/Down first walk the wrapped lines of a multi-line value;
            // at the first/last line they switch sections. A single-line
            // field keeps its cursor-to-end behavior when Tab is the only
            // section switch.
            if (event == Event::ArrowUp || event == Event::ArrowDown) {
                const bool down = event == Event::ArrowDown;
                if (quickInput.find('\n') != std::string::npos) {
                    if (cursorHasLine(quickInput, quickInputCursor,
                                      down)) {
                        return false;
                    }
                    if (!tabOnlySectionSwitch) {
                        focusDirection(down ? NavDir::Down : NavDir::Up);
                    }
                    return true;
                }
                if (!tabOnlySectionSwitch) {
                    focusDirection(down ? NavDir::Down : NavDir::Up);
                    return true;
                }
                const bool atBoundary =
                    down ? quickInputCursor >=
                               static_cast<int>(quickInput.size())
                         : quickInputCursor <= 0;
                return atBoundary;
            }
            if (event == Event::ArrowLeft || event == Event::ArrowRight) {
                // Left/Right edit the text; only at the ends do they act
                // like Up/Down and change sections.
                const bool left = event == Event::ArrowLeft;
                const bool atEdge =
                    left ? quickInputCursor <= 0
                         : quickInputCursor >=
                               static_cast<int>(quickInput.size());
                if (!atEdge) {
                    return false;
                }
                if (!tabOnlySectionSwitch) {
                    focusDirection(left ? NavDir::Left : NavDir::Right);
                }
                return true;
            }
            if (event == Event::PageUp || event == Event::PageDown) {
                // These have no meaning in a single-line field, and
                // letting them reach the container would jump sections.
                return true;
            }
            // Esc leaves the input instead of quitting the launcher, and
            // every other key belongs to the text field.
            if (event == Event::Escape) {
                if (!setFocusNode(kMainFocusProjects)) {
                    mainFocus = 0;
                }
                return true;
            }
            return false;
        }
        if (eventMatchesAny(event,
                            config.menuTheme.keys.quickInput)) {
            if (setFocusNode(kMainFocusQuickInput)) {
                return true;
            }
        }
        if ((focusIs(kMainFocusRecent) || focusIs(kMainFocusCwd)) &&
            event == Event::Escape) {
            if (!setFocusNode(kMainFocusProjects)) {
                mainFocus = 0;
            }
            return true;
        }

        // Arrows belong to the focused list while it can still move. Once
        // it reaches an edge, the key moves the focus to the section that
        // is actually in that direction.
        if (event == Event::ArrowUp || event == Event::ArrowDown ||
            event == Event::ArrowLeft || event == Event::ArrowRight) {
            Component focusedList;
            if (focusIs(kMainFocusProjects)) {
                focusedList = projectListComponent;
            } else if (focusIs(kMainFocusRecent)) {
                focusedList = recentListComponent;
            } else if (focusIs(kMainFocusCwd)) {
                focusedList = cwdListComponent;
            }

            if (focusedList && focusedList->OnEvent(event)) {
                clearProjectStatus();
                return true;
            }

            NavDir direction = NavDir::Down;
            if (event == Event::ArrowUp) {
                direction = NavDir::Up;
            } else if (event == Event::ArrowLeft) {
                direction = NavDir::Left;
            } else if (event == Event::ArrowRight) {
                direction = NavDir::Right;
            }
            if (focusDirection(direction)) {
                clearProjectStatus();
                return true;
            }
            if (tabOnlySectionSwitch) {
                return true;
            }
            // Nothing in that direction: keep Container::Vertical's own
            // edge behavior for Up/Down.
            return event == Event::ArrowLeft || event == Event::ArrowRight;
        }
        if (eventMatchesAny(event, config.menuTheme.keys.quit)) {
            screen.Exit();
            return true;
        }
        // E and Delete act on project-list rows. While the cwd section owns
        // the focus they must not act on a stale mainSelected.
        if (focusIs(kMainFocusCwd) &&
            (eventMatchesAny(event, config.menuTheme.keys.projectMenu) ||
             eventMatchesAny(event, config.menuTheme.keys.remove))) {
            return true;
        }
        if (eventMatchesAny(event,
                            config.menuTheme.keys.projectMenu)) {
            syncSelectionFromFocus();
            beginProjectActions();
            return true;
        }
        if (eventMatchesAny(event, config.menuTheme.keys.remove)) {
            syncSelectionFromFocus();
            beginRemove();
            return true;
        }
        if (eventMatchesAny(event, config.menuTheme.keys.add)) {
            beginAddProject();
            return true;
        }
        if (event == Event::ArrowUp || event == Event::ArrowDown) {
            clearProjectStatus();
        }
        return false;
    };

    auto handlePathEvent = [&](Event event) -> bool {
        if (event.is_mouse() && dialogBar.handle(event)) {
            return true;
        }
        if (event == Event::Tab) {
            movePathField(+1, true);
            return true;
        }
        if (event == Event::TabReverse) {
            movePathField(-1, true);
            return true;
        }
        if (event == Event::ArrowUp || event == Event::ArrowDown) {
            const bool down = event == Event::ArrowDown;
            const PathField field = pathFieldFocused();
            const std::string& text =
                field == PathField::Name ? projectName : pathInput;
            const int cursor =
                field == PathField::Name ? nameCursor : pathCursor;
            // Stay inside a multi-line value until its first/last line;
            // only then leave the field.
            if (cursorHasLine(text, cursor, down)) {
                return false;
            }
            movePathFieldInDirection(down ? NavDir::Down : NavDir::Up);
            return true;
        }
        if (event == Event::ArrowLeft || event == Event::ArrowRight) {
            const bool left = event == Event::ArrowLeft;
            const PathField field = pathFieldFocused();
            const std::string& text =
                field == PathField::Name ? projectName : pathInput;
            const int cursor =
                field == PathField::Name ? nameCursor : pathCursor;
            const bool atEdge =
                left ? cursor <= 0
                     : cursor >= static_cast<int>(text.size());
            if (!atEdge) {
                return false;
            }
            movePathFieldInDirection(left ? NavDir::Left
                                          : NavDir::Right);
            return true;
        }
        if (event == Event::PageUp || event == Event::PageDown) {
            // Page keys do not scroll a single FTXUI field.
            movePathField(event == Event::PageDown ? +1 : -1, false);
            return true;
        }
        if (eventMatchesAny(event, config.menuTheme.keys.back)) {
            pathCancelAction();
            return true;
        }
        if (event == Event::Return) {
            pathContinueAction();
            return true;
        }
        return false;
    };

    auto handleConfirmCreateEvent = [&](Event event) -> bool {
        if (event.is_mouse() && dialogBar.handle(event)) {
            return true;
        }
        if (event == Event::Return ||
            eventMatchesAny(event, config.menuTheme.keys.confirm)) {
            createConfirmAction();
            return true;
        }
        if (eventMatchesAny(event, config.menuTheme.keys.cancel)) {
            createBackAction();
            return true;
        }
        return false;
    };

    auto handleProjectActionsEvent = [&](Event event) -> bool {
        if (event.is_mouse() && actionGrid &&
            actionGrid->OnEvent(event)) {
            return true;
        }
        if (event.is_mouse()) {
            const Mouse& mouse = event.mouse();
            const int row = mouseRowAt(actionRowBoxes, mouse);
            if (mouse.button == Mouse::None &&
                mouse.motion == Mouse::Moved) {
                if (row >= 0) {
                    projectActionSelected = row;
                    return true;
                }
                return false;
            }
            if (mouse.button == Mouse::Left &&
                mouse.motion == Mouse::Pressed) {
                if (row >= 0) {
                    projectActionSelected = row;
                    activateProjectAction();
                    return true;
                }
                return false;
            }
            // Wheel events fall through to the Menu so it can move and
            // scroll the selection.
            return false;
        }
        if (eventMatchesAny(event, config.menuTheme.keys.back)) {
            switchPage(Page::Main);
            return true;
        }
        if (event == Event::Return) {
            if (actionGrid) {
                // Let the component tree deliver Enter to the grid.
                return false;
            }
            activateProjectAction();
            return true;
        }
        return false;
    };

    auto handleConfirmRemoveEvent = [&](Event event) -> bool {
        if (event.is_mouse() && dialogBar.handle(event)) {
            return true;
        }
        if (event == Event::Return ||
            eventMatchesAny(event, config.menuTheme.keys.confirm)) {
            removeConfirmAction();
            return true;
        }
        if (eventMatchesAny(event, config.menuTheme.keys.cancel)) {
            removeCancelAction();
            return true;
        }
        return false;
    };

    auto root = CatchEvent(pages, [&](Event event) {
        // Shift+Enter always inserts a literal line break instead of
        // submitting or moving the focus.
        if (isShiftEnter(event)) {
            if (page == Page::Main && focusIs(kMainFocusQuickInput)) {
                insertLineBreak(quickInput, quickInputCursor);
                return true;
            }
            if (page == Page::Path) {
                if (pathFieldFocused() == PathField::Name) {
                    insertLineBreak(projectName, nameCursor);
                } else {
                    insertLineBreak(pathInput, pathCursor);
                }
                return true;
            }
            return false;
        }
        switch (page) {
            case Page::Main:
                return handleMainEvent(event);
            case Page::Path:
                return handlePathEvent(event);
            case Page::ConfirmCreate:
                return handleConfirmCreateEvent(event);
            case Page::ProjectActions:
                return handleProjectActionsEvent(event);
            case Page::ConfirmRemove:
                return handleConfirmRemoveEvent(event);
        }
        return false;
    });

    bool configJustCreated = false;
    if (config.fileWasMissing()) {
        std::string error;
        if (config.save(error)) {
            configJustCreated = true;
            setStatus(
                messageText(config.menuTheme, "configCreated",
                            {{"config", pathToUtf8(config.configFile())}}),
                false);
        } else {
            setStatus(error, true);
        }
    } else if (config.canRepair() && config.needsRepair()) {
        std::string repairError;
        if (config.repair(repairError)) {
            refreshMenu();
            refreshActions();
            updatePathPlaceholder();
            fs::path backup = config.configFile();
            backup += ".before-fix";
            setStatus(
                messageText(config.menuTheme, "configRepaired",
                            {{"backup", pathToUtf8(backup)}}),
                false);
        } else {
            config.log("ERROR", "automatic config repair failed: " +
                                    repairError);
            setStatus(messageText(config.menuTheme, "configRepairFailed",
                                  {{"error", repairError}}),
                      true);
        }
    } else if (config.loadHadError()) {
        setStatus(config.loadWarning(), true);
    }

    if (!bootstrap.messageKey.empty() && !configJustCreated) {
        const std::string text = messageText(
            config.menuTheme, bootstrap.messageKey.c_str(),
            {{"config", bootstrap.messageConfig}});
        if (!statusMessage.empty()) {
            statusMessage += " ";
        }
        statusMessage += text;
    }

    if (!config.themeStatus().empty()) {
        config.log(config.themeStatusIsError() ? "ERROR" : "INFO",
                   "theme: " + config.themeStatus());
        if (config.themeStatusIsError()) {
            if (!statusMessage.empty()) {
                statusMessage += " ";
            }
            statusMessage += config.themeStatus();
            statusIsError = true;
        } else if (statusMessage.empty()) {
            setStatus(config.themeStatus(), false);
        }
    }

    clearScreen();

    // A terminal too small for any page shows one themeable reminder instead of
    // a half-drawn UI. When the reminder itself is wider than the terminal it
    // is panned across the screen so every character can still be read.
    int tinyHintOffset = 0;
    auto tinyHintLastStep = std::chrono::steady_clock::now();
    auto isTinyTerminal = [&](const Dimensions& terminal) {
        const MenuTheme& theme = config.menuTheme;
        if (!theme.tinyHintEnabled || theme.tinyHintText.empty()) {
            return false;
        }
        return (theme.tinyHintMinWidth > 0 &&
                terminal.dimx < theme.tinyHintMinWidth) ||
               (theme.tinyHintMinHeight > 0 &&
                terminal.dimy < theme.tinyHintMinHeight);
    };

    auto pageView = Renderer(root, [&] {
        const Dimensions terminal = Terminal::Size();
        if (!isTinyTerminal(terminal)) {
            tinyHintOffset = 0;
            return root->Render();
        }
        const int textWidth = menuMarkupDisplayWidth(
            config.menuTheme.tinyHintText, &config.menuTheme.variables);
        if (textWidth > terminal.dimx) {
            const auto now = std::chrono::steady_clock::now();
            if (now - tinyHintLastStep >= std::chrono::milliseconds(250)) {
                tinyHintLastStep = now;
                tinyHintOffset = (tinyHintOffset + 1) % textWidth;
            }
            // Keep frames coming until the whole line has scrolled past.
            screen.RequestAnimationFrame();
        } else {
            tinyHintOffset = 0;
        }
        return renderTinyHint(config.menuTheme, terminal.dimx, terminal.dimy,
                              tinyHintOffset);
    });

    auto gatedRoot = CatchEvent(pageView, [&](Event event) {
        // The reminder hides the page, so input must not reach invisible
        // controls. The quit key still leaves the launcher.
        if (isTinyTerminal(Terminal::Size())) {
            if (eventMatchesAny(event, config.menuTheme.keys.quit)) {
                screen.Exit();
            }
            return true;
        }
        return false;
    });

    while (true) {
        screen.Loop(gatedRoot);
        if (!launchRequested) {
            break;
        }
        launchRequested = false;

        const int launchIndex = pendingLaunchIndex;
        pendingLaunchIndex = -1;
        const fs::path oneOffPath = std::move(pendingLaunchPath);
        pendingLaunchPath.clear();
        const std::string command = std::move(pendingCommand);
        pendingCommand.clear();
        clearScreen();

        if (oneOffPath.empty() &&
            (launchIndex < 0 ||
             launchIndex >= static_cast<int>(config.projects.size()))) {
            setStatus(messageText(config.menuTheme, "projectUnavailable"),
                      true);
        } else {
            const bool oneOff = !oneOffPath.empty();
            fs::path projectPath = oneOffPath;
            bool saved = true;
            std::string saveError;

            if (!oneOff) {
                Project& project =
                    config.projects[static_cast<std::size_t>(launchIndex)];
                projectPath = config.resolveProjectPath(project);
                config.log("INFO", "launch requested; project=" +
                                       pathToUtf8(projectPath));
                project.lastUsed = nowTimestamp();
                saved = config.save(saveError);
                if (!saved) {
                    config.log("ERROR", "config save before launch failed: " +
                                            saveError);
                }
            } else {
                config.log("INFO", "quick launch requested; project=" +
                                       pathToUtf8(projectPath));
            }

            LaunchResult result =
                command.empty()
                    ? launchClaude(config, projectPath)
                    : runCustomProjectCommand(config, projectPath, command);

            if (!result.started) {
                setStatus(result.error.empty()
                              ? messageText(config.menuTheme,
                                            "claudeCouldNotStart")
                              : result.error,
                          true);
            } else if (!result.error.empty()) {
                setStatus(result.error, true);
            } else if (result.exitCode == 0) {
                setStatus(messageText(config.menuTheme,
                                      command.empty() ? "claudeExited"
                                                      : "actionExited"),
                          false);
            } else {
                setStatus(messageText(config.menuTheme,
                                      command.empty()
                                          ? "claudeExitedWithCode"
                                          : "actionExitedWithCode",
                                      {{"code",
                                        std::to_string(result.exitCode)}}),
                          true);
            }
            if (!saved) {
                if (!statusMessage.empty()) {
                    statusMessage += " ";
                }
                statusMessage += messageText(
                    config.menuTheme, "configSaveBeforeLaunchFailed",
                    {{"error", saveError}});
                statusIsError = true;
            }
            config.log("INFO", "launcher returned to the menu after launch");
            refreshMenu();
        }

        clearScreen();
        screen = ScreenInteractive::TerminalOutput();
    }

    config.log("INFO", "launcher exiting");
    clearScreen();
    return 0;
}
