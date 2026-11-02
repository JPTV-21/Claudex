@echo off
rem Standalone console downloader for the native Claude Code binaries published
rem in the official anthropics/claude-code GitHub releases.
rem
rem Windows console downloader. It auto-detects the host architecture, lets you
rem pick a release (latest by default) and one or more targets, verifies
rem SHA-256, and asks before overwriting a binary.
rem
rem Usage: tools\download-claude.bat [options]
rem Run with --help for the full option list.
rem
rem Delayed expansion is deliberately NOT enabled: the project path may contain
rem "!" and delayed expansion would strip it.

setlocal EnableExtensions DisableDelayedExpansion
title Claude Code downloader

set "REPO=anthropics/claude-code"
set "UA=claudex-downloader/1.0"
set "TARGET_COUNT=8"
set "WORK="
set "DLTMP="
set "SELECTED_INDICES="
set "SELECTED_COUNT=0"
set "CONFLICTS=0"
set "KEEP_CONFLICTS=0"
set "OVERWRITE=0"
set "HOST_INDEX="
set "HOST_MARK=0"
set "BAD_TARGET="
set "BAD_TOKEN="

if not defined CLAUDEX_API_BASE set "CLAUDEX_API_BASE=https://api.github.com/repos/anthropics/claude-code"
if not defined CLAUDEX_DOWNLOAD_BASE set "CLAUDEX_DOWNLOAD_BASE=https://github.com/anthropics/claude-code/releases/download"

for %%I in ("%~dp0..") do set "ROOT=%%~fI"
set "DEST=%ROOT%"

set "HOST_ARCH="
if /I "%PROCESSOR_ARCHITECTURE%"=="AMD64" set "HOST_ARCH=x64"
if /I "%PROCESSOR_ARCHITECTURE%"=="ARM64" set "HOST_ARCH=arm64"
if /I "%PROCESSOR_ARCHITEW6432%"=="AMD64" set "HOST_ARCH=x64"
if /I "%PROCESSOR_ARCHITEW6432%"=="ARM64" set "HOST_ARCH=arm64"
if /I "%HOST_ARCH%"=="x64" set "HOST_INDEX=1"
if /I "%HOST_ARCH%"=="arm64" set "HOST_INDEX=2"
if defined HOST_INDEX set "HOST_MARK=%HOST_INDEX%"

rem ---------------------------------------------------------------------------
rem Command line parsing.
rem ---------------------------------------------------------------------------

:parse_args
if "%~1"=="" goto args_done
set "ARG=%~1"

if /I "%ARG%"=="-h" goto show_help
if /I "%ARG%"=="--help" goto show_help
if /I "%ARG%"=="-u" goto opt_update
if /I "%ARG%"=="--update" goto opt_update
if /I "%ARG%"=="-y" goto opt_yes
if /I "%ARG%"=="--yes" goto opt_yes
if /I "%ARG%"=="-v" goto opt_version
if /I "%ARG%"=="--version" goto opt_version
if /I "%ARG%"=="--list" goto opt_list
if /I "%ARG%"=="--target" goto opt_target
if /I "%ARG%"=="--platform" goto opt_platform
if /I "%ARG%"=="--arch" goto opt_arch
if /I "%ARG%"=="--dest" goto opt_dest
if /I "%ARG%"=="--skip-existing" goto opt_skip
if /I "%ARG%"=="--dry-run" goto opt_dry
if /I "%ARG%"=="--no-verify" goto opt_noverify
if "%ARG%"=="--" goto opt_end

if /I "%ARG:~0,10%"=="--version=" goto opt_version_eq
if /I "%ARG:~0,9%"=="--target=" goto opt_target_eq
if /I "%ARG:~0,11%"=="--platform=" goto opt_platform_eq
if /I "%ARG:~0,7%"=="--arch=" goto opt_arch_eq
if /I "%ARG:~0,7%"=="--dest=" goto opt_dest_eq

echo error: unknown option: %ARG% 1>&2
goto usage_error

:opt_update
set "UPDATE=1"
shift
goto parse_args

:opt_yes
set "YES=1"
shift
goto parse_args

:opt_list
set "LIST=1"
shift
goto parse_args

:opt_skip
set "SKIP_EXISTING=1"
shift
goto parse_args

:opt_dry
set "DRY_RUN=1"
shift
goto parse_args

:opt_noverify
set "NO_VERIFY=1"
shift
goto parse_args

:opt_end
shift
goto args_done

:opt_version
if "%~2"=="" goto missing_version
set "VERSION=%~2"
shift
shift
goto parse_args

:opt_target
if "%~2"=="" goto missing_target
set "TARGET_SPECS=%TARGET_SPECS%,%~2"
shift
shift
goto parse_args

:opt_platform
if "%~2"=="" goto missing_platform
set "PLATFORM_FILTER=%PLATFORM_FILTER%,%~2"
shift
shift
goto parse_args

:opt_arch
if "%~2"=="" goto missing_arch
set "ARCH_FILTER=%ARCH_FILTER%,%~2"
shift
shift
goto parse_args

:opt_dest
if "%~2"=="" goto missing_dest
set "DEST=%~2"
shift
shift
goto parse_args

