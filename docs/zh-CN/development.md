# 开发

[返回 README](../../README.zh-CN.md) | [English](../development.md)

## 构建

要求：CMake 3.16+ 和 C++20 编译器。FTXUI 和 nlohmann/json 已捆绑在
`third_party/` 下，因此不需要网络访问。CMake 在配置阶段把
`resources/default-config.json` 嵌入
`build/generated/default_config.hpp`，把 `resources/default-theme.json` 嵌入
`build/generated/default_theme.hpp`。

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
```

Windows 上使用 MinGW：

```powershell
cmake -S . -B build -G "MinGW Makefiles" `
  -DCMAKE_MAKE_PROGRAM=E:/path/to/mingw32-make.exe `
  -DCMAKE_CXX_COMPILER=E:/path/to/g++.exe `
  -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 8
```

在 MinGW 上，可执行文件会静态链接，可以与 `bin/` 文件夹、
`claudex-config.json`、`claudex-theme.json` 和 `data/` 一起复制，作为自包含
启动器使用。

## 项目结构

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

`claudex_core` 包含所有不需要终端的内容；`claudex_ui` 增加 FTXUI 渲染。只有
`main.cpp` 链接 UI 和进程入口点，这使得单元测试保持无头运行。

## 测试

测试是带有小型内置框架的普通可执行文件；没有外部测试依赖。

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target claudex_tests -j
ctest --test-dir build --output-on-failure
```

也可以直接运行 `build/claudex_tests`。传入子字符串可运行子集
（`claudex_tests path`），或使用 `--list` 打印测试名称。测试为每个用例使用
临时文件夹，并且从不启动界面。设置 `-DCLAUDEX_BUILD_TESTS=OFF` 可跳过构建
测试。
