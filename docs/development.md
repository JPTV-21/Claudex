# Development

[Back to README](../README.md) | [简体中文](zh-CN/development.md)

## Build

Requirements: CMake 3.16+ and a C++20 compiler. FTXUI and
nlohmann/json are bundled under `third_party/`, so no network access is needed.
CMake embeds `resources/default-config.json` into
`build/generated/default_config.hpp` and `resources/default-theme.json` into
`build/generated/default_theme.hpp` at configure time.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
```

Windows with MinGW:

```powershell
cmake -S . -B build -G "MinGW Makefiles" `
  -DCMAKE_MAKE_PROGRAM=E:/path/to/mingw32-make.exe `
  -DCMAKE_CXX_COMPILER=E:/path/to/g++.exe `
  -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 8
```

On MinGW, the executable is linked statically and can be copied along with a
`bin/` folder, `claudex-config.json`, `claudex-theme.json`, and `data/` as a
self-contained launcher.

## Project layout

```text
src/                    launcher code, built as two static libraries
  text_util.*           trimming, wildcard matching, shell quoting
  platform.*            UTF-8 path/env helpers, executable location
  path_resolver.*       {root}/{drive}/~ expansion, wildcards, launchCommand paths
  config_types.hpp      Settings, Project, MenuTheme, ScreenTheme, ...
  config_json.*         theme/config parsing, merging, default repair
  config_bootstrap.*    config path selection through configFile
  config_manager.*      loading, startup repair, theme writing, logging
  claude_finder.*       Claude executable detection and claudeExclude
  launcher.*            portable folders, child processes, launch commands
  menu_markup.*         inline <tag> markup, borders, text templates
  screens.*             block-based screen rendering from the theme JSON
  key_events.*          theme key specs to FTXUI events
main.cpp                terminal UI and the event loop
tests/                  unit tests (no external framework)
```

`claudex_core` holds everything that does not need a terminal; `claudex_ui`
adds the FTXUI rendering. Only `main.cpp` links against the UI and the process
entry point, which keeps the unit tests headless.

## Tests

The tests are plain executables with a small built-in harness; there is no
external test dependency.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target claudex_tests -j
ctest --test-dir build --output-on-failure
```

`build/claudex_tests` can also be run directly. Pass a substring to run a
subset (`claudex_tests path`) or `--list` to print the test names. Tests use a
temporary folder per case and never start the UI. Set `-DCLAUDEX_BUILD_TESTS=OFF`
to skip building them.