:opt_version_eq
set "VERSION=%ARG:~10%"
if not defined VERSION goto missing_version
shift
goto parse_args

:opt_target_eq
set "TARGET_SPECS=%TARGET_SPECS%,%ARG:~9%"
shift
goto parse_args

:opt_platform_eq
set "PLATFORM_FILTER=%PLATFORM_FILTER%,%ARG:~11%"
shift
goto parse_args

:opt_arch_eq
set "ARCH_FILTER=%ARCH_FILTER%,%ARG:~7%"
shift
goto parse_args

:opt_dest_eq
set "DEST=%ARG:~7%"
if not defined DEST goto missing_dest
shift
goto parse_args

:missing_version
echo error: --version requires a value 1>&2
goto usage_error
:missing_target
echo error: --target requires a value 1>&2
goto usage_error
:missing_platform
echo error: --platform requires a value 1>&2
goto usage_error
:missing_arch
echo error: --arch requires a value 1>&2
goto usage_error
:missing_dest
echo error: --dest requires a value 1>&2
goto usage_error

:args_done
if defined UPDATE (
  set "YES=1"
  if not defined VERSION set "VERSION=latest"
)
if defined YES if defined SKIP_EXISTING goto usage_flags
if defined TARGET_SPECS if defined PLATFORM_FILTER goto usage_targets
if defined TARGET_SPECS if defined ARCH_FILTER goto usage_targets
if not defined DEST set "DEST=%ROOT%"
for %%I in ("%DEST%") do set "DEST=%%~fI"
set "BIN=%DEST%\bin"
if defined VERSION call :normalize_version

rem ---------------------------------------------------------------------------
rem Validate the selection flags before touching the network.
rem ---------------------------------------------------------------------------

if defined TARGET_SPECS (
  call :validate_regex "%TARGET_SPECS%" "^[A-Za-z0-9,._-]+$"
  if errorlevel 1 goto usage_bad_target
)
if defined PLATFORM_FILTER (
  call :validate_regex "%PLATFORM_FILTER%" "^[A-Za-z0-9,._-]+$"
  if errorlevel 1 goto usage_bad_platform
)
if defined ARCH_FILTER (
  call :validate_regex "%ARCH_FILTER%" "^[A-Za-z0-9,._-]+$"
  if errorlevel 1 goto usage_bad_arch
)
if defined VERSION (
  call :validate_regex "%VERSION%" "^[A-Za-z0-9._-]+$"
  if errorlevel 1 goto usage_bad_version
)

where curl >nul 2>&1
if errorlevel 1 goto fatal_curl
where powershell >nul 2>&1
if errorlevel 1 goto fatal_ps

if not defined TEMP set "TEMP=%TMP%"
if not defined TEMP set "TEMP=%CD%"
set "WORK=%TEMP%\claudex-%RANDOM%%RANDOM%"
mkdir "%WORK%" >nul 2>&1
if errorlevel 1 goto fatal_work

echo Claude Code downloader
echo   Repository: %REPO%
if defined HOST_ARCH (
  echo   Host:       Windows %HOST_ARCH%
) else (
  echo   Host:       unsupported; choose a target manually
)

rem ---------------------------------------------------------------------------
rem Fetch releases only when a list or the latest version is needed.
rem ---------------------------------------------------------------------------

set "NEED_LIST=0"
if defined LIST set "NEED_LIST=1"
if not defined VERSION set "NEED_LIST=1"
if defined VERSION if /I "%VERSION%"=="latest" set "NEED_LIST=1"

if "%NEED_LIST%"=="0" goto version_specific
call :fetch_releases
if errorlevel 1 goto fatal_releases
call :compute_latest
if errorlevel 1 goto fatal_releases
call :count_releases

if defined LIST goto show_list

:version_specific
if defined VERSION goto have_version

call :display_releases
set "PICKED_TAG="
:version_prompt
set "ANSWER="
set /p "ANSWER=Select a version [1-%RELEASE_COUNT%] or tag (Enter = %LATEST_TAG%): "
if not defined ANSWER goto version_latest
if /I "%ANSWER%"=="latest" goto version_latest
set "NONDIGIT="
for /f "delims=0123456789" %%A in ("%ANSWER%") do set "NONDIGIT=1"
if defined NONDIGIT goto version_tag_input
call :pick_release "%ANSWER%"
if defined PICKED_TAG goto have_version
echo Invalid selection. 1>&2
goto version_prompt
:version_tag_input
call :validate_regex "%ANSWER%" "^[A-Za-z0-9._-]+$"
if errorlevel 1 (
  echo Invalid tag. 1>&2
  goto version_prompt
)
set "PICKED_TAG=%ANSWER%"
call :normalize_picked
goto have_version
:version_latest
set "PICKED_TAG=%LATEST_TAG%"
goto have_version

:have_version
if defined PICKED_TAG (
  set "TAG_SELECTED=%PICKED_TAG%"
) else (
  if /I "%VERSION%"=="latest" (
    set "TAG_SELECTED=%LATEST_TAG%"
  ) else (
    set "TAG_SELECTED=%VERSION%"
  )
)
if not defined TAG_SELECTED goto fatal_releases
echo   Release:    %TAG_SELECTED%

if defined LIST goto show_list

rem ---------------------------------------------------------------------------
rem Target selection.
rem ---------------------------------------------------------------------------

