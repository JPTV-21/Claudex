# Usage

[Back to README](../README.md) | [简体中文](zh-CN/usage.md)

## Main menu

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

- `Enter` launches Claude in the highlighted project. Only the focused section
  shows the `>` cursor and the inverse highlight; the other sections keep their
  selection but draw it with the inactive prefix.
- When the launcher starts from a folder other than its own (for example
  `cd ~/workspace && ~/Claudex`), the main screen shows a `Current folder`
  section above the project list. It is hidden when you start the launcher from
  its own folder. `Enter` launches Claude in that folder once without adding it
  to the project list; if the folder is already listed, it launches the listed
  project and updates its last-used time. The section's position, title, and
  row format come from the theme's [`cwdList`](themes.md#custom-screens-full-layout-control)
  block.
- `+ Add project` is always the last row and can be reached with the arrow keys.
- The add/edit screen has two fields: the optional project name and the folder
  path. Arrow keys move to the input the theme actually draws in that
  direction; `Up`/`Down` first walk the wrapped lines of a multi-line value.
  `Tab`/`Shift+Tab` cycle through the inputs in layout order. Enter saves and
  Esc cancels. `Shift+Enter` inserts a line break in any input field without
  submitting. The name is markup, so a project can carry its own color.
  `E` → `Edit project` opens both fields; the old separate `Rename` action was
  removed because it edited the same screen.
- The input placeholders come from the theme's `pathPlaceholder` (default
  `{drive}/Projects/my-project`, so it follows the launcher's drive) and
  `projectNamePlaceholder`.
- The `Recent` section lists the most recently launched projects, newest first.
  Its size and position come from the theme; see
  [`recentList`](themes.md#custom-screens-full-layout-control).
- `Quick launch` is its own section: a heading followed by the input row. It
  starts Claude in a typed folder once, without adding it to the project list.
  The folder still has to exist.

Other keys:

| Key | Action |
| --- | --- |
| `Shift+Enter` | Insert a line break in the focused input instead of submitting (`Alt+Enter` works where the terminal does not report Shift+Enter) |
| `Tab` / `Shift+Tab` | Cycle focus between sections in the order they appear in the layout |
| `Up` / `Down` | Move inside the focused section; at its first/last row, move to the adjacent section |
| `Left` / `Right` | Move within a grid row; on a single-column list they change sections. In the quick-launch and path-screen inputs they move the text cursor, and only at its edges do they move to the input/section drawn in that direction |
| `Enter` (quick launch) | Launch Claude once in the typed folder |
| `Enter` (current folder) | Launch Claude once in the process working directory |
| `Esc` (quick launch) | Leave the input without quitting |
| `E` or `F2` | Open the selected project's action menu |
| `Delete` | Remove the selected project (asks first) |
| `A` | Open the add-project screen |
| `Esc` or `Q` | Quit |
| `Y` / `N` | Confirm/cancel a create or remove prompt |
| `Enter` (confirm dialogs) | Same as `Y`: create the folder or remove the project |

These are defaults; rebind them with [`keys`](themes.md#key-bindings) in the theme file.
`A` also opens the add-project screen, which is useful when the add row is
hidden.

Mouse (every screen):

| Input | Action |
| --- | --- |
| Hover a row or button | Apply the hover highlight |
| Hover a main-screen row | Move the focus and the active cursor to that row |
| Left click a row | Activate the row, like `Enter` (the `Current folder` row starts Claude in that folder) |
| Left click a button | Run that button's action (`Continue`, `Cancel`, `Create`, `Back`, `Remove`) |
| Right click a project row | Open that project's action menu |
| Wheel | Focus the section under the pointer and move its selection |

The path screen shows clickable `Continue` / `Cancel` buttons, the create and
remove dialogs show clickable `Create` / `Back` and `Remove` / `Cancel`
buttons, and the project action menu (`E`) supports hover and click. The
keyboard shortcuts (`Enter`, `Esc`, `Y`, `N`) keep working everywhere.

Sections are traversed top to bottom in the order their blocks appear in
`screens.main.blocks`, so moving the `cwdList`, `projectList`, `quickInput`,
and `recentList` blocks also changes the Up/Down and Tab order. Set
`tabOnlySectionSwitch` to `true` in the config to stop Up/Down from leaving a
list at its first/last row; `Tab` and `Shift+Tab` still switch sections. With
the default `false`, `Up`/`Down` in the single-line quick-launch field switch
sections directly, and at the edges of the project or recent list they move to
the adjacent section.

The startup focus follows the order of
[`defaultFocus`](configuration.md#config-fields) entries: the first main-screen
section that the theme draws and that has something to focus wins; hidden or
empty sections are skipped.

The project action menu defaults to `Launch Claude`, `Edit path`, `Move up`,
`Move down`, `Move to top`, `Move to bottom`, `Remove from list`, and `Back`;
replace or extend it with
[`projectMenu.actions`](themes.md#custom-project-actions). The move actions reorder the
selected project in the list; they have no dedicated shortcuts and are chosen
from this menu. The saved list order is the order shown on the main screen and
written to the config file.

The launcher clears the terminal when it starts, immediately before launching
Claude, after Claude exits, and when the launcher exits. This uses the Windows
console API on Windows and ANSI clear sequences on Linux/macOS, so Claude and
Claudex do not share half of the screen.

The vendored FTXUI copy has its DECRQSS cursor-shape query disabled because
unsupported terminals print the query payload (`$q q`) onto the screen. It
also forwards Windows Shift+Enter as a CSI-u sequence (`\x1b[13;2u`), which
FTXUI has no built-in event for; the launcher uses it to insert a line break.

## Command line

```text
ClaudeX [--config PATH] [--where-claude]
```

`CLAUDEX_CONFIG` can also be set to an alternate config file. The selected
config is `--config`, else `CLAUDEX_CONFIG`, else the default
`./claudex-config.json`. The theme defaults to `./claudex-theme.json` beside
the executable. Both the `--config` value and `CLAUDEX_CONFIG` support
placeholders, environment variables, and wildcards; a relative config path is
resolved from the current working directory. Whatever file is selected, a
non-empty `configFile` field in it selects the file actually loaded, exactly
like `theme` selects the theme file.
`--where-claude` prints the resolved Claude executable and exits without
starting the UI, which is useful for checking `claudePath` and
`claudeExclude`; it honors exclusion patterns. `--help` prints the options and
default key bindings.
