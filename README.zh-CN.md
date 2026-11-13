<p align="center">
  <img src="resources/claudex-icon.png" width="112" alt="ClaudeX 图标">
</p>

<h1 align="center">ClaudeX</h1>

<p align="center">
  <a href="README.md">English</a> | 简体中文
</p>

<p align="center">
  <strong>便携</strong>且高度可定制、键盘优先的 Claude Code 启动器——单个
  自包含文件夹，配备独立的 <code>CLAUDE_CONFIG_DIR</code>。
</p>

<p align="center">
  <img alt="非官方" src="https://img.shields.io/badge/unofficial-not%20affiliated%20with%20Anthropic-orange.svg">
  <a href="LICENSE"><img alt="License: MIT" src="https://img.shields.io/badge/license-MIT-blue.svg"></a>
  <img alt="Platforms: Windows, macOS, Linux" src="https://img.shields.io/badge/platforms-Windows%20%7C%20macOS%20%7C%20Linux-2ea44f.svg">
  <img alt="Architectures: x64, arm64" src="https://img.shields.io/badge/arch-x64%20%7C%20arm64-555.svg">
</p>

> **非官方项目。** ClaudeX 是一个独立的社区启动器，与 Anthropic 没有隶属关系，
> 也未获得其认可或支持。“Claude”和“Claude Code”是 Anthropic 的商标。
> ClaudeX 不捆绑 Claude Code；你需要自行安装并拥有账号才能使用。
>
> **请信任你的主题文件。** `launchCommand` 和自定义项目操作由平台 shell 执行，
> 因此主题文件等同于可执行代码。请只使用来自可信来源的 `claudex-theme.json`。

![ClaudeX 演示：启动项目、退出 Claude 回到启动器、切换主题，以及把整套环境装进一个文件夹](docs/screenshots/demo.gif)

启动器本身是一个 FTXUI 可执行文件，可在 Windows、macOS 和 Linux 上运行。
源码会自动为主机平台选择正确的可执行文件名和目录布局。

> 演示动图中显示的是默认主题的英文文本；界面语言由主题文件决定。

## 重点特性

- **真正便携的 Claude Code**：单个自包含文件夹，内置独立的
  `CLAUDE_CONFIG_DIR`、临时目录和配置。把整个文件夹移动到 U 盘或另一台
  机器，配置和项目列表都保持不变。
- **极强的个性化**：几乎整个界面都由 JSON 主题驱动。页面布局由块
  （`hbox`/`vbox`/`box`）自由组合，内联与整行 markup 控制颜色；自定义
  项目动作、按键绑定、状态消息、边框和分隔线全都是主题字段。甚至可以把
  主界面重建成 Claude Code 风格的欢迎页。
- **键盘优先**：每个动作都有可重新绑定的按键，鼠标同样可用。添加、编辑、
  删除和启动都可以不离开终端完成。

## 快速开始

从[最新发布](https://github.com/JPTV-21/Claudex/releases/latest)下载适合你
平台的压缩包：

| 平台 | 资产 |
| --- | --- |
| Windows x64 | `ClaudeX-windows-x64.zip` |
| Windows arm64 | `ClaudeX-windows-arm64.zip` |
| macOS x64 | `ClaudeX-macos-x64.tar.gz` |
| macOS arm64 | `ClaudeX-macos-arm64.tar.gz` |
| Linux x64 | `ClaudeX-linux-x64.tar.gz` |
| Linux arm64 | `ClaudeX-linux-arm64.tar.gz` |

解压后运行 `ClaudeX.exe`（macOS/Linux 上为 `ClaudeX`）。可执行文件是自包含的，
不需要安装程序。Windows 构建会静态链接 C/C++ 运行时，因此不需要安装
Visual C++ Redistributable。

### 一步式便携安装

安装脚本位于单独的仓库：
**[windblood1667/claude-code-portable](https://github.com/windblood1667/claude-code-portable)**。

它们会下载匹配的 ClaudeX 发布包、获取 Claude Code 二进制文件，并写入便携的
`claudex-config.json`。Windows 可以使用 `setup-portable.ps1`（无需 Git）或
`setup-portable.bat`；Linux 和 macOS 使用 `setup-portable.sh`。交互式提示
（平台、架构、安装目录）、`all` 布局以及无人值守选项请参见该仓库的 README。

## 文档

| 文档 | 内容 |
| --- | --- |
| [使用](docs/zh-CN/usage.md) | 主菜单、键盘与鼠标、命令行 |
| [配置](docs/zh-CN/configuration.md) | 便携目录布局、配置字段、路径模板、Claude 检测、启动命令、日志 |
| [主题](docs/zh-CN/themes.md) | 主题文件、变量、markup、完整屏幕布局、项目菜单、动作、键绑定、状态消息 |
| [开发](docs/zh-CN/development.md) | 构建、项目结构、测试 |

详细参考放在这些文档里，本页只保留概览。