if defined TARGET_SPECS goto targets_from_specs
if defined PLATFORM_FILTER goto targets_from_filters
if defined ARCH_FILTER goto targets_from_filters
if defined UPDATE goto targets_from_host
goto targets_prompt

:targets_from_specs
for %%S in (%TARGET_SPECS%) do call :add_target_spec "%%S"
if defined BAD_TARGET goto fatal_targets
if "%SELECTED_COUNT%"=="0" goto fatal_targets
goto selection_ready

:targets_from_filters
call :try_variant windows x64
call :try_variant windows arm64
call :try_variant macos x64
call :try_variant macos arm64
call :try_variant linux x64
call :try_variant linux arm64
if "%SELECTED_COUNT%"=="0" goto fatal_targets
goto selection_ready

:targets_from_host
if not defined HOST_INDEX goto fatal_host
call :add_selected %HOST_INDEX%
goto selection_ready

:targets_prompt
call :display_targets
set "DEFAULT_TARGET=%HOST_MARK%"
if "%DEFAULT_TARGET%"=="0" set "DEFAULT_TARGET=1"
:target_prompt
set "ANSWER="
set /p "ANSWER=Select targets (comma separated, Enter = %DEFAULT_TARGET%, all = all): "
if not defined ANSWER set "ANSWER=%DEFAULT_TARGET%"
call :validate_regex "%ANSWER%" "^(all|[0-9, -]+)$"
if errorlevel 1 goto target_invalid
if /I "%ANSWER%"=="all" (
  call :select_all
  goto selection_ready
)
set "SELECTED_INDICES="
set "SELECTED_COUNT=0"
set "BAD_TOKEN="
for %%T in (%ANSWER%) do call :parse_target_token "%%T"
if defined BAD_TOKEN goto target_invalid
if "%SELECTED_COUNT%"=="0" goto target_invalid
goto selection_ready
:target_invalid
echo Enter numbers 1-%TARGET_COUNT%, ranges like 1-3, or all. 1>&2
goto target_prompt

:selection_ready
if "%SELECTED_COUNT%"=="0" goto fatal_targets

rem ---------------------------------------------------------------------------
rem Plan and overwrite handling.
rem ---------------------------------------------------------------------------

call :count_conflicts
set "KEEP_CONFLICTS=0"
set "OVERWRITE=0"
if "%CONFLICTS%"=="0" goto plan_ready
if defined SKIP_EXISTING (
  set "KEEP_CONFLICTS=1"
  echo Keeping existing files.
  goto plan_ready
)
if defined YES goto overwrite_yes
if defined DRY_RUN goto overwrite_yes
echo.
echo These files already exist:
for %%I in (%SELECTED_INDICES%) do call :show_conflict %%I
set "ANSWER="
set /p "ANSWER=Overwrite existing file(s)? [y/N]: "
if /I "%ANSWER%"=="y" goto overwrite_yes
if /I "%ANSWER%"=="yes" goto overwrite_yes
set "KEEP_CONFLICTS=1"
echo Keeping existing files.
goto plan_ready
:overwrite_yes
set "OVERWRITE=1"

:plan_ready
call :print_plan
if defined DRY_RUN goto done_dry
call :count_pending
if not defined PENDING set "PENDING=0"
if "%PENDING%"=="0" goto done_nothing

if not exist "%BIN%" mkdir "%BIN%" >nul 2>&1
if not exist "%BIN%" goto fatal_bin
set "DLTMP=%BIN%\.claudex-tmp-%RANDOM%%RANDOM%"
mkdir "%DLTMP%" >nul 2>&1
if errorlevel 1 goto fatal_dltmp

call :fetch_asset_urls
if errorlevel 1 echo warning: could not resolve API asset URLs; using direct download URLs 1>&2

if defined NO_VERIFY goto after_shasums
set "SHASUMS_URL=%CLAUDEX_DOWNLOAD_BASE%/%TAG_SELECTED%/SHASUMS256.txt"
call :lookup_asset "SHASUMS256.txt"
if defined ASSET_URL set "SHASUMS_URL=%ASSET_URL%"
curl -fsSL --retry 2 --retry-delay 1 --connect-timeout 30 -H "Accept: application/octet-stream" -o "%DLTMP%\SHASUMS256.txt" "%SHASUMS_URL%"
if errorlevel 1 (
  echo warning: could not download SHASUMS256.txt; verification disabled 1>&2
  set "NO_VERIFY=1"
)
:after_shasums

set "INDEX=0"
set "DOWNLOADED=0"
set "SKIPPED=0"
set "FAILED=0"
for %%I in (%SELECTED_INDICES%) do call :download_target %%I

echo.
if not "%FAILED%"=="0" (
  echo Finished with errors: %DOWNLOADED% downloaded, %SKIPPED% skipped, %FAILED% failed.
  call :cleanup
  exit /b 1
)
echo Done: %DOWNLOADED% downloaded, %SKIPPED% skipped, %FAILED% failed.
call :cleanup
exit /b 0

:show_list
call :count_releases
if errorlevel 1 goto fatal_releases
call :display_releases
goto done_ok

:done_dry
echo.
echo Dry run: no files written.
goto done_ok

