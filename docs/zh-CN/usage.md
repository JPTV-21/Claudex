# 使用

[返回 README](../../README.zh-CN.md) | [English](../usage.md)

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
- 当启动器从自身目录以外的文件夹启动时（例如
  `cd ~/workspace && ~/Claudex`），主界面会在项目列表上方显示
  `Current folder` 区域。若从启动器自己的目录启动，该区域完全隐藏。
  `Enter` 会在该文件夹中一次性启动 Claude，不会加入项目列表；如果该文件夹
  已经在项目列表中，则按普通项目启动并更新其最近使用时间。区域的位置、标题
  和行格式来自主题的 [`cwdList`](themes.md#custom-screens-full-layout-control)
  块。
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
  参见 [`recentList`](themes.md#custom-screens-full-layout-control)。
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
| `Enter`（当前文件夹） | 在进程的当前工作目录中启动一次 Claude |
| `Esc`（快速启动） | 离开输入框而不退出 |
| `E` 或 `F2` | 打开所选项目的动作菜单 |
| `Delete` | 移除所选项目（会先询问） |
| `A` | 打开添加项目界面 |
| `Esc` 或 `Q` | 退出 |
| `Y` / `N` | 确认/取消创建或移除提示 |
| `Enter`（确认对话框） | 等同于 `Y`：创建文件夹或移除项目 |

这些是默认值；可在主题文件中通过 [`keys`](themes.md#key-bindings) 重新绑定。
`A` 也会打开添加项目界面，在添加行被隐藏时很有用。

鼠标操作（所有界面）：

| 输入 | 动作 |
| --- | --- |
| 悬停在行或按钮上 | 应用悬停高亮 |
| 悬停在主界面某一行上 | 把焦点和活动光标移到该行 |
| 左键单击一行 | 激活该行，等同于 `Enter`（`Current folder` 行会在该文件夹中启动 Claude） |
| 左键单击按钮 | 执行该按钮的动作（`Continue`、`Cancel`、`Create`、`Back`、`Remove`） |
| 右键单击项目行 | 打开该项目的动作菜单 |
| 滚轮 | 聚焦指针下的区域并移动其选择 |

路径界面显示可点击的 `Continue` / `Cancel` 按钮，创建和移除对话框显示可点击的
`Create` / `Back` 与 `Remove` / `Cancel` 按钮，项目动作菜单（`E`）支持悬停和
点击。键盘快捷键（`Enter`、`Esc`、`Y`、`N`）在所有界面都继续有效。

区域按其在 `screens.main.blocks` 中出现的顺序从上到下遍历，因此调整
`cwdList`、`projectList`、`quickInput` 和 `recentList` 块的位置也会改变
Up/Down 和 Tab 的顺序。在配置中把 `tabOnlySectionSwitch` 设为 `true`，可阻止
Up/Down 在列表首行/末行离开列表；`Tab` 和 `Shift+Tab` 仍会切换区域。默认值为
`false` 时，单行快速启动输入框中的 `Up`/`Down` 会直接切换区域，而在项目列表
或最近列表边缘会移到相邻区域。

启动焦点遵循 [`defaultFocus`](configuration.md#配置字段) 数组的顺序：主题实际
绘制且当前有内容可聚焦的第一个主界面区域获胜；隐藏或为空的区域会被跳过。

项目动作菜单默认包含 `Launch Claude`、`Edit path`、`Move up`、`Move down`、
`Move to top`、`Move to bottom`、`Remove from list` 和 `Back`；可以用
[`projectMenu.actions`](themes.md#custom-project-actions) 替换或扩展。移动动作会在列表
中重新排列所选项目；它们没有专用快捷键，只能从此菜单选择。保存的列表顺序
就是主界面显示的顺序，并会写入配置文件。

启动器会在启动时、启动 Claude 之前、Claude 退出之后以及启动器退出时清除
终端。Windows 上使用 Windows 控制台 API，Linux/macOS 上使用 ANSI 清除序列，
因此 Claude 和 Claudex 不会共用半个屏幕。

仓库内置的 FTXUI 副本禁用了 DECRQSS 光标形状查询，因为不支持的终端会把查询
载荷（`$q q`）打印到屏幕上。它还会把 Windows 的 Shift+Enter 转发为 CSI-u
序列（`\x1b[13;2u`），FTXUI 没有对应的内置事件；启动器用它插入换行。

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
