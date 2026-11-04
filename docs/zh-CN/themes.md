# 主题

[返回 README](../../README.zh-CN.md) | [English](../themes.md)

## 主题文件与菜单外观

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

## 主题变量 <a id="theme-variables"></a>

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

## 内联菜单 markup

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

## 自定义屏幕（完全控制布局） <a id="custom-screens-full-layout-control"></a>

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

## 项目菜单外观

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

### 自定义项目动作 <a id="custom-project-actions"></a>

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

### 键绑定 <a id="key-bindings"></a>

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

## 状态消息

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