:done_nothing
echo.
echo Nothing to do: existing files were kept.
goto done_ok

:done_ok
call :cleanup
exit /b 0

rem ---------------------------------------------------------------------------
rem Usage errors and fatal errors.
rem ---------------------------------------------------------------------------

:usage_flags
echo error: use either --yes or --skip-existing, not both 1>&2
goto usage_error
:usage_targets
echo error: use either --target or --platform/--arch, not both 1>&2
goto usage_error
:usage_bad_target
echo error: invalid --target value 1>&2
goto usage_error
:usage_bad_platform
echo error: invalid --platform value 1>&2
goto usage_error
:usage_bad_arch
echo error: invalid --arch value 1>&2
goto usage_error
:usage_bad_version
echo error: invalid --version value 1>&2
goto usage_error
:usage_error
echo Run with --help for usage. 1>&2
call :cleanup
exit /b 2

:fatal_curl
set "FATAL_MSG=curl.exe is required"
goto fatal
:fatal_ps
set "FATAL_MSG=PowerShell is required"
goto fatal
:fatal_work
set "FATAL_MSG=could not create a temporary folder"
goto fatal
:fatal_releases
set "FATAL_MSG=could not fetch GitHub releases"
goto fatal
:fatal_targets
set "FATAL_MSG=could not resolve the requested targets"
goto fatal
:fatal_host
set "FATAL_MSG=cannot detect the host target; use --target or --platform/--arch"
goto fatal
:fatal_bin
set "FATAL_MSG=could not create the bin folder"
goto fatal
:fatal_dltmp
set "FATAL_MSG=could not create a temporary folder in bin"
goto fatal
:fatal
echo error: %FATAL_MSG% 1>&2
call :cleanup
exit /b 1

rem ---------------------------------------------------------------------------
rem Help.
rem ---------------------------------------------------------------------------

