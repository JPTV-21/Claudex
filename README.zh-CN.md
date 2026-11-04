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

![ClaudeX 主菜单](docs/screenshots/main-menu.png)

启动器本身是一个 FTXUI 可执行文件，可在 Windows、macOS 和 Linux 上运行。
源码会自动为主机平台选择正确的可执行文件名和目录布局。

> 界面截图中显示的是默认主题的英文文本；界面语言由主题文件决定。

## 重点特性

- **真正便携的 Claude Code**：单个自包含文件夹，内置独立的
  `CLAUDE_CONFIG_DIR`、临时目录和配置。把整个文件夹移动到 U 盘或另一台
  机器，配置和项目列表都保持不变。
- **极强的个性化**：几乎整个界面都由 JSON 主题驱动。页面布局由块
  （`hbox`/`vbox`/`box`）自由组合，内联与整行 markup 控制颜色；自定义
  项目动作、按键绑定、状态消息、边框和分隔线全都是主题字段。甚至可以把
  主界面重建成 Claude Code 风格的欢迎页。

## 主菜单

```text
Claude Code Portable Launcher
────────────────────────────────────────
> F:/Projects/my-project
  ../another-project
  + Add project
────────────────────────────────────────
Quick launch
 F:/Projects/my-project
────────────────────────────────────────
Recent
  F:/Projects/previous-project
────────────────────────────────────────
 Up/Down select, Enter launches, Tab switches sections.
 [Enter] Launch   [E] Project menu   [Del] Remove   [Tab] Switch section
 [A] Add   [Q] Quit
 Launcher: F:/Claudex   Claude: auto
 Log: F:/Claudex/claudex.log
```

- `Enter` 在高亮的项目中启动 Claude。只有获得焦点的区域会显示 `>` 光标和反色
  高亮；其他区域保留自己的选择，但使用非活动前缀绘制。
- `+ Add project` 始终是最后一行，可以用方向键到达。
- 添加/编辑界面有两个字段：可选的显示名称和文件夹路径。方向键会移动到主题
  实际绘制在该方向的输入框；`Up`/`Down` 会先在多行值中逐行移动。
  `Tab`/`Shift+Tab` 按布局顺序在输入框之间切换。Enter 保存，Esc 取消。
  `Shift+Enter` 在任意输入框中插入换行而不提交。名称支持 markup，因此项目
  可以携带自己的颜色。`E` → `Edit project` 会打开两个字段；原来的独立
  `Rename` 动作已移除，因为它编辑的是同一个界面。
- 输入框占位符来自主题的 `pathPlaceholder`（默认
  `{drive}/Projects/my-project`，因此会跟随启动器所在的盘符）和
  `projectNamePlaceholder`。
