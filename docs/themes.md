# Themes

[Back to README](../README.md) | [简体中文](zh-CN/themes.md)

## Theme file and menu appearance

Menu text and layout live in `claudex-theme.json`, selected by the config's
`theme` field. The flat keys below fill in text and styling; `screens` controls
the page structure itself (see
[Custom screens](#custom-screens-full-layout-control)). Defaults live in
`resources/default-theme.json`; missing theme keys are repaired automatically
at startup. Theme changes are read at startup, so restart Claudex after editing
the theme.

> **Security note.** `launchCommand` and custom project actions are run by the
> platform shell, so a theme file can execute commands. Only use a
> `claudex-theme.json` from a source you trust.

There is one title only: `titleFormat`. Colors and bold are written inline
with tags instead of separate `titleColor`, `titleBold`, etc. keys:

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

| Key | Meaning |
| --- | --- |
| `titleFormat` | Literal multi-line title. `\n` starts a new line. Empty hides the title. |
| `titleAlign` | Title alignment: `left`, `center`, or `right`. |
| `borderStyle` | `none`, `light`, `dashed`, `heavy`, `double`, `rounded`, or `empty`. |
| `borderCharacters` | Custom border glyphs; empty uses `borderStyle`. One character draws a solid border; six characters set top-left, top, top-right, side, bottom-left, bottom-right, for example `"╭─╮│╰╯"`. Spaces between the six glyphs are allowed. |
| `separatorStyle` | Style of the lines around the project list. |
| `separatorCharacter` | Overrides the separator style with a repeated character, for example `-`. |
| `activePrefix` / `inactivePrefix` | Prefixes for the active row and normal rows; markup is allowed. Only the focused section uses `activePrefix`, so exactly one row looks active across the whole screen. |
| `projectEntryFormat` | Project row template for single-line rows. Use `<path>`/`{path}` and `<name>`/`{name}`; markup is allowed. `projectList.entryLines` (see below) builds multi-line rows and takes priority. |
| `projectNameDisplay` | Where the optional project name is drawn: `both` (default; pinned and recent lists), `pinned` (project list only), `recent` (recent list only), or `none`. When a list does not show names, `{name}`/`<name>` expands to nothing and a line made only of it is dropped. |
| `addProjectLabel` | Label for the final add-project row; markup is allowed. |
| `hintText` | Hint line shown when there is no status. |
| `tinyHintEnabled` | Draw `tinyHintText` instead of the pages while the terminal is smaller than `tinyHintMinWidth` × `tinyHintMinHeight`. Set to `false` to always draw the normal UI, however small the terminal is. |
| `tinyHintText` | Fallback message, markup is allowed. When it is wider than the terminal it scrolls a column at a time so every character can still be read. |
| `tinyHintMinWidth` / `tinyHintMinHeight` | Terminal size below which the fallback takes over; `0` disables that dimension. Defaults are `30` and `6`. |
| `projectNamePlaceholder` | Hint shown in the project-name field on the add/edit screen. Path templates are expanded; markup is allowed. |
| `pathPlaceholder` | Hint shown in the add-project path field. Supports path templates such as `{drive}`, `{home}`, and `{root}`; markup is allowed; empty hides it. |
| `footerAlign` / `footerLines` | Footer alignment and complete footer line list. |
| `recentTitleFormat` | Heading shown by a `recentList` block. Empty hides the heading. |
| `recentEntryFormat` | Recent row template. Supports `<path>`/`{path}`, `<name>`/`{name}` and `<lastUsed>`/`{lastUsed}`; markup is allowed. |
| `recentEmptyText` | Text shown when a `recentList` block has no entries. Empty hides the block. |
| `recentLimit` | Default maximum number of recent rows; `0` disables the list. A block's `limit` overrides it. |
| `cwdTitleFormat` | Heading shown by a `cwdList` block. Empty hides the heading. |
| `cwdEntryFormat` | Current-folder row template. Supports `<path>`/`{path}` (the working directory); markup is allowed. |
| `quickInputLabel` | Label in front of the main-screen quick-launch input. Markup is allowed; empty hides it. |
| `quickInputPlaceholder` | Placeholder in the quick-launch input. Path templates are expanded; markup is allowed. Typed paths stay literal. |
| `pathContinueLabel` / `pathCancelLabel` | Clickable buttons on the add/edit path screen. Markup is allowed. |
| `confirmCreateLabel` / `confirmCreateBackLabel` | Clickable buttons on the "create this folder?" dialog. Markup is allowed. |
| `confirmRemoveLabel` / `confirmRemoveBackLabel` | Clickable buttons on the "remove project?" dialog. Markup is allowed. |
| `variables` | Named colors and sizes (CSS-style variables) usable in markup, block colors, and width/height keys; see [Theme variables](#theme-variables). |
| `colors` | Older name for the same table; `variables` wins when both define a name. |
| `screens` | Full page structure; see [Custom screens](#custom-screens-full-layout-control). |
| `keys` | Key bindings; see [Key bindings](#key-bindings). |

## Theme variables

`variables` is one table of named values used by both colors and sizes. String
values are colors; integer values are sizes. Definitions may point at built-in
colors, hex values, palette indexes, other names, or (for sizes) other numeric
variables:

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

Use a color as a tag (`<success>Project added.</success>`) or as a block color
(`"color": "error"`); use a size anywhere a width or height is accepted
(`"width": "sideWidth"`). Names are case-insensitive and ignore `-`, `_`, and
spaces, so `--success` and `success` are the same name.

The theme is the only source of color names: the code has no built-in list.
The shipped default theme defines the common names (`black`, `red`, `green`,
`yellow`, `blue`, `magenta`, `cyan`, `white`, `graylight`/`gray`/`grey`,
`graydark`/`greydark`, and the six `*light` names) and then the semantic ones
(`success`, `error`, `warning`, `accent`, `muted`) on top of them.

Definitions are yours alone: startup repair fills in missing theme keys but
never adds or rewrites entries inside `variables`/`colors`. Names the active
theme does not define stay literal text (or fall back to the key's default for
sizes). `colors` still works as the older name for the same table; when both
define a name, `variables` wins. Reference cycles stop at the lookup depth and
are written to `claudex.log` as a `variable cycle` warning.
The code never assigns a color to a status line itself: the message text
carries its own markup, and the default theme wraps messages in `<success>`,
`<error>`, and `<warning>`.

## Inline menu markup

Any configurable menu text supports nested inline tags:

```text
<accent>accent text</accent> <white>111<bold>222</bold>111</white>
```

Supported tags:

- Names defined by the theme's `variables`, for example `<success>`, `<error>`,
  `<muted>`, `<red>`, or any name you add. The default theme ships the common
  color names listed under [Theme variables](#theme-variables); anything else
  (for example `orange`) must be defined in `variables` first. Names are
  matched case- and separator-insensitively.
- Hex colors: `<#ff8800>text</#ff8800>` (or `#RGB`)
- Palette colors: `<196>text</196>`
- Background colors: `<on-red>text</on-red>`, `<bg-#ff8800>text</bg-#ff8800>`,
  `<on196>text</on196>`. The `on`/`bg` prefix accepts the same names, hex
  values, and palette indexes as foreground tags.
- Styles: `<bold>`/`<b>`, `<dim>`, `<italic>`/`<i>`,
  `<underline>`/`<underlined>`/`<u>`, `<inverted>`/`<inverse>`
- Line directives: `<line ...>` styles a whole menu row instead of only the
  tagged text. `bg=` (also `background=` or `on=`) paints the background
  across the row width, including the prefix and the padding after the text;
  `fg=` (also `foreground=` or `color=`) sets the default text color; the
  style names above can be combined as bare words. For example
  `<line bg=#102030 fg=white bold>` gives a row a colored band. The directive
  applies to the entire line, `</line>` is optional, and inline tags still win
  on the text they cover. In a plain text block (title, footer, status) the
  color covers the text itself; menu rows and grid cells are filled to their
  full width. An invalid attribute or unknown color leaves the directive as
  literal text; a bare `<line>` is still an ordinary color tag for a theme
  variable named `line`.

Tags can be nested and must be closed on the same line. Unknown tags remain
literal text, so ordinary project paths containing `<` or `>` are not harmed.
Markup is parsed after placeholders such as `<root>` and `<project>` are
expanded.

Markup applies to `titleFormat`, `hintText`, `footerLines`, `addProjectLabel`,
`projectEntryFormat`, `cwdTitleFormat`, `cwdEntryFormat`,
`activePrefix`/`inactivePrefix`, `pathPlaceholder`,
`projectNamePlaceholder`, `quickInputPlaceholder`, `tinyHintText`, all
`projectMenu` text fields and action labels, theme `messages`, and every screen
block that renders text (including `status`, `statusOrHint`, and `error`). Text
typed into an input is always literal, so a path can contain `<` and `>`. The
highlighted project row still uses inverse video.

## Custom screens (full layout control)

Every page is now an authored list of blocks under `screens` in
`claudex-theme.json`. The default theme in `resources/default-theme.json`
reproduces the built-in layout, so this is the complete page structure rather
than a small theme override.

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

Screen names: `main`, `path`, `confirmCreate`, `projectActions`,
`confirmRemove`. Screen fields:

| Key | Meaning |
| --- | --- |
| `blocks` | Ordered (and nestable) block list. |
| `border` | `none`, `theme` (use `borderStyle`/`borderCharacters`), a named style (`light`, `dashed`, `heavy`, `double`, `rounded`, `empty`), or custom glyphs. |
| `width` | Maximum page width; `0` means unlimited. |
| `align` | `left`, `center`, or `right`. |
| `addTitle` / `editTitle` | Text for `{mode}` on the path screen. |

Blocks:

| Type | Purpose / keys |
| --- | --- |
| `title` | Theme title. `scope`: `main` or `project`; optional `text` override. |
| `projectFormat` | `projectMenu.projectFormat` line. |
| `text` | Literal or template text. `align`, `if`, `unless`. |
| `separator` | Separator line. `style`, `character` overrides. `vertical: true` (or `orientation: "vertical"`) draws a one-column line that fills the height of its row, so it works as a divider between `hbox` columns. |
| `blank` | One empty row. |
| `projectList` | Project list. `entryLines` (array of line templates, see below), `focusedEntryLines` (templates for the focused row), `entryFormat`, `activePrefix`, `inactivePrefix`, `addProject` (default `true`), `inverseHighlight` (default `true`), `maxHeight` (default `10`), `scroll`, `scrollbar`, `frame`, plus the grid keys below. |
| `recentList` | Recently launched projects, newest first. `entryLines`, `focusedEntryLines`, `limit` (default `recentLimit`), `title` (default `recentTitleFormat`), `titleAlign` (default `left`), `entryFormat` (default `recentEntryFormat`), `activePrefix`, `inactivePrefix`, `inverseHighlight`, `maxHeight`, `scroll`, `scrollbar`, `frame`, `emptyText` (default `recentEmptyText`), plus the grid keys below. Hidden while there are no entries unless `emptyText` is set. |
| `cwdList` | The process working directory as a one-row section. `entryLines`, `focusedEntryLines`, `entryFormat` (default `cwdEntryFormat`), `title` (default `cwdTitleFormat`), `titleAlign` (default `left`), `activePrefix`, `inactivePrefix`, `inverseHighlight`, `maxHeight`. Hidden when the launcher starts from its own folder. Row templates use `{path}`; other block text can use `{cwd}`. `Enter` launches Claude there once without writing the project list. |
| `quickInput` | Input row for a one-time launch. `title` (heading drawn above the input, which makes it a section like `recentList`), `titleAlign` (default the block `align`), `label` (default `quickInputLabel`; set to `""` when the title already names the field). The typed folder is not added to the config. When the page is too short to show both, the title collapses first so the input stays usable. |
| `actionList` | Project action list. `inverseHighlight` (default `true`), `maxHeight`, `scroll`, `scrollbar`, `frame`, plus the grid keys below. |
| `buttons` | Clickable Back/Confirm buttons for the current page (alias `dialogButtons`). The button labels come from the theme keys above. Themes without this block get the buttons appended at the end of the page. |
| `pathInput` | The path input row. |
| `status` / `hint` / `statusOrHint` | Status message, hint line, or either (the default). Text is rendered as markup. |
| `error` | Path error, or `fallback` text when there is no error. Text is rendered as markup. |
| `footer` | Footer lines. `lines` array, or `scope: "project"` for `projectMenu.footerLines`; `align`. |
| `box` | Border around `children`. `border`, plus common layout keys. |
| `hbox` | Horizontal children. Each child may set `"flex": true`. |
| `vbox` | Vertical children. |
| `spacer` | Flexible empty space (`filler`). |

`projectList`, `recentList`, and `cwdList` each remember a selection, but only
the section that currently has keyboard focus draws its cursor (`activePrefix`
plus the inverse highlight). Hovering a row or a section with the mouse moves
that focus.

Lists can render multi-line entries with `entryLines`, an array of line
templates. `focusedEntryLines` is an optional second array used only for the
row that has keyboard focus:

```json
{ "type": "projectList",
  "entryLines": ["{nameOrPath}"],
  "focusedEntryLines": ["{name}", "<muted>{path}</muted>"] }
```

Each line supports these placeholders, with either `{...}` or `<...>`
spelling:

| Placeholder | Expands to |
| --- | --- |
| `{name}` | The project's display name (may be empty). |
| `{path}` | The stored project path. |
| `{lastUsed}` | The recent-list timestamp (recent list only). |
| `{nameOrPath}` | The name when it has visible text; otherwise the path. |
| `{pathIfNamed}` | The path only when a visible name exists; otherwise empty. |
| `{pathIfUnnamed}` | The path only when no visible name exists; otherwise empty. |

A name counts as present only when its markup renders to visible text, so a
name such as `<red></red>` behaves like no name. Substitution is a single
pass: markup, placeholders, or token-looking text inside a name or path are
inserted literally and never expanded again. A line that expands to nothing is
dropped; an entry with no lines left falls back to its bare path. The cursor
prefix goes on the first line and continuation lines are indented to match.

Template priority for each row:

1. If the row is focused (its list has keyboard focus and the row is selected)
   and `focusedEntryLines` is non-empty, its lines are tried first.
2. If that produces no visible line, the base template is used instead.
3. The base template is `entryLines` when non-empty; otherwise one line from
   `entryFormat`.
4. Only when that is empty too does the row fall back to the bare path.

`entryLines` still takes priority over `entryFormat`. The `+ Add project` row
never uses either template. `projectNameDisplay` can suppress the name in
either list; a suppressed name expands as if the project had no name, so
`{nameOrPath}` becomes the path and `{pathIfNamed}` becomes empty.

Common display rules, for either list block:

```json
// Name when present, path when not.
"entryLines": ["{nameOrPath}"]

// Always show the path (the name line disappears for unnamed projects).
"entryLines": ["{name}", "<muted>{path}</muted>"]

// Focused row: name and path. Other rows: name only (path when unnamed).
"entryLines": ["{nameOrPath}"],
"focusedEntryLines": ["{name}", "<muted>{path}</muted>"]

// Focused row: name only (path when unnamed). Other rows: name and path.
"entryLines": ["{name}", "<muted>{path}</muted>"],
"focusedEntryLines": ["{nameOrPath}"]
```

To make a path-only project use the same emphasis as a named one while still
showing the muted path for named projects:

```json
"entryLines": [
  "<bold>{nameOrPath}</bold>",
  "<muted>{pathIfNamed}</muted>"
]
```

A named project renders a bold name plus a muted path. An unnamed project
renders a bold path and no second line. This is the shipped default theme; use
`<bold><white>` instead of `<bold>` if you also want to force a white
foreground. To keep path-only rows muted, simply keep the earlier
`["{name}", "<muted>{path}</muted>"]` form.

Lists accept grid keys:

| Key | Meaning |
| --- | --- |
| `columns` | `1` (default, a plain list), a number of columns, or `"auto"` to fill the available width with as many cells as fit. |
| `cellWidth` | Fixed cell width; `0` (default) widens every cell to the widest entry. |
| `columnGap` | Spaces between cells; default `2`. |
| `pageRows` | Rows moved by PageUp/PageDown; default `5`. |

When `focusedEntryLines` is used in a grid, both the focused and unfocused
variants are measured for the column width, so moving the cursor never reflows
the grid. A wider focused template can therefore make `columns: "auto"` choose
fewer columns; set a fixed `cellWidth` to avoid that.

In a grid, Up/Down move by a row and Left/Right move within a row. A list only
hands an arrow key back to the section switcher at its first and last entry, so
it keeps scrolling until it really ends. Left/Right do nothing on a
single-column list, which leaves them free for the text field.

`scroll` and `frame` are per-block keys; the shipped default theme enables them
on all three list blocks, so a list that no longer fits in the page scrolls
instead of being clipped. When enabled, height limits and the scrollbar count
rendered lines, not entries: a two-line entry occupies two rows. An optional
`scrollbar` object styles the bar:

| Key | Meaning |
| --- | --- |
| `position` | `right` (default) or `left`. |
| `width` | Thickness in columns; default `1`. |
| `maxWidth` | Upper bound for `width`; `0` (default) leaves it uncapped. |
| `color` | Foreground color, as a theme variable or literal. Unset inherits the list block's `color`. |
| `backgroundColor` (alias `bgColor`) | Background color. Unset follows the list content. |
| `characters` | One-cell glyphs: `full` (both halves of a row), `top` and `bottom` (the half-covered rows at the thumb's ends), and `empty` (the track). Defaults are `"┃"`, `"╻"`, `"╹"`, and `" "`. |

```json
{ "type": "projectList",
  "scrollbar": {
    "position": "left",
    "width": 2,
    "color": "accent",
    "characters": { "full": "█", "top": "▄", "bottom": "▀", "empty": "░" }
  } }
```

`top` and `bottom` name the end of the thumb, so the default `top` glyph `"╻"`
fills the lower half of its cell. A bar always leaves at least one column for
the content; on a page narrower than `width`, it shrinks rather than covering
the list.

Common block keys: `width`, `height`, `maxWidth`, `maxHeight`, `minWidth`,
`minHeight`, `align`, `color`, `backgroundColor` (alias `bgColor`),
`separatorBefore`, `separatorAfter`, `if`, `unless`.

`width` and `height` are exact sizes: a smaller block is padded with blanks and
a larger one is cut with `...`. `maxWidth`/`maxHeight` are upper bounds, so the
block shrinks to its content and is only cut when it is too big.
`minWidth`/`minHeight` raise the lower bound. All four accept a variable name.
Sizes include the block's own border. Text that does not fit is truncated with
dots that never push past the limit; a text block whose height limit drops
lines marks the last visible line the same way, while lists scroll instead.

`color` and `backgroundColor` accept the same values as markup colors and
apply to the whole block, including a box border.
`separatorBefore`/`separatorAfter` only draw when the block itself produces
content, so header and footer separators disappear automatically when their
content is empty.

Conditions for `if` / `unless`: `projects`, `noProjects`, `selected`,
`noSelection`, `status`, `noStatus`, `error`, `noError`, `addProject`,
`noAddProject`, `recent`, `noRecent`, `quickInput`, `cwd`, `noCwd`.

All screen text is expanded with the usual placeholders plus:

| Placeholder | Value |
| --- | --- |
| `{path}` | Current path field, pending folder, or target project, depending on the screen. |
| `{mode}` | `addTitle` or `editTitle` on the path screen. |
| `{error}` | Current path error, if any. |
| `{status}` | Current status message. |
| `{cwd}` | Process working directory, or an empty string when the cwd section is hidden. |
| `{theme}` | Theme file path (`disabled` when empty). |

For example, a Claude Code style main screen with a welcome card and no
outer border:

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

A ready-to-copy version is in `resources/presets/claude-code.menu.json`. Copy
its `screens`, `hintText`, `footerAlign`, and `footerLines` keys into
`claudex-theme.json`, then restart Claudex. Missing keys are filled in by theme
repair, so it is safe to use the preset as a partial theme.

Columns are just nested containers. This main screen puts the project list on
the left, a vertical divider in the middle, and the recent list above the quick
launch on the right:

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

A fixed column width is just `"width": 34`; give the growing side `flex` (or
give both sides `flex` for equal columns). Each interactive block
(`projectList`, `recentList`, `quickInput`, `actionList`, `pathInput`,
`buttons`) exists once per screen, and `Tab` still walks them in the order they
appear in the block tree.

The same layout is available ready-made as
`resources/presets/two-column.theme.json`. It is only a preset: the launcher
never loads it on its own. Use it by pointing the config's theme field at it
(`"theme": "presets/two-column.theme.json"`), or copy its keys into the
existing `claudex-theme.json`; repair fills in every key the preset omits.

## Project menu appearance

The `E` project action menu has its own text and labels under `projectMenu` in
the theme file. It reuses the main menu's border, separators, alignment,
prefixes, and inverse highlight:

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

| Key | Meaning |
| --- | --- |
| `titleFormat` | Action menu title text. Empty hides it. |
| `projectFormat` | Project path line. Use `<project>` or `{project}`. Empty hides it. |
| `launchLabel` / `editLabel` / `removeLabel` / `backLabel` | Default action labels. `edit` opens both fields (name and path). Older themes that still list a `rename` action get it folded into `edit` (or converted to it when no `edit` action exists); `renameLabel` is then ignored. |
| `moveUpLabel` / `moveDownLabel` / `moveTopLabel` / `moveBottomLabel` | Default labels for the four move actions. |
| `actions` | Full action list, including custom shell commands; see [Custom project actions](#custom-project-actions). |
| `footerLines` | Action menu footer lines. Empty hides them. |

The four move actions change the selected project's position in the list and
save the new order immediately. `moveUp`/`moveDown` move it one row, while
`moveTop`/`moveBottom` move it to the first or last position.

### Custom project actions

`projectMenu.actions` replaces the built-in action list. `id` selects built-in
behavior; a non-empty `command` runs a shell command in the project folder
instead:

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

| Field | Meaning |
| --- | --- |
| `id` | `launch`, `edit`, `moveUp`, `moveDown`, `moveTop`, `moveBottom`, `remove`, `back`, or any custom name. A legacy `rename` id is treated as `edit` and skipped when `edit` is already listed. |
| `label` | Row label; markup is allowed. Built-in ids fall back to `launchLabel`, `editLabel`, the matching move label, `removeLabel`, and `backLabel` when empty. |
| `command` | Shell command run in the project folder. Overrides `id`. |

Commands run through `cmd.exe /d /s /c` on Windows and `/bin/sh -c`
elsewhere, after the portable environment is set. Only brace placeholders are
expanded in commands: `{root}`, `{config}`, `{theme}`, `{log}`, `{claude}`,
`{project}`, `{path}`. Angle brackets are left alone so shell redirection
still works.

### Key bindings

`keys` in the theme file maps actions to keys. Each value is a string or array
of strings:

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

| Key | Where it applies |
| --- | --- |
| `quit` | Main menu. |
| `projectMenu` | Opens the selected project's action menu. |
| `remove` | Asks to remove the selected project. |
| `add` | Opens the add-project screen (works even when the add row is hidden). |
| `quickInput` | Jumps to the main-screen quick-launch input. `Tab`/`Shift+Tab` always cycle focus between the visible main-screen sections in layout order, even if this binding is removed. |
| `confirm` | Confirmation screens (`y` by default). |
| `cancel` | Confirmation screens. |
| `back` | Path and project action screens (`esc` by default). Keep this separate from `cancel`: letters there are typed into the path field. |

Arrow keys are not bindable actions. Inside a section they move its selection
(Down/Up, and Left/Right inside a grid row). At a section's edge the key moves
the focus to the section that actually lies in that direction: the target is
picked from the sections' last drawn positions, so Down stays in the column and
Right crosses to the next column. When nothing lies in that direction (a plain
stacked layout has nothing left or right) the authored block order is the
fallback. With `tabOnlySectionSwitch`, arrows stop at the edge instead. In the
quick-launch field Left/Right edit the text cursor until it reaches an end.

Supported key names: single characters, `esc`, `enter`, `tab`, `tabreverse`,
`space`, `backspace`, `delete`, `insert`, `home`, `end`, `pageup`,
`pagedown`, `up`, `down`, `left`, `right`, and `f1`–`f12`. Set an empty array
to disable a binding. Up/Down move the selection and Enter activates the
highlighted row unless you explicitly bind those keys to something else.

All theme text fields, including screen block text, support both brace and
angle placeholders:

```text
{root} / <root>         {config} / <config>
{theme} / <theme>       {log} / <log>
{claude} / <claude>     {project} / <project>
```

`{claude}` shows `auto` when `claudePath` was not configured. With an array it
is joined with `" ; "` in menu text; in shell commands it expands to the first
entry, so a command cannot contain an ambiguous list.

The top-level `title` config field and the `{title}` placeholder are gone; use
the theme's `titleFormat` and `pathPlaceholder` instead. An extra key left in an
old config is kept as-is and ignored.

`projectMenu.titleFormat`, `projectMenu.projectFormat`, and
`projectMenu.footerLines` support the same placeholders.

## Status messages

Status lines, path errors, config errors, theme errors, and Claude launch
errors are configurable under `messages` in the theme file. Defaults live in
`resources/default-theme.json`; missing keys are added during startup repair.
Messages are single-line text and support both `{name}` placeholders and
inline markup (color tags, `<bold>`, `<dim>`, and the named colors defined by
the theme variables).

```json
"messages": {
  "projectAdded": "<success>Project added.</success>",
  "claudeExitedWithCode": "<success>Claude exited with code {code}.</success>",
  "pathNotFolder": "<error>That path is not a folder: {path}</error>",
  "selectProjectFirst": "<warning>Select a project first.</warning>"
}
```

Color is part of the message itself, not hardcoded by the block that shows it,
so the same status line can use any color the theme defines.

Project list and path entry:

| Key | Placeholders | Shown when |
| --- | --- | --- |
| `selectProjectFirst` | — | An action needs a selected project. |
| `projectAdded` | — | A project was added and saved. |
| `projectPathUpdated` | — | A project path was edited and saved. |
| `projectRemoved` | — | A project was removed and the config was saved. |
| `projectAddedSaveFailed` | `{error}` | A project was added but the config could not be saved. |
| `projectUnavailable` | — | The project selected for launching disappeared. |
| `projectNoLongerListed` | — | The project being edited disappeared. |
| `enterFolderPath` | — | The path field was submitted empty. |
| `invalidFolderPath` | — | The entered path could not be parsed. |
| `pathNotFolder` | `{path}` | The entered path exists but is not a folder. |
| `pathInspectFailed` | `{path}` `{error}` | The path could not be inspected. |
| `pathCreateFailed` | `{path}` `{error}` | The missing folder could not be created. |

Config:

| Key | Placeholders | Shown when |
| --- | --- | --- |
| `configCreated` | `{config}` | A missing config was written on first run. |
| `configRepaired` | `{backup}` | The config was repaired automatically. |
| `configRepairFailed` | `{error}` | Automatic repair failed. |
| `configSaveBeforeLaunchFailed` | `{error}` | The config could not be saved before launching Claude. |
| `configInspectFailed` | `{path}` `{error}` | The config path could not be inspected. |
| `configOpenFailed` | `{path}` | The config file could not be opened. |
| `configOldArrayFormat` | — | The old project-array config format was found. |
| `configUnsupportedFormat` | — | The config is not a JSON object. |
| `configProjectsNotArray` | — | The config's `projects` field is not an array. |
| `configParseFailed` | `{path}` `{error}` | The config JSON could not be parsed. |
| `configCannotRepair` | `{warning}` | The config cannot be repaired automatically. |
| `configBackupFailed` | `{backup}` `{error}` | The pre-repair backup could not be written. |
| `configFolderCreateFailed` | `{path}` `{error}` | The config folder could not be created. |
| `configSerializeFailed` | `{error}` | The config JSON could not be serialized. |
| `configWriteFailed` | `{path}` | The temporary config file could not be written. |
| `configFinishWriteFailed` | `{path}` | Writing the temporary config file failed. |
| `configInvalidBackupFailed` | `{backup}` `{error}` | An invalid config could not be backed up, so it was not overwritten. |
| `configReplaceFailed` | `{path}` `{error}` | The temporary config could not replace the real one. |

Theme:

| Key | Placeholders | Shown when |
| --- | --- | --- |
| `themeCreated` | `{path}` | A missing theme file was created. |
| `themeRepaired` | `{backup}` | The theme was repaired automatically. |
| `themeInspectFailed` | `{path}` `{error}` | The theme path could not be inspected. |
| `themeOpenFailed` | `{path}` | The theme file could not be opened. |
| `themeParseFailed` | `{path}` `{error}` | The theme JSON could not be parsed. |
| `themeUnsupportedFormat` | — | The theme is not a JSON object. |
| `themePathEmpty` | — | The resolved theme path is empty. |
| `themeFolderCreateFailed` | `{path}` `{error}` | The theme folder could not be created. |
| `themeBackupFailed` | `{backup}` `{error}` | The pre-repair theme backup could not be written. |
| `themeInvalidBackupFailed` | `{backup}` `{error}` | An invalid theme could not be backed up, so it was not overwritten. |
| `themeSerializeFailed` | `{error}` | The theme JSON could not be serialized. |
| `themeWriteFailed` | `{path}` | The temporary theme file could not be written. |
| `themeFinishWriteFailed` | `{path}` | Writing the temporary theme file failed. |
| `themeReplaceFailed` | `{path}` `{error}` | The temporary theme could not replace the real one. |

Claude and launch:

| Key | Placeholders | Shown when |
| --- | --- | --- |
| `claudeExited` | — | Claude exited with code 0. |
| `claudeExitedWithCode` | `{code}` | Claude exited with a non-zero code. |
| `claudeCouldNotStart` | — | Launching failed without a specific error. |
| `claudeProjectMissing` | `{path}` | The project folder does not exist. |
| `claudeNotRunnable` | `{path}` | The configured `claudePath` is a file but not runnable. |
| `claudePathNotFound` | `{path}` | A single configured `claudePath` entry was not found or was excluded by `claudeExclude`. |
| `claudeNotFound` | — | None of the `claudePath` entries or `PATH` contained a non-excluded Claude. |
| `claudeAmbiguous` | `{path}` `{candidates}` | A configured directory has several executables and none matches `claude*`. |
| `pathPatternNoMatch` | `{value}` | A wildcard path matched nothing. |
| `pathPatternAmbiguous` | `{value}` `{candidates}` | A project path wildcard matched more than one folder. |
| `environmentVariableFailed` | `{name}` `{error}` | A portable environment variable for a custom project action could not be set. |
| `createFolderFailed` | `{path}` `{error}` | The config or temp folder could not be created. |
| `notAFolder` | `{path}` | A portable folder path is not a directory. |
| `claudeStartFailed` | `{error}` | The Claude process could not be started. |
| `claudeWaitFailed` | `{error}` | Waiting for Claude failed. |
| `claudeExitCodeUnreadable` | `{error}` | The Claude exit code could not be read. |

The status line still uses green for normal statuses and red for errors; the
message text itself is not colored.
