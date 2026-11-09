# Configuration

[Back to README](../README.md) | [简体中文](zh-CN/configuration.md)

## Portable configuration

The launcher config defaults to `./claudex-config.json` and the theme to
`./claudex-theme.json`, both next to the executable. Portable Claude data
defaults to `./data/`:

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

The theme file is created automatically from the embedded default theme when
it is missing. Its location comes from the config's `theme` field; relative
theme paths start at the launcher folder. The `theme` field, and every other
path field in the config, supports placeholders, environment variables, `~`,
and `*`/`?` wildcards (see
[Path templates and variables](#path-templates-and-variables)). By default the
launcher looks for `claude*` in its own folder and then on `PATH`; the `bin/`
layout is opt-in through `claudePath`.

Relative project paths stay relative and are resolved from the launcher
folder; they are not converted to absolute paths just because they point
outside the launcher folder. Relative paths that resolve inside the launcher
folder are shortened to a clean relative form (no `..`). Absolute paths inside
the launcher folder are also stored relative so the folder stays movable;
other absolute paths stay absolute. The path is only resolved to decide
whether to ask about creating a missing folder.

A project path entered with a path template (`{root}`, `{drive}`, `{home}`,
environment variables, or a leading `~`) is stored in its template form
(Windows backslashes become forward slashes), because the template is expanded
again every time the project is resolved. This keeps one config usable across
drives or machines: for example, adding `{drive}/Projects/my-project` stores
that template, and the launcher resolves it to whichever drive it runs from.
Input containing `*` or `?` wildcards is still resolved to the single matching
folder when the project is added, so the stored path is concrete.

The launcher writes `claudex-config.json` and `claudex-theme.json` on first
run. They can be edited by hand while the launcher is closed. A corrupt config
is backed up to `claudex-config.json.invalid`; a corrupt theme is backed up to
`claudex-theme.json.invalid` and replaced with the default theme. Configs from
older versions that still contain an inline `menu` object are migrated
automatically: the menu is merged into the theme file, then removed from the
config.

The config file location can be changed with `--config PATH` or
`CLAUDEX_CONFIG`; both also accept placeholders and environment variables. All
launcher settings, including the data locations (`claudeConfigDir` and
`tempDir`), are normal config fields.

The config file can also be selected from the config itself, the same way
`theme` selects the theme file: copy `claudex-config.json` to the new location
yourself, then set `configFile` in the config you launch to that path. The
launcher only follows the path; nothing is copied, moved, or deleted
automatically, and the original file is left as it is (it can keep serving as
the small bootstrap that points at the new file). Placeholders and environment
variables are expanded, and a relative `configFile` starts at the launcher
folder. If the target does not exist, the launcher creates a new default
config there. Only the bootstrap's `configFile` field is read; its other
fields are ignored. `configFile` switches only the config file: the theme,
`data/`, `tmp/`, and the log still resolve from the launcher folder.

Existing `data/launcher.json` files and the old project-array format are not
read or migrated automatically. Set the config path explicitly if you want to
keep an existing file at another location.

### Defaults and startup repair

Default values are not hardcoded in C++. They live in
`resources/default-config.json` and `resources/default-theme.json`, which CMake
embeds into the executable at configure time. The launcher trusts these
resources to be valid; there is no second set of fallback defaults.

On every startup, before drawing the menu, the launcher:

- creates `claudex-theme.json` if the resolved theme file is missing;
- merges missing theme keys and wrong JSON types from the default theme, and
  writes a `.before-fix` backup of the previous theme;
- backs up unparseable theme JSON to `.invalid` and replaces it with the
  merged/default theme (or leaves it untouched if the backup fails);
- adds missing config keys using the default values, including a missing
  `claudeExclude`;
- accepts a string `launchCommand` as shorthand for both platforms, and fills
  missing or blank `windows`/`posix` entries from the default object;
- replaces empty `theme`, `claudePath`, `launchCommand`, `claudeConfigDir`, and
  `tempDir` values with the default values, so the saved config is explicit;
  an empty `configFile` is kept because it means "keep the current file";
  an empty `logFile` is kept because it disables logging, and an empty
  `claudeExclude: []` is kept because it means "exclude nothing";
- replaces config keys whose JSON type is wrong with the default value;
- keeps every valid user value and other extra user key;
- writes a backup to `claudex-config.json.before-fix` before repairing the
  config.

Configs from an incompatible layout are not converted. Unparseable config or
theme JSON, and the project-array config format, cannot be compared key-by-key,
so they are reported and left untouched; replace them with a current config.

### Logging

The launcher writes `claudex.log` beside the executable by default. The log
records startup, the resolved config/theme paths, config saves and repairs,
theme creation and repairs, the resolved Claude executable, arguments, working
directory, portable config/temp folders, launch failures, and Claude's exit
code. Claude's interactive terminal output is not redirected,
because that would break the CLI UI.

Set `logFile` in the config to relocate the log, or set it to an empty string to
disable logging.

### Config fields

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
  "defaultFocus": [
    { "section": "cwd", "match": "", "item": 0 },
    { "section": "projects", "match": "", "item": 0 },
    { "section": "recent", "match": "", "item": 0 },
    { "section": "quickInput", "match": "", "item": 0 }
  ],
  "projects": [
    {
      "path": "../my-project",
      "lastUsed": "2026-10-26T12:00:00",
      "name": "<accent>My project</accent>"
    }
  ]
}
```

`name` is optional. It is markup, resolved like every other theme string, so a
project can carry its own color from the theme's variables. The same path may
be listed more than once.

| Field | Meaning |
| --- | --- |
| `theme` | Theme JSON path. Missing or empty means `claudex-theme.json`. Supports placeholders and wildcards. |
| `configFile` | Optional path of the config file to load instead of this one. Copy the file there yourself; the launcher only follows the path, like `theme`. Missing or empty keeps the current file. Relative paths start at the launcher folder. |
| `claudePath` | Where to find Claude. A string or an array of strings. Each entry may be a file, a directory, a wildcard path, or a bare name/pattern such as `claude*`. A directory entry means `<directory>/claude*`. Empty or missing uses the default `["claude*"]`: the launcher folder, then `PATH`. See [Claude executable detection](#claude-executable-detection). |
| `claudeExclude` | User-written exclusions for Claude executable detection. A string or an array of strings. A pattern without a path separator (`/`, or `\` on Windows) that does not expand to an absolute path matches the candidate file name (or stem); otherwise it matches the expanded full path. Supports `*` and `?`; Windows is case-insensitive. Defaults to `["ClaudeX*"]`; an empty array excludes nothing. |
| `launchCommand` | Shell command used to start Claude. A string applies to every platform; an object may provide `windows` and `posix` variants. Defaults to an object that sets `CLAUDE_CONFIG_DIR`, `TEMP`, `TMP`, and `TMPDIR` from the placeholders and runs `{claude}`. Placeholders: `{claude}`, `{claudeConfigDir}`, `{tempDir}`, and the path tokens below. Commands without `{claude}` run as-is and skip detection. Extra arguments go directly in the command, for example `"{claude} --model sonnet"`. |
| `claudeConfigDir` | Portable Claude config/data folder. Defaults to `data`. Relative paths start at the launcher folder. |
| `tempDir` | Portable temporary folder used by the default `launchCommand` for `TEMP`, `TMP`, and `TMPDIR`. Defaults to `data/tmp`. |
| `logFile` | Launcher diagnostics log. Empty disables logging. |
| `tabOnlySectionSwitch` | When `false` (default), Up/Down also move between main-screen sections at a list's first/last row. When `true`, only Tab/Shift+Tab switch sections. The order follows the `screens.main.blocks` layout. |
| `defaultProjectName` | Prefilled into the optional project-name field when a project is added. Empty (the default) leaves the field blank. Only affects new projects, not edits; the value is trimmed like a typed name and may contain markup. |
| `defaultFocus` | Focus targets, in priority order. `section` accepts `cwd` (also `cwdList`, `currentFolder`, `workingDirectory`, `here`), `projects` (also `projectList`, `project`), `recent` (also `recentList`, `recentProjects`), `quickInput` (also `quick`, `input`), `actions` (also `actionList`, `projectMenu`), `name` (also `nameInput`), and `path` (also `pathInput`). For the main screen, entries are tried in array order; the first section the theme draws and that currently has something to focus wins. A hidden cwd section, an empty project list, and an empty recent list are skipped. If none of the main-screen entries is available, the first available section in `screens.main.blocks` order is used. For list sections, `match` is a stable identifier defined by the section: a project's stored path or display name for `projects`/`recent` (or `add` for the "+ Add project" row), or an action id/label for `actions`. It wins over `item` and keeps pointing at the same item when the list is reordered or refreshed. When `match` is empty or does not match, `item` is a zero-based row; a negative value means the last row, and a value past the end clamps. Sections on another page (`actions`, `name`, `path`) apply the target when that page opens and are skipped by the main-screen priority scan. `cwd` and `quickInput` ignore `match` and `item`; the path fields ignore both. Defaults to the four-element array shown above. |
| `projects[].path` | Folder to start Claude in. Relative paths start at the launcher folder and stay relative when saved. |
| `projects[].lastUsed` | Timestamp updated when a project is launched. Drives the recent list; a newly added project stays empty until it is launched. |

`defaultFocus` accepts one object or an array of objects. With an array, the
main-screen entries (`cwd`, `projects`, `recent`, `quickInput`) are a priority
list: the first available one sets the startup focus. Reorder the array to
change that priority; remove `cwd` if the launcher should always start on the
project list. The first `actions` entry is used when the project action menu
opens, and the first `name`/`path` entry is used when the add/edit screen
opens. Entries for other pages are ignored by the main-screen scan.

```json
"defaultFocus": [
  { "section": "recent", "match": "F:/work/app" },
  { "section": "actions", "match": "edit" },
  { "section": "path", "match": "name" }
]
```

The cwd section is available only when the launcher starts from a folder other
than its own; it is also skipped when the theme has no `cwdList` block.

## Path templates and variables

`theme`, `claudePath`, `claudeExclude` (path patterns), `claudeConfigDir`,
`tempDir`, `logFile`, the theme's `pathPlaceholder`, project paths, and the
`--config` / `CLAUDEX_CONFIG` paths support:

| Token | Value |
| --- | --- |
| `{root}` | Launcher folder |
| `{platform}` | `windows`, `macos`, or `linux` |
| `{arch}` | `x86_64`, `arm64`, or `x86` |
| `{home}` | Current user home folder |
| `{drive}` | Windows: launcher drive (`F:`). macOS/Linux: the mount point of the filesystem containing the launcher, for example `/Volumes/Disk` or `/home`; `/` when it is on the root filesystem. |

`launchCommand` uses the same path tokens, plus `{claude}`, `{claudeConfigDir}`,
and `{tempDir}`. Unlike path fields, it leaves environment variables and `~`
to the shell.

Environment variables are expanded too: `%USERPROFILE%` and `$HOME` both work.
A leading `~` is expanded to the home folder; `{home}` works anywhere in the
string and is the more explicit form.

Every path value also accepts `*` and `?` wildcards, matched per path segment.
For example, `bin/*/claude*` or `D:/tools/claude-*/bin`. A wildcard is expanded
before the path is used; when a single value is expected and several paths
match, the first path in sorted order is used. Project paths must match exactly
one folder when they are added or edited. `claudeExclude` full-path patterns are
the exception: they match the whole path string, so `*` and `?` may cross
separators.

## Claude executable detection

`claudePath` defaults to `["claude*"]`. For each entry, in order:

1. A file path: use it directly (with the platform's executable extensions).
2. A directory: shorthand for `<directory>/claude*`. Platform extensions come
   from `PATHEXT` on Windows, so `.exe`, `.cmd`, and `.bat` wrappers are all
   found. On macOS/Linux a file must be executable.
3. A wildcard path such as `./aaa/claude*`: matching runnable files are tried
   first, then matching directories are searched for `claude*`.
4. A bare name or pattern such as `claude` or `claude*`: search the launcher
   folder first, then `PATH`. If a file or folder with that name exists next
   to the launcher, it is treated as that path instead.

Nothing is excluded automatically. The default config avoids matching the
launcher itself with `"claudeExclude": ["ClaudeX*"]`; if you set
`claudeExclude` to `[]`, a launcher named `ClaudeX.exe` in the search path can
be selected as Claude.

`claudeExclude` patterns:

- without a path separator (`/`, or `\` on Windows), and not expanding to an
  absolute path, match the file name (or stem) in any searched folder, for
  example `ClaudeX*` or `uninstaller.exe`;
- with a path separator, or expanding to an absolute path, match the expanded
  full path, for example `./aaa/uninstaller*`, `{root}/tools/claude-old*`, or
  `{root}`;
- support `*` and `?`; in full-path patterns both may cross path separators;
  matching is case-insensitive on Windows and case-sensitive on macOS/Linux.

Excluded files and directories are skipped entirely, including the
single-candidate fallback and ambiguity checks. Name patterns are also tested
against a matched directory's name, so `ClaudeX*` can skip a directory named
`ClaudeX-tools`.

Inside a directory the launcher matches `claude*` against the file name and its
stem (so `claude` matches `claude.exe` and `claude.cmd`). If several candidates
match, the platform's `PATHEXT` order decides; otherwise the first name in
sorted order is used and a warning is written to the log. If nothing matches
and the directory was explicitly configured, a directory containing exactly
one executable is accepted. Otherwise the launcher reports an ambiguity
instead of guessing. A directory with several unrelated executables and no
matching name is an error, not a guess.

Set `claudePath` in the config if Claude lives somewhere else. The `bin/<arch>`
layout is no longer searched automatically; use
`"claudePath": "bin/{arch}/claude*"` or add it to the array.

## Launch command

`launchCommand` is the single source of truth for how Claude starts. It can be
a string that applies to every platform:

```json
"launchCommand": "{claude} --model sonnet"
```

or an object with separate Windows and POSIX commands:

```json
"launchCommand": {
  "windows": "set \"CLAUDE_CONFIG_DIR={claudeConfigDir}\" && set \"TEMP={tempDir}\" && set \"TMP={tempDir}\" && set \"TMPDIR={tempDir}\" && {claude}",
  "posix": "CLAUDE_CONFIG_DIR='{claudeConfigDir}' TEMP='{tempDir}' TMP='{tempDir}' TMPDIR='{tempDir}' {claude}"
}
```

The default object sets the portable environment and then runs the detected
Claude. Supported placeholders:

- `{claude}`: the executable resolved through `claudePath`, already quoted for
  the platform shell;
- `{claudeConfigDir}` and `{tempDir}`: resolved raw paths; quote them in the
  command;
- `{root}`, `{platform}`, `{arch}`, `{home}`, and `{drive}`: the same values as
  the path-template tokens.

The launcher creates `claudeConfigDir` and `tempDir`, expands the placeholders,
and runs the command through `cmd.exe /d /s /c` on Windows or `/bin/sh -c` on
Linux/macOS. It does **not** set `CLAUDE_CONFIG_DIR`, `TEMP`, `TMP`, or
`TMPDIR` itself for Claude launches; the default command does that explicitly.
If you replace the command and omit those assignments, Claude uses the ambient
environment. Environment variables and a leading `~` in the command are left
to the shell.

Relative path words in `launchCommand` resolve against the launcher folder:

- `./bin/claude-test` and `../tools/run.cmd` are rewritten to absolute paths;
- an unquoted first command word such as `bin/claude-test`, or a file that
  exists next to the launcher, is treated the same way;
- a word produced by `{root}`, `{claudeConfigDir}`, or `{tempDir}` is already
  absolute and is quoted if it is the first command word.

Other words are left to the shell, so flags, URLs, environment variables, and
patterns keep their meaning. Forms such as `--config=./x` and
`set "VAR=./x"` are not rewritten; write the path as a separate word or use a
placeholder.

To use a wrapper or bypass detection entirely, write a command without
`{claude}`:

```json
"launchCommand": "wsl claude --model sonnet"
```

With the default command, the Claude process receives:

- `CLAUDE_CONFIG_DIR` = `claudeConfigDir`
- `TEMP`, `TMP`, `TMPDIR` = `tempDir`
- its working directory = the selected project folder

Custom project actions configured in the theme still receive the portable
environment from the launcher.