- `Recent` 区域列出最近启动过的项目，最新的在前。它的大小和位置由主题决定；
  参见 [`recentList`](#custom-screens-full-layout-control)。
- `Quick launch` 是独立区域：一个标题加一行输入。它会在输入的文件夹中启动
  一次 Claude，但不会把该文件夹加入项目列表。文件夹必须已经存在。

其他按键：

| 按键 | 动作 |
| --- | --- |
| `Shift+Enter` | 在获得焦点的输入框中插入换行，而不是提交（终端无法上报 Shift+Enter 时可用 `Alt+Enter`） |
| `Tab` / `Shift+Tab` | 按布局中的出现顺序在区域之间切换焦点 |
| `Up` / `Down` | 在当前区域内移动；到达首行/末行时移到相邻区域 |
| `Left` / `Right` | 在网格行内移动；在单列列表中切换区域。在快速启动和路径界面输入框中，它们移动文本光标，只有到达边缘时才移动到该方向绘制的输入框/区域 |
| `Enter`（快速启动） | 在输入的文件夹中启动一次 Claude |
| `Esc`（快速启动） | 离开输入框而不退出 |
| `E` 或 `F2` | 打开所选项目的动作菜单 |
| `Delete` | 移除所选项目（会先询问） |
| `A` | 打开添加项目界面 |
| `Esc` 或 `Q` | 退出 |
| `Y` / `N` | 确认/取消创建或移除提示 |
| `Enter`（确认对话框） | 等同于 `Y`：创建文件夹或移除项目 |

这些是默认值；可在主题文件中通过 [`keys`](#key-bindings) 重新绑定。
`A` 也会打开添加项目界面，在添加行被隐藏时很有用。

鼠标操作（所有界面）：

| 输入 | 动作 |
| --- | --- |
| 悬停在行或按钮上 | 应用悬停高亮 |
| 悬停在主界面某一行上 | 把焦点和活动光标移到该行 |
| 左键单击一行 | 激活该行，等同于 `Enter` |
| 左键单击按钮 | 执行该按钮的动作（`Continue`、`Cancel`、`Create`、`Back`、`Remove`） |
| 右键单击项目行 | 打开该项目的动作菜单 |
| 滚轮 | 聚焦指针下的区域并移动其选择 |

路径界面显示可点击的 `Continue` / `Cancel` 按钮，创建和移除对话框显示可点击的
`Create` / `Back` 与 `Remove` / `Cancel` 按钮，项目动作菜单（`E`）支持悬停和
点击。键盘快捷键（`Enter`、`Esc`、`Y`、`N`）在所有界面都继续有效。

区域按其在 `screens.main.blocks` 中出现的顺序从上到下遍历，因此调整
`projectList`、`quickInput` 和 `recentList` 块的位置也会改变 Up/Down 和 Tab
的顺序。在配置中把 `tabOnlySectionSwitch` 设为 `true`，可阻止 Up/Down 在列表
首行/末行离开列表；`Tab` 和 `Shift+Tab` 仍会切换区域。默认值为 `false` 时，
单行快速启动输入框中的 `Up`/`Down` 会直接切换区域，而在项目列表或最近列表
边缘会移到相邻区域。

项目动作菜单默认包含 `Launch Claude`、`Edit path`、`Move up`、`Move down`、
`Move to top`、`Move to bottom`、`Remove from list` 和 `Back`；可以用
[`projectMenu.actions`](#custom-project-actions) 替换或扩展。移动动作会在列表
中重新排列所选项目；它们没有专用快捷键，只能从此菜单选择。保存的列表顺序
就是主界面显示的顺序，并会写入配置文件。

启动器会在启动时、启动 Claude 之前、Claude 退出之后以及启动器退出时清除
终端。Windows 上使用 Windows 控制台 API，Linux/macOS 上使用 ANSI 清除序列，
因此 Claude 和 Claudex 不会共用半个屏幕。

仓库内置的 FTXUI 副本禁用了 DECRQSS 光标形状查询，因为不支持的终端会把查询
载荷（`$q q`）打印到屏幕上。它还会把 Windows 的 Shift+Enter 转发为 CSI-u
序列（`\x1b[13;2u`），FTXUI 没有对应的内置事件；启动器用它插入换行。

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
**[JPTV-21/claude-code-portable](https://github.com/JPTV-21/claude-code-portable)**。

它们会下载匹配的 ClaudeX 发布包、获取 Claude Code 二进制文件，并写入便携的
`claudex-config.json`。Windows 可以使用 `setup-portable.ps1`（无需 Git）或
`setup-portable.bat`；Linux 和 macOS 使用 `setup-portable.sh`。交互式提示
（平台、架构、安装目录）、`all` 布局以及无人值守选项请参见该仓库的 README。

## 便携配置

启动器配置默认位于可执行文件旁的 `./claudex-config.json`，主题默认位于
`./claudex-theme.json`。便携 Claude 数据默认位于 `./data/`：

```text
ClaudeX.exe                 (or ClaudeX on macOS/Linux)
claudex-config.json         launcher settings, theme path, and project list
claudex-theme.json          menu text, layout, actions, keys, and messages
claudex.log                 launcher diagnostics log
data/                       portable CLAUDE_CONFIG_DIR (created on launch)
  tmp/                      portable temporary folder (created on launch)
bin/
  x86_64/claude.exe         optional portable Claude binary; set claudePath
  arm64/claude.exe          to "bin/{arch}/claude*" to use this layout
```

主题文件缺失时会根据内置默认主题自动创建。其位置来自配置的 `theme` 字段；
相对主题路径从启动器文件夹开始解析。`theme` 字段以及配置中所有其他路径字段
都支持占位符、环境变量、`~` 和 `*`/`?` 通配符（参见
[路径模板与变量](#path-templates-and-variables)）。默认情况下，启动器先在
自己的文件夹中查找 `claude*`，然后在 `PATH` 中查找；`bin/` 布局需要通过
`claudePath` 显式启用。

相对项目路径保持相对形式，并从启动器文件夹解析；不会仅仅因为指向启动器
文件夹之外就转换为绝对路径。解析后位于启动器文件夹内的相对路径会缩短为
干净的相对形式（不含 `..`）。启动器文件夹内的绝对路径也会以相对形式保存，
以便整个文件夹可以移动；其他绝对路径保持绝对形式。只有在决定是否需要询问
创建缺失文件夹时才会真正解析路径。

使用路径模板输入的（`{root}`、`{drive}`、`{home}`、环境变量或开头的 `~`）
项目路径会以模板形式保存（Windows 反斜杠会转换为正斜杠），因为每次解析
项目时都会再次展开模板。这样同一份配置可以在不同磁盘或机器上使用：例如
添加 `{drive}/Projects/my-project` 会保存该模板，启动器会将其解析为它当前
运行所在的盘符。包含 `*` 或 `?` 通配符的输入在添加项目时仍会解析为唯一的
匹配文件夹，因此保存的是具体路径。

启动器首次运行时会写入 `claudex-config.json` 和 `claudex-theme.json`。可以在
启动器关闭时手动编辑它们。损坏的配置会备份为 `claudex-config.json.invalid`；
损坏的主题会备份为 `claudex-theme.json.invalid` 并替换为默认主题。仍包含内联
`menu` 对象的旧版配置会自动迁移：先合并到主题文件，再从配置中移除。

配置文件位置可以通过 `--config PATH` 或 `CLAUDEX_CONFIG` 更改；两者都接受
占位符和环境变量。所有启动器设置（包括数据位置 `claudeConfigDir` 和
`tempDir`）都是普通配置字段。

配置文件也可以在配置里选择，方式和 `theme` 选择主题文件一样：自己把
`claudex-config.json` 复制到新位置，再把启动时所用的配置里的 `configFile`
设为该路径。启动器只按路径加载，不会自动复制、移动或删除任何文件；原文件
保持不动（可以继续作为指向新文件的小引导）。占位符和环境变量会展开，相对的
`configFile` 从启动器文件夹开始解析。目标不存在时，启动器会在那里创建一份
新的默认配置。引导文件只读取 `configFile` 字段，其他字段会被忽略。
`configFile` 只切换配置文件：主题、`data/`、`tmp/` 和日志仍从启动器文件夹
解析。

已有的 `data/launcher.json` 文件和旧的项目数组格式不会被读取或自动迁移。
如果想把已有文件保留在其他位置，请显式设置配置路径。

### 默认值与启动修复

默认值并没有硬编码在 C++ 中。它们位于 `resources/default-config.json` 和
`resources/default-theme.json`，CMake 在配置阶段将其嵌入可执行文件。启动器
信任这些资源是有效的；不存在第二套回退默认值。

每次启动时，在绘制菜单之前，启动器会：

- 如果解析出的主题文件缺失，创建 `claudex-theme.json`；
- 从默认主题合并缺失的主题键和错误的 JSON 类型，并为之前的主题写入
  `.before-fix` 备份；
- 把无法解析的主题 JSON 备份为 `.invalid`，并替换为合并后的/默认主题
  （如果备份失败则保持原样）；
- 使用默认值添加缺失的配置键，包括缺失的 `claudeExclude`；
- 接受字符串形式的 `launchCommand` 作为两个平台的简写，并用默认对象补齐
  缺失或为空的 `windows`/`posix` 条目；
- 用默认值替换为空的 `theme`、`claudePath`、`launchCommand`、
  `claudeConfigDir` 和 `tempDir`，使保存的配置明确完整；空的 `configFile`
  会保留，因为它表示“不切换”，空的 `logFile` 会保留，因为它会禁用日志，
  空的 `claudeExclude: []` 也会保留，因为它表示“不排除任何内容”；
- 用默认值替换 JSON 类型错误的配置键；
- 保留所有有效的用户值和其他额外的用户键；
- 在修复配置之前写入 `claudex-config.json.before-fix` 备份。

布局不兼容的配置不会被转换。无法解析的配置或主题 JSON，以及项目数组格式的
配置，无法逐键比较，因此只会报告并保持原样；请用当前配置替换它们。

### 日志

默认情况下，启动器会在可执行文件旁写入 `claudex.log`。日志记录启动过程、
解析后的配置/主题路径、配置保存与修复、主题创建与修复、解析出的 Claude
可执行文件、参数、工作目录、便携配置/临时文件夹、启动失败以及 Claude 的
退出码。Claude 的交互式终端输出不会被重定向，因为那会破坏 CLI 界面。

在配置中设置 `logFile` 可以更改日志位置，设置为空字符串则禁用日志。

### 配置字段

```json
{
  "version": 1,
  "theme": "claudex-theme.json",
  "configFile": "",
  "claudePath": ["claude*"],
  "claudeExclude": ["ClaudeX*"],
  "launchCommand": {
    "windows": "set \"CLAUDE_CONFIG_DIR={claudeConfigDir}\" && set \"TEMP={tempDir}\" && set \"TMP={tempDir}\" && set \"TMPDIR={tempDir}\" && {claude}",
    "posix": "CLAUDE_CONFIG_DIR='{claudeConfigDir}' TEMP='{tempDir}' TMP='{tempDir}' TMPDIR='{tempDir}' {claude}"
  },
  "claudeConfigDir": "data",
  "tempDir": "data/tmp",
  "logFile": "claudex.log",
  "tabOnlySectionSwitch": false,
  "defaultProjectName": "",
  "projects": [
    {
      "path": "../my-project",
      "lastUsed": "2026-10-26T12:00:00",
      "name": "<accent>My project</accent>"
    }
  ]
}
```

`name` 是可选的。它支持 markup，并像其他主题字符串一样解析，因此项目可以
使用主题变量携带自己的颜色。同一路径可以多次列出。

| 字段 | 含义 |
| --- | --- |
| `theme` | 主题 JSON 路径。缺失或为空表示 `claudex-theme.json`。支持占位符和通配符。 |
| `configFile` | 要加载的配置文件的可选路径（替代当前文件）。请自己把文件复制过去；启动器只按路径加载，和 `theme` 一样。缺失或为空表示保持当前文件。相对路径从启动器文件夹开始。 |
| `claudePath` | 在哪里查找 Claude。可以是字符串或字符串数组。每个条目可以是文件、目录、通配符路径，或 `claude*` 这样的裸名称/模式。目录条目等价于 `<directory>/claude*`。为空或缺失时使用默认值 `["claude*"]`：先查启动器文件夹，再查 `PATH`。参见 [Claude 可执行文件检测](#claude-executable-detection)。 |
| `claudeExclude` | 用户编写的 Claude 可执行文件检测排除项。可以是字符串或字符串数组。不含路径分隔符（`/`，Windows 上为 `\`）且不会展开为绝对路径的模式会匹配候选文件名（或主文件名）；否则匹配展开后的完整路径。支持 `*` 和 `?`；Windows 上不区分大小写。默认值为 `["ClaudeX*"]`；空数组表示不排除任何内容。 |
| `launchCommand` | 用于启动 Claude 的 shell 命令。字符串会应用于所有平台；对象可以提供 `windows` 和 `posix` 变体。默认对象使用占位符设置 `CLAUDE_CONFIG_DIR`、`TEMP`、`TMP` 和 `TMPDIR`，然后运行 `{claude}`。占位符：`{claude}`、`{claudeConfigDir}`、`{tempDir}` 以及下面的路径标记。不含 `{claude}` 的命令会按原样运行并跳过检测。额外参数可以直接写在命令中，例如 `"{claude} --model sonnet"`。 |
| `claudeConfigDir` | 便携 Claude 配置/数据文件夹。默认为 `data`。相对路径从启动器文件夹开始。 |
| `tempDir` | 默认 `launchCommand` 用作 `TEMP`、`TMP` 和 `TMPDIR` 的便携临时文件夹。默认为 `data/tmp`。 |
| `logFile` | 启动器诊断日志。为空则禁用日志。 |
| `tabOnlySectionSwitch` | 为 `false`（默认）时，Up/Down 也会在列表首行/末行处切换主界面区域。为 `true` 时，只有 Tab/Shift+Tab 切换区域。顺序遵循 `screens.main.blocks` 布局。 |
| `defaultProjectName` | 添加项目时预填到可选名称输入框的默认名字。为空（默认）则保持输入框空白。只影响新增项目，不影响编辑；与手动输入的名字一样会去掉首尾空白，允许 markup。 |
| `projects[].path` | 启动 Claude 的文件夹。相对路径从启动器文件夹开始，保存时保持相对形式。 |
| `projects[].lastUsed` | 启动项目时更新的时间戳。用于最近列表；新添加的项目在首次启动前保持为空。 |

### 主题文件与菜单外观

菜单文本和布局位于 `claudex-theme.json`，由配置的 `theme` 字段选择。下面的
扁平键负责填充文本和样式；`screens` 控制页面结构本身（参见
[自定义屏幕](#custom-screens-full-layout-control)）。默认值位于
`resources/default-theme.json`；缺失的主题键会在启动时自动修复。主题更改只
在启动时读取，因此编辑主题后请重启 Claudex。

标题只有一个：`titleFormat`。颜色和加粗通过标签内联书写，而不是使用单独的
`titleColor`、`titleBold` 等键：

```json
{
  "addProjectLabel": "<success>+ Add project</success>",
  "projectEntryFormat": "<path>",
  "projectNameDisplay": "both",
  "titleFormat": "<bold><orange>------ ClaudeX ------</orange></bold>",
  "titleAlign": "center",
  "borderStyle": "double",
  "borderCharacters": "",
  "separatorStyle": "light",
  "separatorCharacter": "-",
  "activePrefix": "> ",
  "inactivePrefix": "  ",
  "hintText": "<graydark>Up/Down select, Enter launches, Tab switches sections.</graydark>",
  "tinyHintEnabled": true,
  "tinyHintText": "PLEASE GIVE A BIGGER PLACE",
  "tinyHintMinWidth": 30,
  "tinyHintMinHeight": 6,
  "pathPlaceholder": "{drive}/Projects/my-project",
  "projectNamePlaceholder": "Optional display name, markup allowed",
  "recentTitleFormat": "<graydark>Recent</graydark>",
  "recentEntryFormat": "<path>",
  "recentEmptyText": "",
  "recentLimit": 5,
  "quickInputLabel": "<graydark>Quick launch:</graydark>",
  "quickInputPlaceholder": "{drive}/Projects/my-project",
  "pathContinueLabel": "<green>[Enter] Continue</green>",
  "pathCancelLabel": "<graydark>[Esc] Cancel</graydark>",
  "confirmCreateLabel": "<green>[Y] Create</green>",
  "confirmCreateBackLabel": "<graydark>[N] Back</graydark>",
  "confirmRemoveLabel": "<red>[Y] Remove</red>",
  "confirmRemoveBackLabel": "<graydark>[N] Cancel</graydark>",
  "footerAlign": "center",
  "footerLines": [
    "<graydark>[Enter] Launch   [E] Project menu   [Del] Remove</graydark>",
    "<graydark>[Q] Quit</graydark>",
    "<graydark>Launcher: <root>   Claude: <claude></graydark>",
    "<graydark>Log: <log></graydark>"
  ]
}
```

| 键 | 含义 |
| --- | --- |
| `titleFormat` | 字面多行标题。`\n` 开始新行。为空则隐藏标题。 |
| `titleAlign` | 标题对齐：`left`、`center` 或 `right`。 |
| `borderStyle` | `none`、`light`、`dashed`、`heavy`、`double`、`rounded` 或 `empty`。 |
| `borderCharacters` | 自定义边框字形；为空时使用 `borderStyle`。一个字符绘制实线边框；六个字符依次设置左上、上、右上、侧边、左下、右下，例如 `"╭─╮│╰╯"`。六个字形之间允许有空格。 |
| `separatorStyle` | 项目列表周围线条的样式。 |
| `separatorCharacter` | 用重复字符覆盖分隔线样式，例如 `-`。 |
| `activePrefix` / `inactivePrefix` | 活动行和普通行的前缀；允许 markup。只有获得焦点的区域使用 `activePrefix`，因此整个屏幕中恰好只有一行看起来是活动的。 |
| `projectEntryFormat` | 单行项目行的模板。使用 `<path>`/`{path}` 和 `<name>`/`{name}`；允许 markup。`projectList.entryLines`（见下文）会构建多行行，并优先于它。 |
| `projectNameDisplay` | 可选项目名称的绘制位置：`both`（默认；固定列表和最近列表）、`pinned`（仅项目列表）、`recent`（仅最近列表）或 `none`。当列表不显示名称时，`{name}`/`<name>` 展开为空，只由它组成的行会被丢弃。 |
| `addProjectLabel` | 最后一行添加项目行的标签；允许 markup。 |
| `hintText` | 没有状态消息时显示的提示行。 |
| `tinyHintEnabled` | 当终端小于 `tinyHintMinWidth` × `tinyHintMinHeight` 时，改为绘制 `tinyHintText` 而不是各个页面。设为 `false` 可让 Claudex 无论终端多小都尝试绘制正常界面。 |
| `tinyHintText` | 备用提示消息，允许 markup。当它比终端更宽时，会逐列滚动，确保每个字符都能被读到。 |
| `tinyHintMinWidth` / `tinyHintMinHeight` | 低于此终端尺寸时启用备用提示；`0` 表示不按该维度判断。默认分别为 `30` 和 `6`。 |
| `projectNamePlaceholder` | 添加/编辑界面中项目名称字段的提示。路径模板会被展开；允许 markup。 |
| `pathPlaceholder` | 添加项目路径字段的提示。支持 `{drive}`、`{home}`、`{root}` 等路径模板；允许 markup；为空则隐藏。 |
| `footerAlign` / `footerLines` | 页脚对齐方式和完整页脚行列表。 |
| `recentTitleFormat` | `recentList` 块显示的标题。为空则隐藏标题。 |
| `recentEntryFormat` | 最近行模板。支持 `<path>`/`{path}`、`<name>`/`{name}` 和 `<lastUsed>`/`{lastUsed}`；允许 markup。 |
| `recentEmptyText` | `recentList` 块没有条目时显示的文本。为空则隐藏该块。 |
| `recentLimit` | 最近行的默认最大数量；`0` 禁用列表。块的 `limit` 会覆盖它。 |
| `quickInputLabel` | 主界面快速启动输入框前方的标签。允许 markup；为空则隐藏。 |
| `quickInputPlaceholder` | 快速启动输入框中的占位符。路径模板会被展开；允许 markup。输入的路径始终保持字面文本。 |
| `pathContinueLabel` / `pathCancelLabel` | 添加/编辑路径界面上的可点击按钮。允许 markup。 |
| `confirmCreateLabel` / `confirmCreateBackLabel` | “创建此文件夹？”对话框上的可点击按钮。允许 markup。 |
| `confirmRemoveLabel` / `confirmRemoveBackLabel` | “移除项目？”对话框上的可点击按钮。允许 markup。 |
| `variables` | 命名颜色和尺寸（CSS 风格变量），可用于 markup、块颜色以及宽度/高度键；参见 [主题变量](#theme-variables)。 |
| `colors` | 同一张表的旧名称；两者都定义某个名称时，`variables` 优先。 |
| `screens` | 完整页面结构；参见 [自定义屏幕](#custom-screens-full-layout-control)。 |
| `keys` | 键绑定；参见 [键绑定](#key-bindings)。 |

### 主题变量 <a id="theme-variables"></a>

`variables` 是一张命名值表，同时用于颜色和尺寸。字符串值是颜色，整数值是
尺寸。定义可以指向内置颜色、十六进制值、调色板索引、其他名称，或（对于
尺寸）其他数值变量：

```json
"variables": {
  "success": "green",
  "error": "red",
  "warning": "yellow",
  "accent": "#22aaff",
  "listHeight": 10,
  "sideWidth": "listHeight"
}
```

可以把颜色用作标签（`<success>Project added.</success>`）或块颜色
（`"color": "error"`）；可以在任何接受宽度或高度的位置使用尺寸
（`"width": "sideWidth"`）。名称不区分大小写，并忽略 `-`、`_` 和空格，因此
`--success` 和 `success` 是同一个名称。

主题是颜色名称的唯一来源：代码中没有内置列表。随附的默认主题定义了常见
名称（`black`、`red`、`green`、`yellow`、`blue`、`magenta`、`cyan`、
`white`、`graylight`/`gray`/`grey`、`graydark`/`greydark`，以及六个
`*light` 名称），并在它们之上定义了语义名称（`success`、`error`、
`warning`、`accent`、`muted`）。

定义完全属于你：启动修复会补齐缺失的主题键，但绝不会添加或改写
`variables`/`colors` 中的条目。当前主题未定义的名称会保持字面文本（对于
尺寸则回退到该键的默认值）。`colors` 仍可作为同一张表的旧名称使用；两者都
定义某个名称时，`variables` 优先。引用循环会在查找深度处停止，并作为
`variable cycle` 警告写入 `claudex.log`。代码本身绝不会给状态行指定颜色：
消息文本携带自己的 markup，默认主题用 `<success>`、`<error>` 和
`<warning>` 包裹消息。

### 内联菜单 markup

所有可配置的菜单文本都支持嵌套内联标签：

```text
<accent>accent text</accent> <white>111<bold>222</bold>111</white>
```

支持的标签：

- 主题 `variables` 定义的名称，例如 `<success>`、`<error>`、`<muted>`、
  `<red>`，或你添加的任何名称。默认主题附带
  [主题变量](#theme-variables)下列出的常见颜色名称；其他名称（例如
  `orange`）必须先在 `variables` 中定义。名称匹配不区分大小写和分隔符。
- 十六进制颜色：`<#ff8800>text</#ff8800>`（或 `#RGB`）
- 调色板颜色：`<196>text</196>`
- 背景颜色：`<on-red>text</on-red>`、`<bg-#ff8800>text</bg-#ff8800>`、
  `<on196>text</on196>`。`on`/`bg` 前缀接受与前景标签相同的名称、十六进制
  值和调色板索引。
- 样式：`<bold>`/`<b>`、`<dim>`、`<italic>`/`<i>`、
  `<underline>`/`<underlined>`/`<u>`、`<inverted>`/`<inverse>`
- 行指令：`<line ...>` 为整个菜单行设置样式，而不仅仅是标签覆盖的文本。
  `bg=`（也接受 `background=` 或 `on=`）会沿整行宽度绘制背景，包括前缀和
  文本之后的空白；`fg=`（也接受 `foreground=` 或 `color=`）设置默认文字
  颜色；上述样式名称可以作为裸单词组合使用。例如
  `<line bg=#102030 fg=white bold>` 会给一行加上彩色色带。该指令作用于整行，
  `</line>` 是可选的，内联标签在它们覆盖的文本上仍然优先。在纯文本块
  （标题、页脚、状态）中，颜色覆盖文本本身；菜单行和网格单元格会填满其
  完整宽度。无效属性或未知颜色会让指令保持字面文本；裸 `<line>` 仍然是名为
  `line` 的主题变量的普通颜色标签。

标签可以嵌套，并且必须在同一行关闭。未知标签会保持字面文本，因此包含 `<`
或 `>` 的普通项目路径不会受到影响。Markup 在 `<root>` 和 `<project>` 等
占位符展开之后解析。

Markup 适用于 `titleFormat`、`hintText`、`footerLines`、`addProjectLabel`、
`projectEntryFormat`、`activePrefix`/`inactivePrefix`、`pathPlaceholder`、
`projectNamePlaceholder`、`quickInputPlaceholder`、`tinyHintText`、所有
`projectMenu` 文本字段和动作标签、主题 `messages`，以及所有渲染文本的屏幕
块（包括 `status`、`statusOrHint` 和 `error`）。输入框中键入的文本始终按
字面处理，因此路径可以包含 `<` 和 `>`。高亮的项目行仍然使用反色视频。

### 自定义屏幕（完全控制布局） <a id="custom-screens-full-layout-control"></a>

现在每个页面都是 `claudex-theme.json` 中 `screens` 下的一个由块组成的列表。
`resources/default-theme.json` 中的默认主题复现了内置布局，因此这是完整的
页面结构，而不是一个小型主题覆盖。

```json
"screens": {
  "main": {
    "width": 100,
    "align": "center",
    "border": "theme",
    "blocks": [
      { "type": "title" },
      { "type": "separator" },
      { "type": "projectList", "height": 10 },
      {
        "type": "quickInput",
        "title": "<graydark>Quick launch</graydark>",
        "label": "",
        "separatorBefore": true
      },
      { "type": "recentList", "separatorBefore": true },
      { "type": "separator" },
      { "type": "statusOrHint" },
      { "type": "footer" }
    ]
  }
}
```

屏幕名称：`main`、`path`、`confirmCreate`、`projectActions`、
`confirmRemove`。屏幕字段：

| 键 | 含义 |
| --- | --- |
| `blocks` | 有序（且可嵌套）的块列表。 |
| `border` | `none`、`theme`（使用 `borderStyle`/`borderCharacters`）、命名样式（`light`、`dashed`、`heavy`、`double`、`rounded`、`empty`），或自定义字形。 |
| `width` | 页面最大宽度；`0` 表示不限制。 |
| `align` | `left`、`center` 或 `right`。 |
| `addTitle` / `editTitle` | 路径界面上 `{mode}` 的文本。 |

块：

| 类型 | 用途 / 键 |
| --- | --- |
| `title` | 主题标题。`scope`：`main` 或 `project`；可选 `text` 覆盖。 |
| `projectFormat` | `projectMenu.projectFormat` 行。 |
| `text` | 字面或模板文本。`align`、`if`、`unless`。 |
| `separator` | 分隔线。可用 `style`、`character` 覆盖。`vertical: true`（或 `orientation: "vertical"`）绘制一条填满所在行高度的一列线条，因此可用作 `hbox` 列之间的分隔线。 |
| `blank` | 一个空行。 |
| `projectList` | 项目列表。`entryLines`（行模板数组，见下文）、`focusedEntryLines`（焦点行的行模板）、`entryFormat`、`activePrefix`、`inactivePrefix`、`addProject`（默认 `true`）、`inverseHighlight`（默认 `true`）、`maxHeight`（默认 `10`）、`scroll`、`scrollbar`、`frame`，以及下面的网格键。 |
| `recentList` | 最近启动的项目，最新的在前。`entryLines`、`focusedEntryLines`、`limit`（默认 `recentLimit`）、`title`（默认 `recentTitleFormat`）、`titleAlign`（默认 `left`）、`entryFormat`（默认 `recentEntryFormat`）、`activePrefix`、`inactivePrefix`、`inverseHighlight`、`maxHeight`、`scroll`、`scrollbar`、`frame`、`emptyText`（默认 `recentEmptyText`），以及下面的网格键。没有条目时隐藏，除非设置了 `emptyText`。 |
| `quickInput` | 一次性启动的输入行。`title`（在输入框上方绘制的标题，使其像 `recentList` 一样成为独立区域）、`titleAlign`（默认使用块的 `align`）、`label`（默认 `quickInputLabel`；当标题已经表明字段用途时设为 `""`）。输入的文件夹不会加入配置。页面高度不足以同时显示两者时，标题会先收起，让输入框保持可用。 |
| `actionList` | 项目动作列表。`inverseHighlight`（默认 `true`）、`maxHeight`、`scroll`、`scrollbar`、`frame`，以及下面的网格键。 |
| `buttons` | 当前页面的可点击 Back/Confirm 按钮（别名 `dialogButtons`）。按钮标签来自上面的主题键。没有此块的主题会在页面末尾追加按钮。 |
| `pathInput` | 路径输入行。 |
| `status` / `hint` / `statusOrHint` | 状态消息、提示行，或两者（默认）。文本按 markup 渲染。 |
| `error` | 路径错误，或没有错误时的 `fallback` 文本。文本按 markup 渲染。 |
| `footer` | 页脚行。`lines` 数组，或 `scope: "project"` 使用 `projectMenu.footerLines`；`align`。 |
| `box` | 围绕 `children` 的边框。`border`，以及公共布局键。 |
| `hbox` | 水平子元素。每个子元素可以设置 `"flex": true`。 |
| `vbox` | 垂直子元素。 |
| `spacer` | 可伸缩的空白空间（`filler`）。 |

`projectList` 和 `recentList` 各自记住自己的选择，但只有当前拥有键盘焦点的
区域会绘制光标（`activePrefix` 加反色高亮）。用鼠标悬停在某一行或某个区域
上会移动该焦点。

列表可以通过 `entryLines`（一个行模板数组）渲染多行条目。可选的
`focusedEntryLines` 是第二个数组，只用于当前拥有键盘焦点的行：

```json
{ "type": "projectList",
  "entryLines": ["{nameOrPath}"],
  "focusedEntryLines": ["{name}", "<muted>{path}</muted>"] }
```

每一行支持以下占位符，`{...}` 与 `<...>` 两种写法都可以：

| 占位符 | 展开结果 |
| --- | --- |
| `{name}` | 项目的显示名（可能为空）。 |
| `{path}` | 保存的项目路径。 |
| `{lastUsed}` | 最近列表的时间戳（仅最近列表）。 |
| `{nameOrPath}` | 名字有可见文本时用名字；否则用路径。 |
| `{pathIfNamed}` | 只有存在可见名字时才输出路径；否则为空。 |
| `{pathIfUnnamed}` | 只有不存在可见名字时才输出路径；否则为空。 |

名字只有在 markup 渲染后仍有可见文本时才算存在，因此 `<red></red>` 这类
名字等同于没有名字。替换是单次扫描：名字或路径中的 markup、占位符或看似
占位符的文本会原样插入，不会被再次展开。展开后为空的行走会被丢弃；没有任何
剩余行的条目会回退到裸路径。光标前缀放在第一行，续行会缩进以对齐。

每一行的模板优先级：

1. 如果该行是焦点行（所在列表拥有键盘焦点且该行被选中），并且
   `focusedEntryLines` 非空，先尝试它。
2. 如果它没有产生任何可见行，改用基础模板。
3. 基础模板在 `entryLines` 非空时用它；否则用 `entryFormat` 的一行。
4. 只有基础模板也为空时，才回退到裸路径。

`entryLines` 仍然优先于 `entryFormat`。`+ Add project` 行永远不套这些模板。
`projectNameDisplay` 可以在任一列表中抑制名称；被抑制的名称会按“没有名字”
展开，因此 `{nameOrPath}` 变成路径，`{pathIfNamed}` 变成空。

两个列表块都可以使用这些常用显示规则：

```json
// 有名字用名字，没名字用路径。
"entryLines": ["{nameOrPath}"]

// 无论有没有名字都显示路径（无名字时第一行自动消失）。
"entryLines": ["{name}", "<muted>{path}</muted>"]

// 焦点行：名字+路径；其他行：只显示名字（无名则路径）。
"entryLines": ["{nameOrPath}"],
"focusedEntryLines": ["{name}", "<muted>{path}</muted>"]

// 焦点行：只显示名字（无名则路径）；其他行：名字+路径。
"entryLines": ["{name}", "<muted>{path}</muted>"],
"focusedEntryLines": ["{nameOrPath}"]
```

如果希望“只有路径的项目”也获得与名字相同的强调，同时有名字的项目仍显示
暗色路径：

```json
"entryLines": [
  "<bold>{nameOrPath}</bold>",
  "<muted>{pathIfNamed}</muted>"
]
```

有名字时：加粗名字 + 暗色路径；没名字时：加粗路径，第二行自动消失，不会
重复。这正是随附默认主题的写法；如果需要强制白色前景，把 `<bold>` 换成
`<bold><white>`。如果希望只有路径的项目保持暗色，继续使用
`["{name}", "<muted>{path}</muted>"]` 即可。

列表接受网格键：

| 键 | 含义 |
| --- | --- |
| `columns` | `1`（默认，普通列表）、列数，或 `"auto"` 以用尽可能多的单元格填满可用宽度。 |
| `cellWidth` | 固定单元格宽度；`0`（默认）会把每个单元格加宽到最宽条目的宽度。 |
| `columnGap` | 单元格之间的空格数；默认 `2`。 |
| `pageRows` | PageUp/PageDown 移动的行数；默认 `5`。 |

网格使用 `focusedEntryLines` 时，焦点与非焦点两种变体都会参与列宽计算，
因此移动光标不会让整个网格重排。较宽的焦点模板可能让 `columns: "auto"`
选择更少的列；设置固定的 `cellWidth` 可以避免。

在网格中，Up/Down 按行移动，Left/Right 在行内移动。列表只会在第一个和最后
一个条目处把方向键交还给区域切换器，因此它会一直滚动到真正的末尾。
Left/Right 在单列列表上不做任何事，从而留给文本字段使用。

`scroll` 和 `frame` 是每个列表块自己的键；随附的默认主题在三个列表块上都
开启了它们，因此放不下的列表会滚动，而不是被直接裁掉。开启后，高度限制和
滚动条都按实际渲染的行数计算，而不是条目数：一个占两行的条目就占两行。
可选的 `scrollbar` 对象用于设置滚动条样式：

| 键 | 含义 |
| --- | --- |
| `position` | `right`（默认）或 `left`。 |
| `width` | 厚度（列数）；默认 `1`。 |
| `maxWidth` | `width` 的上限；`0`（默认）表示不设上限。 |
| `color` | 前景色，可使用主题变量或字面颜色。未设置时继承列表块的 `color`。 |
| `backgroundColor`（别名 `bgColor`） | 背景色。未设置时跟随列表内容。 |
| `characters` | 单格字形：`full`（占满整行两半）、`top` 和 `bottom`（滑块两端的半格）、`empty`（轨道）。默认依次为 `"┃"`、`"╻"`、`"╹"`、`" "`。 |

```json
{ "type": "projectList",
  "scrollbar": {
    "position": "left",
    "width": 2,
    "color": "accent",
    "characters": { "full": "█", "top": "▄", "bottom": "▀", "empty": "░" }
  } }
```

`top` 和 `bottom` 指的是滑块的两端，因此默认的 `top` 字形 `"╻"` 只填充所在
单元格的下半部分。滚动条始终至少给内容留一列；当页面比 `width` 更窄时，它会
收窄而不是盖住列表。

公共块键：`width`、`height`、`maxWidth`、`maxHeight`、`minWidth`、
`minHeight`、`align`、`color`、`backgroundColor`（别名 `bgColor`）、
`separatorBefore`、`separatorAfter`、`if`、`unless`。

`width` 和 `height` 是精确尺寸：较小的块会用空白填充，较大的块会用 `...`
裁剪。`maxWidth`/`maxHeight` 是上限，因此块会收缩到内容大小，只有太大时
才被裁剪。`minWidth`/`minHeight` 提高下限。这四个都接受变量名。尺寸包含
块自身的边框。放不下的文本会用点号截断，且绝不会超出限制；高度限制丢弃
行的文本块会用同样方式标记最后可见行，而列表则改为滚动。

`color` 和 `backgroundColor` 接受与 markup 颜色相同的值，并应用于整个块，
包括盒子的边框。`separatorBefore`/`separatorAfter` 只在块本身产生内容时
绘制，因此当内容为空时，页眉和页脚的分隔线会自动消失。

`if` / `unless` 的条件：`projects`、`noProjects`、`selected`、
`noSelection`、`status`、`noStatus`、`error`、`noError`、`addProject`、
`noAddProject`、`recent`、`noRecent`、`quickInput`。

所有屏幕文本都会使用常规占位符加上以下内容展开：

| 占位符 | 值 |
| --- | --- |
| `{path}` | 当前路径字段、待处理文件夹或目标项目，具体取决于屏幕。 |
| `{mode}` | 路径界面上的 `addTitle` 或 `editTitle`。 |
| `{error}` | 当前路径错误（如果有）。 |
| `{status}` | 当前状态消息。 |
| `{theme}` | 主题文件路径（为空时为 `disabled`）。 |

例如，一个 Claude Code 风格的主界面，带欢迎卡片且没有外边框：

```json
"screens": {
  "main": {
    "width": 100,
    "align": "left",
    "border": "none",
    "blocks": [
      {
        "type": "box",
        "border": "rounded",
        "width": 74,
        "children": [
          { "type": "text", "text": " <bold><cyan>✻ Welcome to ClaudeX!</cyan></bold>" },
          { "type": "blank" },
          { "type": "text", "text": " <dim>Select a project below, or press A to add one.</dim>" },
          { "type": "blank" },
          { "type": "text", "text": " <dim>cwd:</dim> {path}" }
        ]
      },
      { "type": "blank" },
      { "type": "projectList", "height": 8, "activePrefix": " > ", "inactivePrefix": "   " },
      {
        "type": "quickInput",
        "title": "<dim>Quick launch</dim>",
        "label": "",
        "separatorBefore": true
      },
      { "type": "recentList", "limit": 3, "title": "<dim>Recent</dim>", "separatorBefore": true },
      { "type": "statusOrHint" },
      { "type": "footer", "lines": ["<dim>? for shortcuts</dim>"], "align": "left" }
    ]
  }
}
```

可直接复制的版本位于 `resources/presets/claude-code.menu.json`。把其中的
`screens`、`hintText`、`footerAlign` 和 `footerLines` 键复制到
`claudex-theme.json`，然后重启 Claudex。缺失的键会由主题修复补齐，因此可以
安全地把该预设作为部分主题使用。

列只是嵌套容器。下面的主界面把项目列表放在左侧，中间放一条垂直分隔线，
右侧把最近列表放在快速启动上方：

```json
"screens": {
  "main": {
    "width": 100,
    "blocks": [
      { "type": "title" },
      { "type": "separator" },
      {
        "type": "hbox",
        "children": [
          {
            "type": "box", "border": "theme", "minWidth": 32, "width": 32,
            "children": [ { "type": "projectList" } ]
          },
          { "type": "separator", "vertical": true },
          {
            "type": "vbox", "flex": true,
            "children": [
              { "type": "box", "border": "none",
                "children": [ { "type": "recentList" } ] },
              { "type": "separator" },
              { "type": "box", "border": "none",
                "children": [ { "type": "quickInput", "label": "" } ] }
            ]
          }
        ]
      },
      { "type": "separator" },
      { "type": "statusOrHint" },
      { "type": "footer" }
    ]
  }
}
```

固定列宽只需 `"width": 34`；给会增长的一侧设置 `flex`（或者给两侧都设置
`flex` 以获得等宽列）。每个交互块（`projectList`、`recentList`、
`quickInput`、`actionList`、`pathInput`、`buttons`）在每个屏幕中只存在一次，
`Tab` 仍会按它们在块树中出现的顺序遍历它们。

同样的布局也以现成形式提供：`resources/presets/two-column.theme.json`。
它只是一个预设：启动器绝不会自行加载它。把配置的 theme 字段指向它
（`"theme": "presets/two-column.theme.json"`），或把它的键复制到现有的
`claudex-theme.json` 中即可；修复会补齐预设省略的所有键。

### 项目菜单外观

`E` 项目动作菜单在主题文件的 `projectMenu` 下拥有自己的文本和标签。它复用
主菜单的边框、分隔线、对齐方式、前缀和反色高亮：

```json
"projectMenu": {
  "titleFormat": "<bold><accent>What to do with this project?</accent></bold>",
  "projectFormat": "<muted><project></muted>",
  "launchLabel": "<success>Launch Claude</success>",
  "editLabel": "Edit project",
  "removeLabel": "Remove from list",
  "backLabel": "Back",
  "moveUpLabel": "Move up",
  "moveDownLabel": "Move down",
  "moveTopLabel": "Move to top",
  "moveBottomLabel": "Move to bottom",
  "footerLines": [
    "<muted>[Up/Down] Select   [Enter] Choose   [Esc] Back</muted>"
  ]
}
```

| 键 | 含义 |
| --- | --- |
| `titleFormat` | 动作菜单标题文本。为空则隐藏。 |
| `projectFormat` | 项目路径行。使用 `<project>` 或 `{project}`。为空则隐藏。 |
| `launchLabel` / `editLabel` / `removeLabel` / `backLabel` | 默认动作标签。`edit` 打开两个字段（名称和路径）。仍然列出 `rename` 动作的旧主题会把它合并到 `edit`（如果不存在 `edit` 动作则转换为 `edit`）；此时 `renameLabel` 会被忽略。 |
| `moveUpLabel` / `moveDownLabel` / `moveTopLabel` / `moveBottomLabel` | 四个移动动作的默认标签。 |
| `actions` | 完整动作列表，包括自定义 shell 命令；参见 [自定义项目动作](#custom-project-actions)。 |
| `footerLines` | 动作菜单页脚行。为空则隐藏。 |

四个移动动作会改变所选项目在列表中的位置，并立即保存新顺序。
`moveUp`/`moveDown` 移动一行，`moveTop`/`moveBottom` 移到第一个或最后一个
位置。

#### 自定义项目动作 <a id="custom-project-actions"></a>

`projectMenu.actions` 替换内置动作列表。`id` 选择内置行为；非空的
`command` 则会在项目文件夹中运行 shell 命令：

```json
"actions": [
  { "id": "launch" },
  { "id": "edit", "label": "Change folder" },
  { "id": "moveUp" },
  { "id": "moveDown" },
  { "id": "shell", "label": "Open shell", "command": "pwsh" },
  { "id": "remove", "label": "<red>Remove</red>" },
  { "id": "back" }
]
```

| 字段 | 含义 |
| --- | --- |
| `id` | `launch`、`edit`、`moveUp`、`moveDown`、`moveTop`、`moveBottom`、`remove`、`back`，或任何自定义名称。旧式 `rename` id 会被视为 `edit`，如果已经列出 `edit` 则跳过。 |
| `label` | 行标签；允许 markup。为空时，内置 id 会回退到 `launchLabel`、`editLabel`、对应的移动标签、`removeLabel` 和 `backLabel`。 |
| `command` | 在项目文件夹中运行的 shell 命令。覆盖 `id`。 |

命令在设置便携环境后，Windows 上通过 `cmd.exe /d /s /c`、其他平台通过
`/bin/sh -c` 运行。命令中只展开花括号占位符：`{root}`、`{config}`、
`{theme}`、`{log}`、`{claude}`、`{project}`、`{path}`。尖括号保持不变，
因此 shell 重定向仍然有效。

#### 键绑定 <a id="key-bindings"></a>

主题文件中的 `keys` 把动作映射到按键。每个值是一个字符串或字符串数组：

```json
"keys": {
  "quit": ["q", "esc"],
  "projectMenu": ["e", "f2"],
  "remove": ["delete"],
  "add": ["a"],
  "quickInput": ["tab"],
  "confirm": ["y"],
  "cancel": ["n", "esc"],
  "back": ["esc"]
}
```

| 键 | 作用位置 |
| --- | --- |
| `quit` | 主菜单。 |
| `projectMenu` | 打开所选项目的动作菜单。 |
| `remove` | 请求移除所选项目。 |
| `add` | 打开添加项目界面（即使添加行被隐藏也有效）。 |
| `quickInput` | 跳到主界面的快速启动输入框。`Tab`/`Shift+Tab` 始终按布局顺序在可见的主界面区域之间切换焦点，即使移除了此绑定。 |
| `confirm` | 确认界面（默认为 `y`）。 |
| `cancel` | 确认界面。 |
| `back` | 路径和项目动作界面（默认为 `esc`）。请把它与 `cancel` 分开：其中的字母会输入到路径字段。 |

方向键不是可绑定的动作。在区域内它们移动选择（Down/Up，网格行内为
Left/Right）。在区域边缘，按键会把焦点移到实际位于该方向的区域：目标从
各区域最后绘制的位置中选择，因此 Down 会留在列内，Right 会跨到下一列。
当该方向没有任何区域时（普通堆叠布局左右没有内容），会回退到编排的块顺序。
使用 `tabOnlySectionSwitch` 时，方向键会在边缘停止。在快速启动输入框中，
Left/Right 编辑文本光标，直到到达某一端。

支持的键名：单个字符、`esc`、`enter`、`tab`、`tabreverse`、`space`、
`backspace`、`delete`、`insert`、`home`、`end`、`pageup`、`pagedown`、
`up`、`down`、`left`、`right`，以及 `f1`–`f12`。设置为空数组可禁用绑定。
Up/Down 移动选择，Enter 激活高亮行，除非你显式把这些键绑定到其他动作。

所有主题文本字段（包括屏幕块文本）都同时支持花括号和尖括号占位符：

```text
{root} / <root>         {config} / <config>
{theme} / <theme>       {log} / <log>
{claude} / <claude>     {project} / <project>
```

未配置 `claudePath` 时，`{claude}` 显示 `auto`。在菜单文本中，数组会用
`" ; "` 连接；在 shell 命令中，它展开为第一个条目，因此命令中不会包含
有歧义的列表。

顶层的 `title` 配置字段和 `{title}` 占位符已被移除；请改用主题的
`titleFormat` 和 `pathPlaceholder`。旧配置中遗留的额外键会原样保留并被忽略。

`projectMenu.titleFormat`、`projectMenu.projectFormat` 和
`projectMenu.footerLines` 支持相同的占位符。

### 状态消息

状态行、路径错误、配置错误、主题错误和 Claude 启动错误都可以在主题文件的
`messages` 下配置。默认值位于 `resources/default-theme.json`；缺失的键会在
启动修复期间添加。消息是单行文本，同时支持 `{name}` 占位符和内联 markup
（颜色标签、`<bold>`、`<dim>`，以及主题变量定义的命名颜色）。

```json
"messages": {
  "projectAdded": "<success>Project added.</success>",
  "claudeExitedWithCode": "<success>Claude exited with code {code}.</success>",
  "pathNotFolder": "<error>That path is not a folder: {path}</error>",
  "selectProjectFirst": "<warning>Select a project first.</warning>"
}
```

颜色是消息本身的一部分，而不是由显示它的块硬编码，因此同一状态行可以使用
主题定义的任何颜色。

项目列表和路径输入：

| 键 | 占位符 | 显示时机 |
| --- | --- | --- |
| `selectProjectFirst` | — | 某个动作需要先选择项目。 |
| `projectAdded` | — | 项目已添加并保存。 |
| `projectPathUpdated` | — | 项目路径已编辑并保存。 |
| `projectRemoved` | — | 项目已移除且配置已保存。 |
| `projectAddedSaveFailed` | `{error}` | 项目已添加，但配置无法保存。 |
| `projectUnavailable` | — | 为启动而选择的项目消失了。 |
| `projectNoLongerListed` | — | 正在编辑的项目消失了。 |
| `enterFolderPath` | — | 路径字段被空提交。 |
| `invalidFolderPath` | — | 输入的路径无法解析。 |
| `pathNotFolder` | `{path}` | 输入的路径存在但不是文件夹。 |
| `pathInspectFailed` | `{path}` `{error}` | 无法检查该路径。 |
| `pathCreateFailed` | `{path}` `{error}` | 无法创建缺失的文件夹。 |

配置：

| 键 | 占位符 | 显示时机 |
| --- | --- | --- |
| `configCreated` | `{config}` | 首次运行时写入了缺失的配置。 |
| `configRepaired` | `{backup}` | 配置已自动修复。 |
| `configRepairFailed` | `{error}` | 自动修复失败。 |
| `configSaveBeforeLaunchFailed` | `{error}` | 启动 Claude 之前无法保存配置。 |
| `configInspectFailed` | `{path}` `{error}` | 无法检查配置路径。 |
| `configOpenFailed` | `{path}` | 无法打开配置文件。 |
| `configOldArrayFormat` | — | 发现旧的项目数组配置格式。 |
| `configUnsupportedFormat` | — | 配置不是 JSON 对象。 |
| `configProjectsNotArray` | — | 配置的 `projects` 字段不是数组。 |
| `configParseFailed` | `{path}` `{error}` | 无法解析配置 JSON。 |
| `configCannotRepair` | `{warning}` | 配置无法自动修复。 |
| `configBackupFailed` | `{backup}` `{error}` | 无法写入修复前备份。 |
| `configFolderCreateFailed` | `{path}` `{error}` | 无法创建配置文件夹。 |
| `configSerializeFailed` | `{error}` | 无法序列化配置 JSON。 |
| `configWriteFailed` | `{path}` | 无法写入临时配置文件。 |
| `configFinishWriteFailed` | `{path}` | 写入临时配置文件失败。 |
| `configInvalidBackupFailed` | `{backup}` `{error}` | 无法备份无效配置，因此没有覆盖它。 |
| `configReplaceFailed` | `{path}` `{error}` | 临时配置无法替换真实配置。 |

主题：

| 键 | 占位符 | 显示时机 |
| --- | --- | --- |
| `themeCreated` | `{path}` | 已创建缺失的主题文件。 |
| `themeRepaired` | `{backup}` | 主题已自动修复。 |
| `themeInspectFailed` | `{path}` `{error}` | 无法检查主题路径。 |
| `themeOpenFailed` | `{path}` | 无法打开主题文件。 |
| `themeParseFailed` | `{path}` `{error}` | 无法解析主题 JSON。 |
| `themeUnsupportedFormat` | — | 主题不是 JSON 对象。 |
| `themePathEmpty` | — | 解析出的主题路径为空。 |
| `themeFolderCreateFailed` | `{path}` `{error}` | 无法创建主题文件夹。 |
| `themeBackupFailed` | `{backup}` `{error}` | 无法写入修复前主题备份。 |
| `themeInvalidBackupFailed` | `{backup}` `{error}` | 无法备份无效主题，因此没有覆盖它。 |
| `themeSerializeFailed` | `{error}` | 无法序列化主题 JSON。 |
| `themeWriteFailed` | `{path}` | 无法写入临时主题文件。 |
| `themeFinishWriteFailed` | `{path}` | 写入临时主题文件失败。 |
| `themeReplaceFailed` | `{path}` `{error}` | 临时主题无法替换真实主题。 |

Claude 与启动：

| 键 | 占位符 | 显示时机 |
| --- | --- | --- |
| `claudeExited` | — | Claude 以退出码 0 退出。 |
| `claudeExitedWithCode` | `{code}` | Claude 以非零退出码退出。 |
| `claudeCouldNotStart` | — | 启动失败且没有具体错误。 |
| `claudeProjectMissing` | `{path}` | 项目文件夹不存在。 |
| `claudeNotRunnable` | `{path}` | 配置的 `claudePath` 是文件但不可运行。 |
| `claudePathNotFound` | `{path}` | 配置的单个 `claudePath` 条目未找到，或被 `claudeExclude` 排除。 |
| `claudeNotFound` | — | 所有 `claudePath` 条目和 `PATH` 中都没有未被排除的 Claude。 |
| `claudeAmbiguous` | `{path}` `{candidates}` | 配置的目录中有多个可执行文件，且没有匹配 `claude*` 的。 |
| `pathPatternNoMatch` | `{value}` | 通配符路径没有匹配任何内容。 |
| `pathPatternAmbiguous` | `{value}` `{candidates}` | 项目路径通配符匹配了多个文件夹。 |
| `environmentVariableFailed` | `{name}` `{error}` | 无法为自定义项目动作设置某个便携环境变量。 |
| `createFolderFailed` | `{path}` `{error}` | 无法创建配置或临时文件夹。 |
| `notAFolder` | `{path}` | 某个便携文件夹路径不是目录。 |
| `claudeStartFailed` | `{error}` | 无法启动 Claude 进程。 |
| `claudeWaitFailed` | `{error}` | 等待 Claude 失败。 |
| `claudeExitCodeUnreadable` | `{error}` | 无法读取 Claude 的退出码。 |

状态行仍对普通状态使用绿色、对错误使用红色；消息文本本身不会着色。

### 路径模板与变量 <a id="path-templates-and-variables"></a>

`theme`、`claudePath`、`claudeExclude`（路径模式）、`claudeConfigDir`、
`tempDir`、`logFile`、主题的 `pathPlaceholder`、项目路径，以及 `--config` /
`CLAUDEX_CONFIG` 路径都支持：

| 标记 | 值 |
| --- | --- |
| `{root}` | 启动器文件夹 |
| `{platform}` | `windows`、`macos` 或 `linux` |
| `{arch}` | `x86_64`、`arm64` 或 `x86` |
| `{home}` | 当前用户主目录 |
| `{drive}` | Windows：启动器所在盘符（`F:`）。macOS/Linux：包含启动器的文件系统挂载点，例如 `/Volumes/Disk` 或 `/home`；位于根文件系统时为 `/`。 |

`launchCommand` 使用相同的路径标记，外加 `{claude}`、`{claudeConfigDir}` 和
`{tempDir}`。与路径字段不同，它把环境变量和 `~` 留给 shell 处理。

环境变量也会展开：`%USERPROFILE%` 和 `$HOME` 都有效。开头的 `~` 会展开为
主目录；`{home}` 可以在字符串中的任何位置使用，是更明确的形式。

每个路径值还接受 `*` 和 `?` 通配符，按路径段匹配。例如 `bin/*/claude*` 或
`D:/tools/claude-*/bin`。通配符会在使用路径之前展开；当期望单个值而有多个
路径匹配时，使用排序后的第一个路径。项目路径在添加或编辑时必须恰好匹配
一个文件夹。`claudeExclude` 的完整路径模式是例外：它们匹配整个路径字符串，
因此 `*` 和 `?` 可以跨分隔符。

### Claude 可执行文件检测 <a id="claude-executable-detection"></a>

`claudePath` 默认为 `["claude*"]`。按顺序检查每个条目：

1. 文件路径：直接使用（考虑平台的可执行扩展名）。
2. 目录：等价于 `<directory>/claude*`。平台扩展名来自 Windows 的
   `PATHEXT`，因此 `.exe`、`.cmd` 和 `.bat` 包装器都能被找到。在
   macOS/Linux 上，文件必须可执行。
3. 通配符路径，例如 `./aaa/claude*`：先尝试匹配的可运行文件，然后在匹配的
   目录中查找 `claude*`。
4. 裸名称或模式，例如 `claude` 或 `claude*`：先搜索启动器文件夹，再搜索
   `PATH`。如果启动器旁边存在同名文件或文件夹，则会改为将其视为该路径。

不会自动排除任何内容。默认配置使用 `"claudeExclude": ["ClaudeX*"]` 避免匹配
启动器自身；如果把 `claudeExclude` 设为 `[]`，搜索路径中名为 `ClaudeX.exe`
的启动器可能被选为 Claude。

`claudeExclude` 模式：

- 不含路径分隔符（`/`，Windows 上为 `\`）且不会展开为绝对路径时，匹配任何
  被搜索文件夹中的文件名（或主文件名），例如 `ClaudeX*` 或
  `uninstaller.exe`；
- 含路径分隔符，或会展开为绝对路径时，匹配展开后的完整路径，例如
  `./aaa/uninstaller*`、`{root}/tools/claude-old*` 或 `{root}`；
- 支持 `*` 和 `?`；在完整路径模式中两者都可以跨路径分隔符；Windows 上
  匹配不区分大小写，macOS/Linux 上区分大小写。

被排除的文件和目录会被完全跳过，包括单候选回退和歧义检查。名称模式也会
与被匹配目录的名称进行比较，因此 `ClaudeX*` 可以跳过名为 `ClaudeX-tools`
的目录。

在目录中，启动器会把 `claude*` 与文件名及其主文件名匹配（因此 `claude`
可以匹配 `claude.exe` 和 `claude.cmd`）。如果多个候选匹配，则由平台的
`PATHEXT` 顺序决定；否则使用排序后的第一个名称，并向日志写入一条警告。
如果没有匹配项，且该目录是显式配置的，则接受恰好包含一个可执行文件的目录。
否则启动器会报告歧义，而不是猜测。包含多个不相关可执行文件且没有匹配名称
的目录会被视为错误，而不是猜测。

如果 Claude 位于其他位置，请在配置中设置 `claudePath`。`bin/<arch>` 布局
不再自动搜索；请使用 `"claudePath": "bin/{arch}/claude*"` 或把它加入数组。

### 启动命令

`launchCommand` 是 Claude 启动方式的唯一来源。它可以是一个应用于所有平台的
字符串：

```json
"launchCommand": "{claude} --model sonnet"
```

也可以是一个包含 Windows 和 POSIX 命令的对象：

```json
"launchCommand": {
  "windows": "set \"CLAUDE_CONFIG_DIR={claudeConfigDir}\" && set \"TEMP={tempDir}\" && set \"TMP={tempDir}\" && set \"TMPDIR={tempDir}\" && {claude}",
  "posix": "CLAUDE_CONFIG_DIR='{claudeConfigDir}' TEMP='{tempDir}' TMP='{tempDir}' TMPDIR='{tempDir}' {claude}"
}
```

默认对象设置便携环境，然后运行检测到的 Claude。支持的占位符：

- `{claude}`：通过 `claudePath` 解析出的可执行文件，已按平台 shell 的规则
  加引号；
- `{claudeConfigDir}` 和 `{tempDir}`：解析后的原始路径；请在命令中给它们
  加引号；
- `{root}`、`{platform}`、`{arch}`、`{home}` 和 `{drive}`：与路径模板标记
  相同的值。

启动器会创建 `claudeConfigDir` 和 `tempDir`，展开占位符，并在 Windows 上通过
`cmd.exe /d /s /c`、Linux/macOS 上通过 `/bin/sh -c` 运行命令。它**不会**在
启动 Claude 时自行设置 `CLAUDE_CONFIG_DIR`、`TEMP`、`TMP` 或 `TMPDIR`；
默认命令会显式设置它们。如果替换了命令并省略这些赋值，Claude 将使用环境
中已有的值。命令中的环境变量和开头的 `~` 会留给 shell 处理。

`launchCommand` 中的相对路径词会相对于启动器文件夹解析：

- `./bin/claude-test` 和 `../tools/run.cmd` 会被改写为绝对路径；
- 未加引号的第一个命令词（例如 `bin/claude-test`），或启动器旁边存在的
  文件，也会按同样方式处理；
- 由 `{root}`、`{claudeConfigDir}` 或 `{tempDir}` 产生的词已经是绝对路径，
  如果它是第一个命令词，会被加上引号。

其他词留给 shell，因此标志、URL、环境变量和模式会保持其含义。
`--config=./x` 和 `set "VAR=./x"` 这类形式不会被改写；请把路径写成单独的
词，或使用占位符。

要使用包装器或完全绕过检测，请编写不含 `{claude}` 的命令：

```json
"launchCommand": "wsl claude --model sonnet"
```

使用默认命令时，Claude 进程会收到：

- `CLAUDE_CONFIG_DIR` = `claudeConfigDir`
- `TEMP`、`TMP`、`TMPDIR` = `tempDir`
- 工作目录 = 所选项目文件夹

主题中配置的自定义项目动作仍会从启动器获得便携环境。

## 命令行

```text
ClaudeX [--config PATH] [--where-claude]
```

也可以把 `CLAUDEX_CONFIG` 设置为备用配置文件。选中的配置依次是
`--config`、`CLAUDEX_CONFIG`，最后是默认的 `./claudex-config.json`；主题
默认是旁边的 `./claudex-theme.json`。`--config` 的值和 `CLAUDEX_CONFIG`
都支持占位符、环境变量和通配符；相对配置路径从当前工作目录解析。无论选中
哪个文件，只要其中的 `configFile` 非空，就会像 `theme` 选择主题文件一样
加载它指向的文件。`--where-claude` 会打印解析出的 Claude 可执行
文件并退出，不启动界面，可用于检查 `claudePath` 和 `claudeExclude`；它会
遵循排除模式。`--help` 会打印选项和默认键绑定。

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
