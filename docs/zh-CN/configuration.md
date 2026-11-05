# 配置

[返回 README](../../README.zh-CN.md) | [English](../configuration.md)

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
  "defaultFocus": {
    "section": "projects",
    "match": "",
    "item": 0
  },
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
| `defaultFocus` | 默认焦点目标。`section` 支持 `projects`（也可写 `projectList`、`project`）、`recent`（也可写 `recentList`、`recentProjects`）、`quickInput`（也可写 `quick`、`input`）、`actions`（也可写 `actionList`、`projectMenu`）、`name`（也可写 `nameInput`）和 `path`（也可写 `pathInput`）。对列表区域，`match` 是各区域自己定义的稳定标识：`projects`/`recent` 匹配项目的存储路径或显示名称（`projects` 中的 `add` 匹配“+ 添加项目”行），`actions` 匹配动作 id 或标签。它优先于 `item`，并且列表重新排序或刷新后仍指向同一个条目。`match` 为空或匹配不到时，`item` 作为从 0 开始的行号回退；负数表示最后一行，超出范围时取边界值。其他页面上的区域（`actions`、`name`、`path`）会在该页面打开时应用目标，不改变主界面焦点。`quickInput` 忽略 `match` 和 `item`；路径输入框忽略两者。默认值为 `{"section": "projects", "match": "", "item": 0}`。 |
| `projects[].path` | 启动 Claude 的文件夹。相对路径从启动器文件夹开始，保存时保持相对形式。 |
| `projects[].lastUsed` | 启动项目时更新的时间戳。用于最近列表；新添加的项目在首次启动前保持为空。 |

`defaultFocus` 可以是一个对象，也可以是对象数组。使用数组时，各页面取自己的
目标：第一个主界面条目（`projects`、`recent` 或 `quickInput`）决定启动焦点，
第一个 `actions` 条目在打开项目动作菜单时使用，第一个 `name`/`path` 条目在
打开添加/编辑页时使用。其他页面的条目会被忽略。

```json
"defaultFocus": [
  { "section": "recent", "match": "F:/work/app" },
  { "section": "actions", "match": "edit" },
  { "section": "path", "match": "name" }
]
```

## 路径模板与变量 <a id="path-templates-and-variables"></a>

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

## Claude 可执行文件检测 <a id="claude-executable-detection"></a>

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

## 启动命令

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
