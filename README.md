<p align="center">
  <img src="resources/claudex-icon.png" width="112" alt="ClaudeX icon">
</p>

<h1 align="center">ClaudeX</h1>

<p align="center">
  English | <a href="README.zh-CN.md">简体中文</a>
</p>

<p align="center">
  <strong>Portable</strong> and deeply customizable, keyboard-first launcher
  for Claude Code — one self-contained folder with an isolated
  <code>CLAUDE_CONFIG_DIR</code>.
</p>

<p align="center">
  <img alt="Unofficial" src="https://img.shields.io/badge/unofficial-not%20affiliated%20with%20Anthropic-orange.svg">
  <a href="LICENSE"><img alt="License: MIT" src="https://img.shields.io/badge/license-MIT-blue.svg"></a>
  <img alt="Platforms: Windows, macOS, Linux" src="https://img.shields.io/badge/platforms-Windows%20%7C%20macOS%20%7C%20Linux-2ea44f.svg">
  <img alt="Architectures: x64, arm64" src="https://img.shields.io/badge/arch-x64%20%7C%20arm64-555.svg">
</p>

> **Unofficial project.** ClaudeX is an independent, community-made launcher.
> It is not affiliated with, endorsed by, or supported by Anthropic. “Claude”
> and “Claude Code” are trademarks of Anthropic. ClaudeX does not bundle Claude
> Code; you need your own installation and account to use it.
>
> **Trust your theme.** `launchCommand` and custom project actions are run by
> the platform shell, so a theme file can execute commands. Only use a
> `claudex-theme.json` from a source you trust.

![ClaudeX demo: launch a project, quit Claude back to the launcher, switch between themes, and carry the whole setup in one folder](docs/screenshots/demo.gif)

The launcher itself is a single FTXUI executable. It works on Windows, macOS,
and Linux. The source chooses the right executable name and layout for the host
platform automatically.

## Highlights

- **Truly portable Claude Code**: one self-contained folder with an isolated
  `CLAUDE_CONFIG_DIR`, temporary folder, and config. Move or copy the whole
  folder to a USB drive or another machine and keep your setup intact.
- **Deeply customizable**: almost the entire UI comes from JSON themes. Page
  layouts are built from blocks (`hbox`/`vbox`/`box`), and inline or whole-row
  markup controls colors; custom project actions, key bindings, status
  messages, borders, and separators are all theme fields. You can even rebuild
  the main screen as a Claude Code-style welcome page.
- **Keyboard-first**: every action has a rebindable key, and the mouse works
  everywhere too. Add, edit, remove, and launch without leaving the terminal.

## Quick start

Download the archive for your platform from the
[latest release](https://github.com/JPTV-21/Claudex/releases/latest):

| Platform | Asset |
| --- | --- |
| Windows x64 | `ClaudeX-windows-x64.zip` |
| Windows arm64 | `ClaudeX-windows-arm64.zip` |
| macOS x64 | `ClaudeX-macos-x64.tar.gz` |
| macOS arm64 | `ClaudeX-macos-arm64.tar.gz` |
| Linux x64 | `ClaudeX-linux-x64.tar.gz` |
| Linux arm64 | `ClaudeX-linux-arm64.tar.gz` |

Extract the archive and run `ClaudeX.exe` (`ClaudeX` on macOS/Linux). The
executable is self-contained; no installer is required. The Windows build links
the C/C++ runtime statically, so it does not need the Visual C++
Redistributable.

### One-step portable setup

The setup scripts live in a separate repository:
**[windblood1667/claude-code-portable](https://github.com/windblood1667/claude-code-portable)**.

They download the matching ClaudeX release, fetch the Claude Code binary, and
write a portable `claudex-config.json`. Windows can use `setup-portable.ps1`
(no Git required) or `setup-portable.bat`; Linux and macOS use
`setup-portable.sh`. See that repository's README for the interactive prompts
(platform, architecture, install directory), the `all` layout, and unattended
options.

## Documentation

| Guide | What it covers |
| --- | --- |
| [Usage](docs/usage.md) | Main menu, keyboard and mouse, command line |
| [Configuration](docs/configuration.md) | Portable layout, config fields, path templates, Claude detection, launch command, logging |
| [Themes](docs/themes.md) | Theme file, variables, markup, full screen layouts, project menu, actions, key bindings, status messages |
| [Development](docs/development.md) | Build, project layout, tests |

Detailed reference material lives in these guides; this page stays a short
overview.