:show_help
echo Usage: tools\download-claude.bat [options]
echo.
echo Download official Claude Code binaries from GitHub releases into the
echo launcher's bin folder. Set claudePath to that folder in
echo claudex-config.json to use it.
echo.
echo Version selection:
echo   -v, --version TAG        Release tag, or latest (default)
echo       --list               List recent releases and exit
echo.
echo Target selection (comma separated or repeated):
echo       --target NAME        windows-x64, windows-arm64, macos-x64,
echo                           macos-arm64, linux-x64, linux-arm64,
echo                           linux-x64-musl, linux-arm64-musl, all
echo       --platform NAME      windows, macos, linux
echo       --arch NAME          x64, arm64
echo   --platform/--arch select one variant per combination. Use --target
echo   for musl explicitly. Without a target flag the downloader asks
echo   interactively.
echo.
echo Output:
echo       --dest DIR           Launcher root (default: script's parent)
echo   Files are written to bin\ARCH\claude.exe for one detected host
echo   target, otherwise to bin\PLATFORM-ARCH\claude.exe. Point claudePath
echo   at the chosen folder to use it.
echo.
echo Other:
echo   -u, --update             Latest release, detected host, overwrite
echo   -y, --yes                Overwrite existing binaries without asking
echo       --skip-existing      Keep existing binaries and skip their targets
echo       --dry-run            Print the plan without downloading
echo       --no-verify          Skip SHA-256 verification
echo   -h, --help               Show this help
echo.
echo Environment:
echo   GITHUB_TOKEN / GH_TOKEN  Raise the GitHub API rate limit
echo   HTTPS_PROXY / HTTP_PROXY Used by curl automatically
echo.
echo Examples:
echo   tools\download-claude.bat
echo   tools\download-claude.bat --update
echo   tools\download-claude.bat --version v2.1.312 --target windows-x64
echo   tools\download-claude.bat --platform windows,macos --arch x64 --yes
echo   tools\download-claude.bat --list
call :cleanup
exit /b 0

rem ---------------------------------------------------------------------------
rem Subroutines.
rem ---------------------------------------------------------------------------

:validate_regex
set "CHECK_VALUE=%~1"
set "CHECK_REGEX=%~2"
powershell -NoProfile -ExecutionPolicy Bypass -Command "if ($env:CHECK_VALUE -match $env:CHECK_REGEX) { exit 0 } else { exit 1 }"
if errorlevel 1 exit /b 1
exit /b 0

:normalize_version
set "FIRST=%VERSION:~0,1%"
set "NONDIGIT="
for /f "delims=0123456789" %%A in ("%FIRST%") do set "NONDIGIT=1"
if not defined NONDIGIT set "VERSION=v%VERSION%"
exit /b 0

:normalize_picked
set "FIRST=%PICKED_TAG:~0,1%"
set "NONDIGIT="
for /f "delims=0123456789" %%A in ("%FIRST%") do set "NONDIGIT=1"
if not defined NONDIGIT set "PICKED_TAG=v%PICKED_TAG%"
exit /b 0

:fetch_releases
set "PS_API=%CLAUDEX_API_BASE%"
set "PS_OUT=%WORK%\releases.txt"
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
 "$ErrorActionPreference='Stop'; $ProgressPreference='SilentlyContinue';" ^
 "$h=@{ 'User-Agent'='claudex-downloader/1.0'; 'Accept'='application/vnd.github+json' };" ^
 "if ($env:GITHUB_TOKEN) { $h['Authorization']='Bearer ' + $env:GITHUB_TOKEN } elseif ($env:GH_TOKEN) { $h['Authorization']='Bearer ' + $env:GH_TOKEN };" ^
 "$r=Invoke-RestMethod -Headers $h -Uri ($env:PS_API + '/releases?per_page=20');" ^
 "$lines = $r | Where-Object { -not $_.draft } | ForEach-Object { $d=$_.published_at; if ($d -is [datetime]) { $d=$d.ToString('yyyy-MM-dd') } else { $d=[string]$d; if ($d.Length -ge 10) { $d=$d.Substring(0,10) } }; '{0}|{1}|{2}' -f $_.tag_name,$d,[int]$_.prerelease };" ^
 "if (-not $lines) { exit 1 };" ^
 "$lines | Set-Content -LiteralPath $env:PS_OUT -Encoding ASCII"
if errorlevel 1 exit /b 1
if not exist "%PS_OUT%" exit /b 1
exit /b 0

:fetch_asset_urls
set "PS_API=%CLAUDEX_API_BASE%"
set "PS_TAG=%TAG_SELECTED%"
set "PS_OUT=%WORK%\assets.txt"
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
 "$ErrorActionPreference='Stop'; $ProgressPreference='SilentlyContinue';" ^
 "$h=@{ 'User-Agent'='claudex-downloader/1.0'; 'Accept'='application/vnd.github+json' };" ^
 "if ($env:GITHUB_TOKEN) { $h['Authorization']='Bearer ' + $env:GITHUB_TOKEN } elseif ($env:GH_TOKEN) { $h['Authorization']='Bearer ' + $env:GH_TOKEN };" ^
 "$r=Invoke-RestMethod -Headers $h -Uri ($env:PS_API + '/releases/tags/' + $env:PS_TAG);" ^
 "$lines = $r.assets | ForEach-Object { '{0}|{1}' -f $_.name, $_.url };" ^
 "if (-not $lines) { exit 1 };" ^
 "$lines | Set-Content -LiteralPath $env:PS_OUT -Encoding ASCII"
if errorlevel 1 exit /b 1
if not exist "%PS_OUT%" exit /b 1
exit /b 0

:lookup_asset
set "ASSET_URL="
if not exist "%WORK%\assets.txt" exit /b 1
for /f "usebackq tokens=1,* delims=|" %%A in ("%WORK%\assets.txt") do if /I "%%A"=="%~1" set "ASSET_URL=%%B"
if not defined ASSET_URL exit /b 1
exit /b 0

:compute_latest
set "LATEST_TAG="
for /f "usebackq tokens=1,3 delims=|" %%A in ("%WORK%\releases.txt") do if not defined LATEST_TAG if "%%B"=="0" set "LATEST_TAG=%%A"
if not defined LATEST_TAG for /f "usebackq tokens=1 delims=|" %%A in ("%WORK%\releases.txt") do if not defined LATEST_TAG set "LATEST_TAG=%%A"
if not defined LATEST_TAG exit /b 1
exit /b 0

:count_releases
set "RELEASE_COUNT=0"
for /f "usebackq delims=" %%A in ("%WORK%\releases.txt") do set /a RELEASE_COUNT+=1
if "%RELEASE_COUNT%"=="0" exit /b 1
exit /b 0

:display_releases
echo.
echo Recent releases
for /f "tokens=1,* delims=:" %%A in ('findstr /n "^" "%WORK%\releases.txt"') do call :display_release "%%A" "%%B"
exit /b 0

:display_release
set "RNUM=%~1"
set "RLINE=%~2"
for /f "tokens=1,2,3 delims=|" %%A in ("%RLINE%") do (
  set "RTAG=%%A"
  set "RDATE=%%B"
  set "RPRE=%%C"
)
set "RMARK="
if /I "%RTAG%"=="%LATEST_TAG%" set "RMARK=  latest"
if "%RPRE%"=="1" set "RMARK=%RMARK%  pre-release"
echo   %RNUM%. %RTAG% %RDATE%%RMARK%
exit /b 0

:pick_release
set "PICK_NUM=%~1"
set "VALID="
for /l %%I in (1,1,%RELEASE_COUNT%) do if "%%I"=="%PICK_NUM%" set "VALID=1"
if not defined VALID exit /b 1
set /a SKIP=%PICK_NUM%-1
if %SKIP% LSS 0 exit /b 1
for /f "usebackq skip=%SKIP% delims=" %%A in ("%WORK%\releases.txt") do (
  set "RELEASE_LINE=%%A"
  goto pick_release_found
)
exit /b 1
:pick_release_found
for /f "tokens=1 delims=|" %%A in ("%RELEASE_LINE%") do set "PICKED_TAG=%%A"
if not defined PICKED_TAG exit /b 1
exit /b 0

rem Target metadata -----------------------------------------------------------

:get_target
if "%~1"=="1" (
  set "T_KEY=windows-x64"
  set "T_PLATFORM=windows"
  set "T_ARCH=x64"
  set "T_ARCHNAME=x86_64"
  set "T_LIBC="
  set "T_DIR=windows-x86_64"
  set "T_ASSET=claude-win32-x64.zip"
  set "T_TYPE=zip"
  set "T_BIN=claude.exe"
  set "T_LABEL=Windows x64"
  exit /b 0
)
if "%~1"=="2" (
  set "T_KEY=windows-arm64"
  set "T_PLATFORM=windows"
  set "T_ARCH=arm64"
  set "T_ARCHNAME=arm64"
  set "T_LIBC="
  set "T_DIR=windows-arm64"
  set "T_ASSET=claude-win32-arm64.zip"
  set "T_TYPE=zip"
  set "T_BIN=claude.exe"
  set "T_LABEL=Windows arm64"
  exit /b 0
)
if "%~1"=="3" (
  set "T_KEY=macos-x64"
  set "T_PLATFORM=macos"
  set "T_ARCH=x64"
  set "T_ARCHNAME=x86_64"
  set "T_LIBC="
  set "T_DIR=macos-x86_64"
  set "T_ASSET=claude-darwin-x64.tar.gz"
  set "T_TYPE=tar.gz"
  set "T_BIN=claude"
  set "T_LABEL=macOS x64"
  exit /b 0
)
if "%~1"=="4" (
  set "T_KEY=macos-arm64"
  set "T_PLATFORM=macos"
  set "T_ARCH=arm64"
  set "T_ARCHNAME=arm64"
  set "T_LIBC="
  set "T_DIR=macos-arm64"
  set "T_ASSET=claude-darwin-arm64.tar.gz"
  set "T_TYPE=tar.gz"
  set "T_BIN=claude"
  set "T_LABEL=macOS arm64"
  exit /b 0
)
if "%~1"=="5" (
  set "T_KEY=linux-x64"
  set "T_PLATFORM=linux"
  set "T_ARCH=x64"
  set "T_ARCHNAME=x86_64"
  set "T_LIBC="
  set "T_DIR=linux-x86_64"
  set "T_ASSET=claude-linux-x64.tar.gz"
  set "T_TYPE=tar.gz"
  set "T_BIN=claude"
  set "T_LABEL=Linux x64"
  exit /b 0
)
if "%~1"=="6" (
  set "T_KEY=linux-arm64"
  set "T_PLATFORM=linux"
  set "T_ARCH=arm64"
  set "T_ARCHNAME=arm64"
  set "T_LIBC="
  set "T_DIR=linux-arm64"
  set "T_ASSET=claude-linux-arm64.tar.gz"
  set "T_TYPE=tar.gz"
  set "T_BIN=claude"
  set "T_LABEL=Linux arm64"
  exit /b 0
)
if "%~1"=="7" (
  set "T_KEY=linux-x64-musl"
  set "T_PLATFORM=linux"
  set "T_ARCH=x64"
  set "T_ARCHNAME=x86_64"
  set "T_LIBC=musl"
  set "T_DIR=linux-x86_64-musl"
  set "T_ASSET=claude-linux-x64-musl.tar.gz"
  set "T_TYPE=tar.gz"
  set "T_BIN=claude"
  set "T_LABEL=Linux x64 musl"
  exit /b 0
)
if "%~1"=="8" (
  set "T_KEY=linux-arm64-musl"
  set "T_PLATFORM=linux"
  set "T_ARCH=arm64"
  set "T_ARCHNAME=arm64"
  set "T_LIBC=musl"
  set "T_DIR=linux-arm64-musl"
  set "T_ASSET=claude-linux-arm64-musl.tar.gz"
  set "T_TYPE=tar.gz"
  set "T_BIN=claude"
  set "T_LABEL=Linux arm64 musl"
  exit /b 0
)
exit /b 1

:is_selected
for %%I in (%SELECTED_INDICES%) do if "%%I"=="%~1" exit /b 0
exit /b 1

:add_selected
call :is_selected %1
if not errorlevel 1 exit /b 0
set "SELECTED_INDICES=%SELECTED_INDICES% %1"
set /a SELECTED_COUNT+=1
exit /b 0

:select_all
set "SELECTED_INDICES="
set "SELECTED_COUNT=0"
for /l %%I in (1,1,%TARGET_COUNT%) do call :add_selected %%I
exit /b 0

:add_target_spec
set "SPEC=%~1"
if /I "%SPEC%"=="all" (
  call :select_all
  exit /b 0
)
set "FOUND="
for /l %%I in (1,1,%TARGET_COUNT%) do call :match_target %%I "%SPEC%"
if not defined FOUND (
  echo error: unknown target: %SPEC% 1>&2
  set "BAD_TARGET=1"
  exit /b 1
)
call :add_selected %FOUND%
exit /b 0

:match_target
call :get_target %1
if /I "%T_KEY%"=="%~2" set "FOUND=%1"
exit /b 0

:filter_match
if "%~1"=="" exit /b 0
for %%F in (%~1) do if /I "%%F"=="%~2" exit /b 0
exit /b 1

:try_variant
set "VP=%~1"
set "VA=%~2"
call :filter_match "%PLATFORM_FILTER%" "%VP%"
if errorlevel 1 exit /b 0
call :filter_match "%ARCH_FILTER%" "%VA%"
if errorlevel 1 exit /b 0
call :select_variant "%VP%" "%VA%"
exit /b 0

:select_variant
set "VP=%~1"
set "VA=%~2"
set "BEST="
if defined HOST_INDEX call :match_host "%VP%" "%VA%"
if not defined BEST call :find_variant "%VP%" "%VA%" 1
if not defined BEST call :find_variant "%VP%" "%VA%" 0
if defined BEST call :add_selected %BEST%
exit /b 0

:match_host
call :get_target %HOST_INDEX%
if /I "%T_PLATFORM%"=="%~1" if /I "%T_ARCH%"=="%~2" set "BEST=%HOST_INDEX%"
exit /b 0

:find_variant
for /l %%I in (1,1,%TARGET_COUNT%) do call :check_variant %%I "%~1" "%~2" %3
exit /b 0

:check_variant
call :get_target %1
if /I not "%T_PLATFORM%"=="%~2" exit /b 0
if /I not "%T_ARCH%"=="%~3" exit /b 0
if "%~4"=="1" if not "%T_LIBC%"=="" exit /b 0
if defined BEST exit /b 0
set "BEST=%1"
exit /b 0

:display_targets
echo.
echo Available targets
for /l %%I in (1,1,%TARGET_COUNT%) do call :display_target %%I
exit /b 0

:display_target
call :get_target %1
set "TMARK="
if "%1"=="%HOST_MARK%" set "TMARK=  (detected host)"
echo   %1. %T_LABEL% %T_ASSET%%TMARK%
exit /b 0

:is_valid_index
set "VALID="
for /l %%I in (1,1,%TARGET_COUNT%) do if "%%I"=="%~1" set "VALID=1"
if not defined VALID exit /b 1
exit /b 0

:parse_target_token
set "TOKEN=%~1"
set "HASDASH="
echo %TOKEN%| findstr /C:"-" >nul
if not errorlevel 1 set "HASDASH=1"
if not defined HASDASH goto token_single
for /f "tokens=1,2 delims=-" %%A in ("%TOKEN%") do (
  set "START=%%A"
  set "END=%%B"
)
set "RECON=%START%-%END%"
if not "%RECON%"=="%TOKEN%" goto token_bad
call :is_valid_index "%START%"
if errorlevel 1 goto token_bad
call :is_valid_index "%END%"
if errorlevel 1 goto token_bad
if %START% GTR %END% (
  set "TMP=%START%"
  set "START=%END%"
  set "END=%TMP%"
)
for /l %%I in (%START%,1,%END%) do call :add_selected %%I
exit /b 0
:token_single
call :is_valid_index "%TOKEN%"
if errorlevel 1 goto token_bad
call :add_selected %TOKEN%
exit /b 0
:token_bad
set "BAD_TOKEN=1"
exit /b 1

:target_dest
call :get_target %1
if "%SELECTED_COUNT%"=="1" if defined HOST_INDEX if "%1"=="%HOST_INDEX%" (
  set "DEST_DIR=%BIN%\%T_ARCHNAME%"
  set "DEST_PATH=%BIN%\%T_ARCHNAME%\%T_BIN%"
  exit /b 0
)
set "DEST_DIR=%BIN%\%T_DIR%"
set "DEST_PATH=%BIN%\%T_DIR%\%T_BIN%"
exit /b 0

:count_conflicts
set "CONFLICTS=0"
for %%I in (%SELECTED_INDICES%) do call :check_conflict %%I
exit /b 0

:check_conflict
call :target_dest %1
if exist "%DEST_PATH%" set /a CONFLICTS+=1
exit /b 0

:show_conflict
call :target_dest %1
if exist "%DEST_PATH%" echo   %DEST_PATH%
exit /b 0

:count_pending
set "PENDING=0"
for %%I in (%SELECTED_INDICES%) do call :check_pending %%I
exit /b 0

:check_pending
call :target_dest %1
if not exist "%DEST_PATH%" (
  set /a PENDING+=1
  exit /b 0
)
if "%KEEP_CONFLICTS%"=="1" exit /b 0
set /a PENDING+=1
exit /b 0

:print_plan
echo.
echo Download plan
echo   Launcher: %DEST%
echo   Release:  %TAG_SELECTED%
echo   Targets:  %SELECTED_COUNT%
for %%I in (%SELECTED_INDICES%) do call :print_plan_item %%I
exit /b 0

:print_plan_item
call :target_dest %1
call :get_target %1
set "STATUS="
if exist "%DEST_PATH%" set "STATUS=  overwrite"
if "%KEEP_CONFLICTS%"=="1" if exist "%DEST_PATH%" set "STATUS=  keep"
echo   - %T_LABEL% %DEST_PATH%%STATUS%
exit /b 0

:download_target
call :get_target %1
call :target_dest %1
set "DL_ERROR="
if exist "%DEST_PATH%" if "%KEEP_CONFLICTS%"=="1" (
  set /a SKIPPED+=1
  echo   %T_LABEL% kept
  exit /b 0
)
set /a INDEX+=1
echo.
echo [%INDEX%/%PENDING%] %T_LABEL% %T_ASSET%
set "ARCHIVE=%DLTMP%\%T_ASSET%"
set "URL=%CLAUDEX_DOWNLOAD_BASE%/%TAG_SELECTED%/%T_ASSET%"
call :lookup_asset "%T_ASSET%"
if defined ASSET_URL set "URL=%ASSET_URL%"
if exist "%ARCHIVE%" del /f /q "%ARCHIVE%" >nul 2>&1
curl -fL --retry 2 --retry-delay 1 --connect-timeout 30 --progress-bar -H "Accept: application/octet-stream" -o "%ARCHIVE%" "%URL%"
if errorlevel 1 (
  set "DL_ERROR=download failed"
  goto dl_failed
)
if defined NO_VERIFY goto dl_extract
call :verify_archive
if errorlevel 1 (
  set "DL_ERROR=SHA-256 verification failed"
  goto dl_failed
)
:dl_extract
set "EXTRACT=%DLTMP%\extract-%1"
if exist "%EXTRACT%" rmdir /s /q "%EXTRACT%" >nul 2>&1
mkdir "%EXTRACT%" >nul 2>&1
call :extract_archive "%ARCHIVE%" "%T_TYPE%" "%T_BIN%"
if errorlevel 1 (
  set "DL_ERROR=could not extract archive"
  goto dl_failed
)
call :find_binary "%EXTRACT%" "%T_BIN%"
if errorlevel 1 (
  set "DL_ERROR=binary not found in archive"
  goto dl_failed
)
mkdir "%DEST_DIR%" >nul 2>&1
move /Y "%FOUND%" "%DEST_PATH%" >nul
if errorlevel 1 (
  set "DL_ERROR=could not install binary"
  goto dl_failed
)
rmdir /s /q "%EXTRACT%" >nul 2>&1
del /f /q "%ARCHIVE%" >nul 2>&1
set /a DOWNLOADED+=1
echo   Saved %DEST_PATH%
exit /b 0
:dl_failed
if not defined DL_ERROR set "DL_ERROR=download failed"
if exist "%ARCHIVE%" del /f /q "%ARCHIVE%" >nul 2>&1
if exist "%EXTRACT%" rmdir /s /q "%EXTRACT%" >nul 2>&1
set /a FAILED+=1
echo   Failed: %DL_ERROR% 1>&2
exit /b 1

:verify_archive
if not exist "%DLTMP%\SHASUMS256.txt" exit /b 1
findstr /I /C:"  %T_ASSET%" "%DLTMP%\SHASUMS256.txt" > "%DLTMP%\expected.txt"
if errorlevel 1 exit /b 1
set "EXPECTED="
for /f "usebackq tokens=1,2" %%A in ("%DLTMP%\expected.txt") do if /I "%%B"=="%T_ASSET%" set "EXPECTED=%%A"
if not defined EXPECTED exit /b 1
set "HASH_FILE=%ARCHIVE%"
set "ACTUAL="
for /f "usebackq delims=" %%H in (`powershell -NoProfile -ExecutionPolicy Bypass -Command "(Get-FileHash -LiteralPath $env:HASH_FILE -Algorithm SHA256).Hash.ToLower()"`) do set "ACTUAL=%%H"
if not defined ACTUAL exit /b 1
if /I not "%ACTUAL%"=="%EXPECTED%" exit /b 1
echo   SHA-256 verified
exit /b 0

:extract_archive
set "EA_ARCHIVE=%~1"
set "EA_TYPE=%~2"
set "EA_BIN=%~3"
if /I "%EA_TYPE%"=="tar.gz" goto extract_tar
call :ensure_tar
if defined TAR_EXE (
  call :run_tar -xf "%EA_ARCHIVE%" -C "%EXTRACT%" %EA_BIN% >nul 2>&1
  if not errorlevel 1 exit /b 0
)
:extract_zip_ps
set "ARCHIVE_PATH=%EA_ARCHIVE%"
set "EXTRACT_PATH=%EXTRACT%"
powershell -NoProfile -ExecutionPolicy Bypass -Command "Expand-Archive -LiteralPath $env:ARCHIVE_PATH -DestinationPath $env:EXTRACT_PATH -Force" >nul 2>&1
if errorlevel 1 exit /b 1
exit /b 0
:extract_tar
call :ensure_tar
if not defined TAR_EXE exit /b 1
call :run_tar -xzf "%EA_ARCHIVE%" -C "%EXTRACT%" %EA_BIN% >nul 2>&1
if errorlevel 1 exit /b 1
exit /b 0

:ensure_tar
if defined TAR_EXE exit /b 0
if exist "%SystemRoot%\System32\tar.exe" (
  set "TAR_EXE=%SystemRoot%\System32\tar.exe"
  exit /b 0
)
where tar >nul 2>&1
if errorlevel 1 exit /b 0
set "TAR_EXE=tar"
exit /b 0

:run_tar
if /I "%TAR_EXE%"=="tar" goto run_tar_path
"%TAR_EXE%" %*
exit /b %ERRORLEVEL%
:run_tar_path
tar %*
exit /b %ERRORLEVEL%

:find_binary
set "FOUND="
for /r "%~1" %%F in (%~2) do if not defined FOUND set "FOUND=%%F"
if not defined FOUND exit /b 1
exit /b 0

:cleanup
if defined DLTMP if exist "%DLTMP%" rmdir /s /q "%DLTMP%" >nul 2>&1
if defined WORK if exist "%WORK%" rmdir /s /q "%WORK%" >nul 2>&1
exit /b 0
