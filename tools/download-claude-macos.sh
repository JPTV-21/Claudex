#!/bin/sh
# Standalone console downloader for the native Claude Code binaries published
# in the official anthropics/claude-code GitHub releases.
#
# macOS entry point. This POSIX shell script also runs on Linux; the Linux
# copy is identical. It auto-detects the host platform/architecture, lets you
# pick a release (latest by default) and one or more targets, verifies
# SHA-256, and asks before overwriting a binary.
#
# Usage: sh tools/download-claude-macos.sh [options]
# Run with --help for the full option list.

set -u
export LC_ALL=C

REPO="${CLAUDEX_REPO:-anthropics/claude-code}"
API_BASE="${CLAUDEX_API_BASE:-https://api.github.com/repos/$REPO}"
DOWNLOAD_BASE="${CLAUDEX_DOWNLOAD_BASE:-https://github.com/$REPO/releases/download}"
UA="claudex-downloader/1.0"
TARGET_COUNT=8

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
DEST_DEFAULT=$ROOT

TOKEN="${GITHUB_TOKEN:-${GH_TOKEN:-}}"

# key|platform|arch|archName|libc|dir|asset|type|binary|label
TARGETS='windows-x64|windows|x64|x86_64|-|windows-x86_64|claude-win32-x64.zip|zip|claude.exe|Windows x64
windows-arm64|windows|arm64|arm64|-|windows-arm64|claude-win32-arm64.zip|zip|claude.exe|Windows arm64
macos-x64|macos|x64|x86_64|-|macos-x86_64|claude-darwin-x64.tar.gz|tar.gz|claude|macOS x64
macos-arm64|macos|arm64|arm64|-|macos-arm64|claude-darwin-arm64.tar.gz|tar.gz|claude|macOS arm64
linux-x64|linux|x64|x86_64|-|linux-x86_64|claude-linux-x64.tar.gz|tar.gz|claude|Linux x64
linux-arm64|linux|arm64|arm64|-|linux-arm64|claude-linux-arm64.tar.gz|tar.gz|claude|Linux arm64
linux-x64-musl|linux|x64|x86_64|musl|linux-x86_64-musl|claude-linux-x64-musl.tar.gz|tar.gz|claude|Linux x64 (musl)
linux-arm64-musl|linux|arm64|arm64|musl|linux-arm64-musl|claude-linux-arm64-musl.tar.gz|tar.gz|claude|Linux arm64 (musl)'

# ---------------------------------------------------------------------------
# Output helpers.
# ---------------------------------------------------------------------------

if [ -t 1 ] && [ -z "${NO_COLOR:-}" ] && [ "${TERM:-}" != dumb ]; then
  BOLD=$(printf '\033[1m')
  GREEN=$(printf '\033[32m')
  YELLOW=$(printf '\033[33m')
  RED=$(printf '\033[31m')
  CYAN=$(printf '\033[36m')
  RESET=$(printf '\033[0m')
else
  BOLD=''
  GREEN=''
  YELLOW=''
  RED=''
  CYAN=''
  RESET=''
fi

say() {
  printf '%s\n' "$*"
}

warn() {
  printf '%swarning:%s %s\n' "$YELLOW" "$RESET" "$*" >&2
}

fatal() {
  printf '%serror:%s %s\n' "$RED" "$RESET" "$*" >&2
  exit 1
}

usage_error() {
  printf '%serror:%s %s\n' "$RED" "$RESET" "$*" >&2
  printf 'Run with --help for usage.\n' >&2
  exit 2
}

platform_label() {
  case "$1" in
    windows) printf 'Windows\n' ;;
    macos) printf 'macOS\n' ;;
    linux) printf 'Linux\n' ;;
    *) printf '%s\n' "$1" ;;
  esac
}

# ---------------------------------------------------------------------------
# Help.
# ---------------------------------------------------------------------------

usage() {
  cat <<EOF
Usage: sh $0 [options]

Download official Claude Code binaries from GitHub releases into the
launcher's bin/ folder. Set claudePath to that folder in claudex-config.json
to use it. macOS and Linux are supported by this POSIX shell script.

Version selection:
  -v, --version <tag|latest>  Release tag, or "latest" (default)
      --list                  List recent releases and exit

Target selection (comma separated or repeated):
      --target <name>         windows-x64, windows-arm64, macos-x64,
                              macos-arm64, linux-x64, linux-arm64,
                              linux-x64-musl, linux-arm64-musl, all
      --platform <name>       windows, macos, linux
      --arch <name>           x64, arm64
  --platform/--arch select one variant per combination (the host's libc
  on the host target, otherwise glibc). Use --target for musl explicitly.
  If neither is given, the downloader asks interactively. When stdin is
  not a terminal it uses the detected host target.

Output:
      --dest <dir>            Launcher root (default: script's parent)
  Files are written to bin/<arch>/claude for one detected host target,
  otherwise to bin/<platform>-<arch>/claude.

Other:
  -u, --update                Latest release, detected target, overwrite
  -y, --yes                   Overwrite existing binaries without asking
      --skip-existing         Keep existing binaries and skip their targets
      --dry-run               Print the plan without downloading
      --no-verify             Skip SHA-256 verification
  -h, --help                  Show this help

Environment:
  GITHUB_TOKEN / GH_TOKEN     Raise the GitHub API rate limit
  HTTPS_PROXY / HTTP_PROXY    Used by curl automatically
  NO_COLOR                    Disable colored output

Examples:
  sh $0
  sh $0 --update
  sh $0 --version v2.1.312 --target linux-x64
  sh $0 --platform linux,macos --arch x64 --yes
  sh $0 --list
EOF
}

# ---------------------------------------------------------------------------
# Host detection.
# ---------------------------------------------------------------------------

HOST_PLATFORM=''
HOST_ARCH=''
HOST_LIBC=''

case "$(uname -s 2>/dev/null || true)" in
  Darwin) HOST_PLATFORM=macos ;;
  Linux) HOST_PLATFORM=linux ;;
esac

case "$(uname -m 2>/dev/null || true)" in
  x86_64|amd64) HOST_ARCH=x64 ;;
  arm64|aarch64) HOST_ARCH=arm64 ;;
esac

if [ "$HOST_PLATFORM" = macos ] && command -v sysctl >/dev/null 2>&1; then
  if [ "$(sysctl -in sysctl.proc_translated 2>/dev/null || echo 0)" = 1 ]; then
    HOST_ARCH=arm64
  fi
fi

if [ "$HOST_PLATFORM" = linux ]; then
  if [ -f /etc/alpine-release ] ||
     (ldd --version 2>&1 || true) | grep -qi musl; then
    HOST_LIBC=musl
  fi
fi

# ---------------------------------------------------------------------------
# Target table helpers.
# ---------------------------------------------------------------------------

target_line() {
  printf '%s\n' "$TARGETS" | sed -n "${1}p"
}

load_target() {
  old_ifs=$IFS
  IFS='|'
  set -f
  set -- $1
  set +f
  IFS=$old_ifs
  T_KEY=$1
  T_PLATFORM=$2
  T_ARCH=$3
  T_ARCHNAME=$4
  T_LIBC=$5
  T_DIR=$6
  T_ASSET=$7
  T_TYPE=$8
  T_BINARY=$9
  shift 9
  T_LABEL=$*
  [ "$T_LIBC" = "-" ] && T_LIBC=''
}

find_host_index() {
  i=1
  while [ "$i" -le "$TARGET_COUNT" ]; do
    load_target "$(target_line "$i")"
    if [ "$T_PLATFORM" = "$HOST_PLATFORM" ] &&
       [ "$T_ARCH" = "$HOST_ARCH" ] &&
       [ "$T_LIBC" = "$HOST_LIBC" ]; then
      printf '%s\n' "$i"
      return 0
    fi
    i=$((i + 1))
  done
  i=1
  while [ "$i" -le "$TARGET_COUNT" ]; do
    load_target "$(target_line "$i")"
    if [ "$T_PLATFORM" = "$HOST_PLATFORM" ] &&
       [ "$T_ARCH" = "$HOST_ARCH" ]; then
      printf '%s\n' "$i"
      return 0
    fi
    i=$((i + 1))
  done
  return 1
}

HOST_INDEX=$(find_host_index || true)

normalize_platform() {
  value=$(printf '%s' "$1" | tr 'A-Z' 'a-z')
  case "$value" in
    win|win32|windows) printf 'windows\n' ;;
    mac|macos|darwin|osx) printf 'macos\n' ;;
    linux) printf 'linux\n' ;;
    *) return 1 ;;
  esac
}

normalize_arch() {
  value=$(printf '%s' "$1" | tr 'A-Z' 'a-z')
  case "$value" in
    x64|x86_64|x86-64|amd64) printf 'x64\n' ;;
    arm64|aarch64) printf 'arm64\n' ;;
    *) return 1 ;;
  esac
}

parse_target_spec() {
  spec=$(printf '%s' "$1" | tr 'A-Z' 'a-z')
  spec=${spec#claude-}
  spec=${spec%.tar.gz}
  spec=${spec%.zip}
  spec=${spec%.tgz}
  libc=''
  case "$spec" in
    *-musl|*_musl)
      libc=musl
      spec=${spec%-musl}
      spec=${spec%_musl}
      ;;
  esac
  platform_part=${spec%%-*}
  rest=${spec#*-}
  [ "$rest" = "$spec" ] && return 1
  platform=$(normalize_platform "$platform_part") || return 1
  arch=$(normalize_arch "$rest") || return 1
  printf '%s|%s|%s\n' "$platform" "$arch" "$libc"
}

strip_zeros() {
  value=$(printf '%s' "$1" | sed 's/^0*//')
  [ -n "$value" ] || value=0
  printf '%s\n' "$value"
}

# ---------------------------------------------------------------------------
# GitHub API.
# ---------------------------------------------------------------------------

api_get_to_file() {
  url=$1
  out=$2
  if [ -n "$TOKEN" ]; then
    code=$(curl -sS -L --connect-timeout 30 --max-time 120 \
      -o "$out" -w '%{http_code}' \
      -H "User-Agent: $UA" \
      -H 'Accept: application/vnd.github+json' \
      -H 'X-GitHub-Api-Version: 2022-11-28' \
      -H "Authorization: Bearer $TOKEN" \
      "$url" 2>/dev/null || printf '000')
  else
    code=$(curl -sS -L --connect-timeout 30 --max-time 120 \
      -o "$out" -w '%{http_code}' \
      -H "User-Agent: $UA" \
      -H 'Accept: application/vnd.github+json' \
      -H 'X-GitHub-Api-Version: 2022-11-28' \
      "$url" 2>/dev/null || printf '000')
  fi
  if [ "$code" = 200 ]; then
    return 0
  fi
  if [ -s "$out" ]; then
    message=$(sed -n 's/.*"message": *"\([^"]*\)".*/\1/p' "$out" | sed -n '1p')
    [ -n "$message" ] && printf 'GitHub API: %s\n' "$message" >&2
  fi
  printf 'GitHub API HTTP %s for %s\n' "$code" "$url" >&2
  return 1
}

api_status() {
  if [ -n "$TOKEN" ]; then
    curl -s --connect-timeout 30 --max-time 30 \
      -o /dev/null -w '%{http_code}' \
      -H "User-Agent: $UA" \
      -H 'Accept: application/vnd.github+json' \
      -H 'X-GitHub-Api-Version: 2022-11-28' \
      -H "Authorization: Bearer $TOKEN" \
      "$1" 2>/dev/null || printf '000'
  else
    curl -s --connect-timeout 30 --max-time 30 \
      -o /dev/null -w '%{http_code}' \
      -H "User-Agent: $UA" \
      -H 'Accept: application/vnd.github+json' \
      -H 'X-GitHub-Api-Version: 2022-11-28' \
      "$1" 2>/dev/null || printf '000'
  fi
}

fetch_latest_tag() {
  api_get_to_file "$API_BASE/releases/latest" "$TMP_API/latest.json" ||
    return 1
  tag=$(sed -n 's/.*"tag_name": *"\([^"]*\)".*/\1/p' \
    "$TMP_API/latest.json" | sed -n '1p')
  [ -n "$tag" ] || return 1
  printf '%s\n' "$tag"
}

fetch_releases() {
  api_get_to_file "$API_BASE/releases?per_page=20" \
    "$TMP_API/releases.json" || return 1

  json=$TMP_API/releases.json
  tags=$TMP_API/tags.txt
  pre=$TMP_API/pre.txt
  dates=$TMP_API/dates.txt
  drafts=$TMP_API/drafts.txt

  # GitHub may return compact or pretty JSON depending on the endpoint and
  # cache. Extract each field independently and pair them by order instead of
  # relying on line breaks.
  grep -oE '"tag_name": *"[^"]*"' "$json" |
    sed -E 's/.*"tag_name": *"([^"]*)".*/\1/' > "$tags" || true
  grep -oE '"prerelease": *(true|false)' "$json" |
    sed -E 's/.*: *(true|false).*/\1/' > "$pre" || true
  grep -oE '"published_at": *("[^"]*"|null)' "$json" |
    sed -E 's/.*"published_at": *"([^"]*)".*/\1/; s/.*null.*//; s/T.*//' \
    > "$dates" || true
  grep -oE '"draft": *(true|false)' "$json" |
    sed -E 's/.*: *(true|false).*/\1/' > "$drafts" || true

  paste -d'|' "$tags" "$pre" "$dates" "$drafts" |
    awk -F'|' 'NF >= 4 && $1 != "" && $4 != "true" {
      print $1 "|" $3 "|" ($2 == "true" ? 1 : 0)
    }' > "$TMP_API/releases.txt"

  [ -s "$TMP_API/releases.txt" ]
}

latest_from_list() {
  tag=$(awk -F'|' '$3 == "0" { print $1; exit }' \
    "$TMP_API/releases.txt")
  [ -n "$tag" ] || tag=$(sed -n '1s/|.*//p' "$TMP_API/releases.txt")
  printf '%s\n' "$tag"
}

normalize_tag() {
  case "$1" in
    v*) printf '%s\n' "$1" ;;
    [0-9]*) printf 'v%s\n' "$1" ;;
    *) printf '%s\n' "$1" ;;
  esac
}

resolve_tag() {
  raw=$1
  normalized=$(normalize_tag "$raw")
  for candidate in "$normalized" "$raw"; do
    [ -n "$candidate" ] || continue
    status=$(api_status "$API_BASE/releases/tags/$candidate")
    if [ "$status" = 200 ]; then
      printf '%s\n' "$candidate"
      return 0
    fi
    [ "$candidate" = "$raw" ] && break
  done
  return 1
}

fetch_asset_urls() {
  api_get_to_file "$API_BASE/releases/tags/$TAG" \
    "$TMP_API/assets.json" || return 1
  grep -oE '"url": *"https://api.github.com/[^"]*/releases/assets/[0-9]+"' \
    "$TMP_API/assets.json" |
    sed -E 's/.*"url": *"([^"]*)".*/\1/' > "$TMP_API/asset_urls.txt" || true
  grep -oE '"name": *"(claude-[^"]*|SHASUMS256\.txt(\.sig)?)"' \
    "$TMP_API/assets.json" |
    sed -E 's/.*"name": *"([^"]*)".*/\1/' > "$TMP_API/asset_names.txt" || true
  paste -d'|' "$TMP_API/asset_urls.txt" "$TMP_API/asset_names.txt" \
    > "$TMP_API/assets.txt"
  [ -s "$TMP_API/assets.txt" ]
}

lookup_asset_url() {
  [ -s "$TMP_API/assets.txt" ] || return 1
  awk -F'|' -v name="$1" '$2 == name { print $1; exit }' \
    "$TMP_API/assets.txt"
}

print_release_list() {
  n=0
  printf '\n%s\n' "${BOLD}Recent releases${RESET}"
  while IFS='|' read -r tag date pre; do
    [ -n "$tag" ] || continue
    n=$((n + 1))
    mark=''
    [ "$tag" = "$LATEST_TAG" ] && mark="  ${GREEN}latest${RESET}"
    [ "$pre" = 1 ] && mark="$mark  ${YELLOW}pre-release${RESET}"
    printf '  %2d. %-12s %s%s\n' "$n" "$tag" "$date" "$mark"
  done < "$TMP_API/releases.txt"
}

choose_version() {
  if [ -n "$VERSION" ]; then
    case "$VERSION" in
      latest|LATEST|Latest)
        TAG=$(fetch_latest_tag) || return 1
        ;;
      *)
        TAG=$(resolve_tag "$VERSION") || return 1
        ;;
    esac
    return 0
  fi

  fetch_releases || return 1
  LATEST_TAG=$(latest_from_list)
  [ -n "$LATEST_TAG" ] || return 1

  if [ "$INTERACTIVE" != 1 ]; then
    TAG=$LATEST_TAG
    return 0
  fi

  print_release_list
  count=$(wc -l < "$TMP_API/releases.txt" | tr -d ' ')
  while :; do
    printf 'Select a version [1-%s] or tag (Enter = %s): ' \
      "$count" "$LATEST_TAG"
    IFS= read -r answer || answer=''
    if [ -z "$answer" ]; then
      TAG=$LATEST_TAG
      return 0
    fi
    case "$answer" in
      latest|LATEST|Latest)
        TAG=$LATEST_TAG
        return 0
        ;;
    esac
    case "$answer" in
      *[!0-9]*)
        resolved=$(resolve_tag "$answer" 2>/dev/null) || resolved=''
        if [ -n "$resolved" ]; then
          TAG=$resolved
          return 0
        fi
        printf 'Release not found: %s\n' "$answer" >&2
        ;;
      *)
        number=$(strip_zeros "$answer")
        if [ "$number" -ge 1 ] 2>/dev/null &&
           [ "$number" -le "$count" ] 2>/dev/null; then
          line=$(sed -n "${number}p" "$TMP_API/releases.txt")
          TAG=${line%%|*}
          return 0
        fi
        printf 'Invalid selection: %s\n' "$answer" >&2
        ;;
    esac
  done
}

# ---------------------------------------------------------------------------
# Target selection.
# ---------------------------------------------------------------------------

print_targets() {
  i=1
  printf '\n%s\n' "${BOLD}Available targets${RESET}"
  while [ "$i" -le "$TARGET_COUNT" ]; do
    load_target "$(target_line "$i")"
    marker=''
    if [ -n "$HOST_INDEX" ] && [ "$i" -eq "$HOST_INDEX" ]; then
      marker="  ${GREEN}(detected host)${RESET}"
    fi
    printf '  %2d. %-20s %-28s%s\n' \
      "$i" "$T_LABEL" "$T_ASSET" "$marker"
    i=$((i + 1))
  done
}

choose_targets_interactive() {
  print_targets
  default_index=${HOST_INDEX:-1}
  while :; do
    printf 'Select targets (comma separated, Enter = %s, all = all): ' \
      "$default_index"
    IFS= read -r answer || answer=''
    [ -n "$answer" ] || answer=$default_index

    case "$(printf '%s' "$answer" | tr 'A-Z' 'a-z')" in
      all)
        SELECTED=''
        i=1
        while [ "$i" -le "$TARGET_COUNT" ]; do
          SELECTED="$SELECTED $i"
          i=$((i + 1))
        done
        break
        ;;
    esac

    SELECTED=''
    ok=1
    normalized=$(printf '%s' "$answer" | tr ',' ' ')
    set -f
    for token in $normalized; do
      case "$token" in
        *-*)
          start=${token%%-*}
          end=${token##*-}
          case "$start$end" in
            *[!0-9]*) ok=0; break ;;
          esac
          start=$(strip_zeros "$start")
          end=$(strip_zeros "$end")
          if [ "$start" -gt "$end" ]; then
            tmp=$start
            start=$end
            end=$tmp
          fi
          if [ "$start" -lt 1 ] || [ "$end" -gt "$TARGET_COUNT" ]; then
            ok=0
            break
          fi
          while [ "$start" -le "$end" ]; do
            SELECTED="$SELECTED $start"
            start=$((start + 1))
          done
          ;;
        *[!0-9]*)
          ok=0
          break
          ;;
        *)
          token=$(strip_zeros "$token")
          if [ "$token" -lt 1 ] || [ "$token" -gt "$TARGET_COUNT" ]; then
            ok=0
            break
          fi
          SELECTED="$SELECTED $token"
          ;;
      esac
    done
    set +f
    if [ "$ok" = 1 ] && [ -n "$SELECTED" ]; then
      SELECTED=$(printf '%s\n' $SELECTED | sort -n | uniq |
        tr '\n' ' ' | sed 's/^ *//;s/ *$//')
      break
    fi
    printf 'Enter numbers 1-%s, ranges like 1-3, or all.\n' \
      "$TARGET_COUNT" >&2
  done
}

select_from_target_specs() {
  case "$TARGET_SPECS" in
    *[!A-Za-z0-9,._-]*) return 1 ;;
  esac
  SELECTED=''
  for spec in $(printf '%s' "$TARGET_SPECS" | tr ',' ' '); do
    case "$(printf '%s' "$spec" | tr 'A-Z' 'a-z')" in
      all)
        SELECTED=''
        i=1
        while [ "$i" -le "$TARGET_COUNT" ]; do
          SELECTED="$SELECTED $i"
          i=$((i + 1))
        done
        SELECTED=$(printf '%s' "$SELECTED" | sed 's/^ *//')
        return 0
        ;;
    esac
    parsed=$(parse_target_spec "$spec") || return 1
    p=${parsed%%|*}
    rest=${parsed#*|}
    a=${rest%%|*}
    l=${rest#*|}
    found=''
    i=1
    while [ "$i" -le "$TARGET_COUNT" ]; do
      load_target "$(target_line "$i")"
      if [ "$T_PLATFORM" = "$p" ] &&
         [ "$T_ARCH" = "$a" ] &&
         [ "$T_LIBC" = "$l" ]; then
        found=$i
        break
      fi
      i=$((i + 1))
    done
    [ -n "$found" ] || return 1
    case " $SELECTED " in
      *" $found "*) ;;
      *) SELECTED="$SELECTED $found" ;;
    esac
  done
  SELECTED=$(printf '%s' "$SELECTED" | sed 's/^ *//')
  [ -n "$SELECTED" ]
}

parse_filters() {
  case "$PLATFORM_FILTER$ARCH_FILTER" in
    *[!A-Za-z0-9,._-]*) return 1 ;;
  esac
  PLATFORMS=''
  for value in $(printf '%s' "$PLATFORM_FILTER" | tr ',' ' '); do
    p=$(normalize_platform "$value") || return 1
    PLATFORMS="$PLATFORMS $p"
  done
  ARCHES=''
  for value in $(printf '%s' "$ARCH_FILTER" | tr ',' ' '); do
    a=$(normalize_arch "$value") || return 1
    ARCHES="$ARCHES $a"
  done
  PLATFORMS=$(printf '%s' "$PLATFORMS" | sed 's/^ *//;s/ *$//')
  ARCHES=$(printf '%s' "$ARCHES" | sed 's/^ *//;s/ *$//')
}

select_from_filters() {
  SELECTED=''
  for key in windows-x64 windows-arm64 macos-x64 macos-arm64 linux-x64 linux-arm64; do
    p=${key%%-*}
    a=${key#*-}
    if [ -n "$PLATFORMS" ]; then
      case " $PLATFORMS " in
        *" $p "*) ;;
        *) continue ;;
      esac
    fi
    if [ -n "$ARCHES" ]; then
      case " $ARCHES " in
        *" $a "*) ;;
        *) continue ;;
      esac
    fi

    best=''
    if [ -n "$HOST_INDEX" ]; then
      load_target "$(target_line "$HOST_INDEX")"
      if [ "$T_PLATFORM" = "$p" ] && [ "$T_ARCH" = "$a" ]; then
        best=$HOST_INDEX
      fi
    fi
    if [ -z "$best" ]; then
      i=1
      while [ "$i" -le "$TARGET_COUNT" ]; do
        load_target "$(target_line "$i")"
        if [ "$T_PLATFORM" = "$p" ] &&
           [ "$T_ARCH" = "$a" ] &&
           [ -z "$T_LIBC" ]; then
          best=$i
          break
        fi
        i=$((i + 1))
      done
    fi
    if [ -z "$best" ]; then
      i=1
      while [ "$i" -le "$TARGET_COUNT" ]; do
        load_target "$(target_line "$i")"
        if [ "$T_PLATFORM" = "$p" ] && [ "$T_ARCH" = "$a" ]; then
          best=$i
          break
        fi
        i=$((i + 1))
      done
    fi
    [ -n "$best" ] && SELECTED="$SELECTED $best"
  done
  SELECTED=$(printf '%s' "$SELECTED" | sed 's/^ *//')
  [ -n "$SELECTED" ]
}

select_targets() {
  if [ -n "$TARGET_SPECS" ]; then
    select_from_target_specs || return 1
    return 0
  fi
  if [ -n "$PLATFORM_FILTER$ARCH_FILTER" ]; then
    parse_filters || return 1
    select_from_filters || return 1
    return 0
  fi
  if [ "$UPDATE" = 1 ]; then
    [ -n "$HOST_INDEX" ] || return 1
    SELECTED=$HOST_INDEX
    return 0
  fi
  if [ "$INTERACTIVE" = 1 ]; then
    choose_targets_interactive
    return 0
  fi
  [ -n "$HOST_INDEX" ] || return 1
  SELECTED=$HOST_INDEX
}

# ---------------------------------------------------------------------------
# Plan and overwrite handling.
# ---------------------------------------------------------------------------

BIN=''

compute_selected_count() {
  SELECTED_COUNT=$(printf '%s\n' $SELECTED | wc -l | tr -d ' ')
}

target_dest() {
  i=$1
  load_target "$(target_line "$i")"
  if [ "$SELECTED_COUNT" -eq 1 ] &&
     [ -n "$HOST_INDEX" ] &&
     [ "$i" -eq "$HOST_INDEX" ]; then
    DEST_PATH=$BIN/$T_ARCHNAME/$T_BINARY
  else
    DEST_PATH=$BIN/$T_DIR/$T_BINARY
  fi
}

count_conflicts() {
  CONFLICTS=0
  CONFLICT_LIST=''
  for i in $SELECTED; do
    target_dest "$i"
    if [ -e "$DEST_PATH" ]; then
      CONFLICTS=$((CONFLICTS + 1))
      CONFLICT_LIST="$CONFLICT_LIST$DEST_PATH
"
    fi
  done
}

resolve_overwrites() {
  KEEP_CONFLICTS=0
  OVERWRITE=0
  [ "$CONFLICTS" -gt 0 ] || return 0

  if [ "$SKIP_EXISTING" = 1 ]; then
    KEEP_CONFLICTS=1
    say "${YELLOW}Keeping existing files (--skip-existing).${RESET}"
    return 0
  fi
  if [ "$YES" = 1 ] || [ "$DRY_RUN" = 1 ]; then
    OVERWRITE=1
    return 0
  fi
  if [ "$INTERACTIVE" != 1 ]; then
    fatal "destination file already exists: $CONFLICT_LIST
Use --yes to overwrite or --skip-existing to keep it."
  fi

  say ''
  say "${YELLOW}These files already exist:${RESET}"
  while IFS= read -r path; do
    [ -n "$path" ] && printf '  %s\n' "$path"
  done <<EOF
$CONFLICT_LIST
EOF
  printf 'Overwrite existing file(s)? [y/N]: '
  IFS= read -r answer || answer=''
  case "$(printf '%s' "$answer" | tr 'A-Z' 'a-z')" in
    y|yes)
      OVERWRITE=1
      ;;
    *)
      KEEP_CONFLICTS=1
      say 'Keeping existing files.'
      ;;
  esac
}

print_plan() {
  printf '\n%s\n' "${BOLD}Download plan${RESET}"
  printf '  Launcher: %s\n' "$DEST"
  printf '  Release:  %s\n' "$TAG"
  printf '  Targets:  %s\n' "$SELECTED_COUNT"
  for i in $SELECTED; do
    target_dest "$i"
    load_target "$(target_line "$i")"
    status=''
    if [ -e "$DEST_PATH" ]; then
      if [ "$KEEP_CONFLICTS" = 1 ]; then
        status="  ${YELLOW}keep${RESET}"
      else
        status="  ${YELLOW}overwrite${RESET}"
      fi
    fi
    printf '  - %-20s %s%s\n' "$T_LABEL" "$DEST_PATH" "$status"
  done
}

count_pending() {
  PENDING=0
  for i in $SELECTED; do
    target_dest "$i"
    if [ -e "$DEST_PATH" ] && [ "$KEEP_CONFLICTS" = 1 ]; then
      continue
    fi
    PENDING=$((PENDING + 1))
  done
}

# ---------------------------------------------------------------------------
# Download, verify, extract, install.
# ---------------------------------------------------------------------------

sha256_of() {
  if command -v sha256sum >/dev/null 2>&1; then
    sha256sum "$1" | awk '{ print $1 }'
  elif command -v shasum >/dev/null 2>&1; then
    shasum -a 256 "$1" | awk '{ print $1 }'
  elif command -v openssl >/dev/null 2>&1; then
    openssl dgst -sha256 "$1" | awk '{ print $NF }'
  else
    return 1
  fi
}

extract_archive() {
  archive=$1
  type=$2
  outdir=$3
  case "$type" in
    tar.gz)
      tar -xzf "$archive" -C "$outdir" || return 1
      ;;
    zip)
      if command -v unzip >/dev/null 2>&1; then
        unzip -q -o "$archive" -d "$outdir" || return 1
      elif command -v bsdtar >/dev/null 2>&1; then
        bsdtar -xf "$archive" -C "$outdir" || return 1
      elif command -v python3 >/dev/null 2>&1; then
        python3 -c 'import sys, zipfile; zipfile.ZipFile(sys.argv[1]).extractall(sys.argv[2])' \
          "$archive" "$outdir" || return 1
      elif command -v 7z >/dev/null 2>&1; then
        7z x -y -o"$outdir" "$archive" >/dev/null || return 1
      else
        printf 'error: no ZIP extractor found (unzip, bsdtar, python3, or 7z)\n' >&2
        return 1
      fi
      ;;
    *)
      return 1
      ;;
  esac
}

download_target() {
  i=$1
  load_target "$(target_line "$i")"
  target_dest "$i"

  if [ -e "$DEST_PATH" ] && [ "$KEEP_CONFLICTS" = 1 ]; then
    SKIP_COUNT=$((SKIP_COUNT + 1))
    printf '  %s (kept)\n' "$T_LABEL"
    return 0
  fi

  INDEX=$((INDEX + 1))
  printf '\n%s[%d/%d]%s %s  %s\n' \
    "$CYAN" "$INDEX" "$PENDING" "$RESET" "$T_LABEL" "$T_ASSET"

  archive=$TMP_DL/$T_ASSET
  url=$(lookup_asset_url "$T_ASSET" 2>/dev/null || true)
  [ -n "$url" ] || url=$DOWNLOAD_BASE/$TAG/$T_ASSET

  if [ -t 1 ]; then
    if ! curl -fL --retry 2 --retry-delay 1 --connect-timeout 30 \
      -H 'Accept: application/octet-stream' \
      --progress-bar -o "$archive" "$url"; then
      rm -f "$archive"
      printf '  %sFailed%s: download failed\n' "$RED" "$RESET" >&2
      printf '  %s\n' "$url" >&2
      FAILED_COUNT=$((FAILED_COUNT + 1))
      return 1
    fi
  else
    if ! curl -fsSL --retry 2 --retry-delay 1 --connect-timeout 30 \
      -H 'Accept: application/octet-stream' \
      -o "$archive" "$url"; then
      rm -f "$archive"
      printf '  %sFailed%s: download failed\n' "$RED" "$RESET" >&2
      printf '  %s\n' "$url" >&2
      FAILED_COUNT=$((FAILED_COUNT + 1))
      return 1
    fi
  fi

  if [ "$VERIFY" = 1 ]; then
    expected=$(awk -v name="$T_ASSET" '
      {
        n = $2
        sub(/^\*/, "", n)
        if (n == name) { print $1; exit }
      }
    ' "$TMP_DL/SHASUMS256.txt")
    if [ -z "$expected" ]; then
      printf '  %sWarning%s: no SHA-256 for %s; skipping verification\n' \
        "$YELLOW" "$RESET" "$T_ASSET"
    else
      actual=$(sha256_of "$archive") || actual=''
      if [ -z "$actual" ]; then
        rm -f "$archive"
        printf '  %sFailed%s: no SHA-256 tool available\n' \
          "$RED" "$RESET" >&2
        FAILED_COUNT=$((FAILED_COUNT + 1))
        return 1
      fi
      actual=$(printf '%s' "$actual" | tr 'A-Z' 'a-z')
      expected=$(printf '%s' "$expected" | tr 'A-Z' 'a-z')
      if [ "$actual" != "$expected" ]; then
        rm -f "$archive"
        printf '  %sFailed%s: SHA-256 mismatch\n' \
          "$RED" "$RESET" >&2
        printf '  expected %s\n  got      %s\n' \
          "$expected" "$actual" >&2
        FAILED_COUNT=$((FAILED_COUNT + 1))
        return 1
      fi
      printf '  %sSHA-256 verified%s\n' "$GREEN" "$RESET"
    fi
  fi

  extract_dir=$TMP_DL/extract-$i
  rm -rf "$extract_dir"
  mkdir -p "$extract_dir" || {
    FAILED_COUNT=$((FAILED_COUNT + 1))
    return 1
  }
  printf '  Extracting %s...\n' "$T_BINARY"
  if ! extract_archive "$archive" "$T_TYPE" "$extract_dir"; then
    rm -rf "$extract_dir"
    rm -f "$archive"
    printf '  %sFailed%s: could not extract archive\n' \
      "$RED" "$RESET" >&2
    FAILED_COUNT=$((FAILED_COUNT + 1))
    return 1
  fi

  found=$(find "$extract_dir" -type f -name "$T_BINARY" | sed -n '1p')
  if [ -z "$found" ]; then
    rm -rf "$extract_dir"
    rm -f "$archive"
    printf '  %sFailed%s: %s not found in archive\n' \
      "$RED" "$RESET" "$T_BINARY" >&2
    FAILED_COUNT=$((FAILED_COUNT + 1))
    return 1
  fi

  chmod 755 "$found" || true
  mkdir -p "$(dirname "$DEST_PATH")" || {
    FAILED_COUNT=$((FAILED_COUNT + 1))
    return 1
  }
  if ! mv -f "$found" "$DEST_PATH"; then
    rm -rf "$extract_dir"
    rm -f "$archive"
    printf '  %sFailed%s: could not install %s\n' \
      "$RED" "$RESET" "$DEST_PATH" >&2
    FAILED_COUNT=$((FAILED_COUNT + 1))
    return 1
  fi

  rm -rf "$extract_dir"
  rm -f "$archive"
  printf '  %sSaved%s %s\n' "$GREEN" "$RESET" "$DEST_PATH"
  DOWNLOADED_COUNT=$((DOWNLOADED_COUNT + 1))
  return 0
}

# ---------------------------------------------------------------------------
# Argument parsing.
# ---------------------------------------------------------------------------

VERSION=''
LIST=0
TARGET_SPECS=''
PLATFORM_FILTER=''
ARCH_FILTER=''
DEST=$DEST_DEFAULT
YES=0
SKIP_EXISTING=0
DRY_RUN=0
VERIFY=1
UPDATE=0

while [ $# -gt 0 ]; do
  case "$1" in
    -h|--help)
      usage
      exit 0
      ;;
    -u|--update)
      UPDATE=1
      shift
      ;;
    -y|--yes)
      YES=1
      shift
      ;;
    -v|--version)
      [ $# -ge 2 ] || usage_error '--version requires a value'
      VERSION=$2
      shift 2
      ;;
    --version=*)
      VERSION=${1#*=}
      [ -n "$VERSION" ] || usage_error '--version requires a value'
      shift
      ;;
    --list)
      LIST=1
      shift
      ;;
    --target)
      [ $# -ge 2 ] || usage_error '--target requires a value'
      [ -n "$2" ] || usage_error '--target requires a value'
      TARGET_SPECS="$TARGET_SPECS,$2"
      shift 2
      ;;
    --target=*)
      value=${1#*=}
      [ -n "$value" ] || usage_error '--target requires a value'
      TARGET_SPECS="$TARGET_SPECS,$value"
      shift
      ;;
    --platform)
      [ $# -ge 2 ] || usage_error '--platform requires a value'
      [ -n "$2" ] || usage_error '--platform requires a value'
      PLATFORM_FILTER="$PLATFORM_FILTER,$2"
      shift 2
      ;;
    --platform=*)
      value=${1#*=}
      [ -n "$value" ] || usage_error '--platform requires a value'
      PLATFORM_FILTER="$PLATFORM_FILTER,$value"
      shift
      ;;
    --arch)
      [ $# -ge 2 ] || usage_error '--arch requires a value'
      [ -n "$2" ] || usage_error '--arch requires a value'
      ARCH_FILTER="$ARCH_FILTER,$2"
      shift 2
      ;;
    --arch=*)
      value=${1#*=}
      [ -n "$value" ] || usage_error '--arch requires a value'
      ARCH_FILTER="$ARCH_FILTER,$value"
      shift
      ;;
    --dest)
      [ $# -ge 2 ] || usage_error '--dest requires a value'
      DEST=$2
      shift 2
      ;;
    --dest=*)
      DEST=${1#*=}
      [ -n "$DEST" ] || usage_error '--dest requires a value'
      shift
      ;;
    --skip-existing)
      SKIP_EXISTING=1
      shift
      ;;
    --dry-run)
      DRY_RUN=1
      shift
      ;;
    --no-verify)
      VERIFY=0
      shift
      ;;
    --)
      shift
      break
      ;;
    -*)
      usage_error "unknown option: $1"
      ;;
    *)
      usage_error "unexpected argument: $1"
      ;;
  esac
done

if [ -n "$VERSION" ] && [ -z "$(printf '%s' "$VERSION" | tr -d ' ')" ]; then
  usage_error '--version requires a tag or "latest"'
fi
if [ -n "$TARGET_SPECS" ] && [ -n "$PLATFORM_FILTER$ARCH_FILTER" ]; then
  usage_error 'use either --target or --platform/--arch, not both'
fi
if [ "$UPDATE" = 1 ]; then
  YES=1
  [ -n "$VERSION" ] || VERSION=latest
fi
if [ "$YES" = 1 ] && [ "$SKIP_EXISTING" = 1 ]; then
  usage_error 'use either --yes or --skip-existing, not both'
fi

case "$DEST" in
  '~') DEST=${HOME:-} ;;
  '~/'*) DEST=${HOME:-}/${DEST#~/} ;;
  /*) ;;
  *) DEST=$(pwd)/$DEST ;;
esac
[ -n "$DEST" ] || usage_error '--dest requires a path'
BIN=$DEST/bin

# ---------------------------------------------------------------------------
# Main.
# ---------------------------------------------------------------------------

main() {
  command -v curl >/dev/null 2>&1 || fatal 'curl is required'
  command -v awk >/dev/null 2>&1 || fatal 'awk is required'
  command -v sed >/dev/null 2>&1 || fatal 'sed is required'
  command -v tar >/dev/null 2>&1 || fatal 'tar is required'
  command -v find >/dev/null 2>&1 || fatal 'find is required'
  command -v mktemp >/dev/null 2>&1 || fatal 'mktemp is required'

  [ -t 0 ] && [ -t 1 ] && INTERACTIVE=1 || INTERACTIVE=0

  TMP_API=$(mktemp -d "${TMPDIR:-/tmp}/claudex-api-XXXXXX") ||
    fatal 'could not create a temporary directory'
  TMP_DL=''
  cleanup() {
    [ -n "$TMP_DL" ] && rm -rf "$TMP_DL"
    [ -n "$TMP_API" ] && rm -rf "$TMP_API"
  }
  trap cleanup EXIT
  trap 'exit 130' INT
  trap 'exit 143' TERM

  printf '%s\n' "${BOLD}Claude Code downloader${RESET}"
  printf '  Repository: %s\n' "$REPO"
  if [ -n "$HOST_PLATFORM" ] && [ -n "$HOST_ARCH" ]; then
    printf '  Host:       %s %s%s\n' \
      "$(platform_label "$HOST_PLATFORM")" "$HOST_ARCH" \
      "${HOST_LIBC:+ ($HOST_LIBC)}"
  else
    printf '  Host:       unsupported; choose a target manually\n'
  fi

  if [ "$LIST" = 1 ]; then
    fetch_releases || fatal 'could not fetch releases'
    LATEST_TAG=$(latest_from_list)
    print_release_list
    return 0
  fi

  choose_version || fatal 'could not resolve the requested release'
  printf '  Release:    %s\n' "$TAG"

  select_targets || fatal 'could not resolve the requested targets'
  compute_selected_count

  count_conflicts
  resolve_overwrites
  print_plan
  count_pending

  if [ "$DRY_RUN" = 1 ]; then
    printf '\nDry run: no files written.\n'
    return 0
  fi
  if [ "$PENDING" -eq 0 ]; then
    printf '\nNothing to do: existing files were kept.\n'
    return 0
  fi

  mkdir -p "$BIN" || fatal "could not create $BIN"
  TMP_DL=$(mktemp -d "$BIN/.claudex-tmp-XXXXXX") ||
    fatal "could not create a temporary folder under $BIN"

  if fetch_asset_urls; then
    :
  else
    warn 'could not resolve API asset URLs; using direct download URLs'
  fi

  if [ "$VERIFY" = 1 ]; then
    shasums_url=$(lookup_asset_url 'SHASUMS256.txt' 2>/dev/null || true)
    [ -n "$shasums_url" ] ||
      shasums_url=$DOWNLOAD_BASE/$TAG/SHASUMS256.txt
    if ! curl -fsSL --retry 2 --retry-delay 1 --connect-timeout 30 \
      -H 'Accept: application/octet-stream' \
      -o "$TMP_DL/SHASUMS256.txt" "$shasums_url"; then
      warn 'could not download SHASUMS256.txt; continuing without verification'
      VERIFY=0
    fi
  fi

  DOWNLOADED_COUNT=0
  SKIP_COUNT=0
  FAILED_COUNT=0
  INDEX=0

  for i in $SELECTED; do
    download_target "$i" || true
  done

  printf '\n'
  if [ "$FAILED_COUNT" -gt 0 ]; then
    printf '%sFinished with errors%s: %d downloaded, %d skipped, %d failed.\n' \
      "$RED" "$RESET" "$DOWNLOADED_COUNT" "$SKIP_COUNT" "$FAILED_COUNT"
    return 1
  fi
  printf '%sDone%s: %d downloaded, %d skipped, %d failed.\n' \
    "$GREEN" "$RESET" "$DOWNLOADED_COUNT" "$SKIP_COUNT" "$FAILED_COUNT"
  return 0
}

if [ "${CLAUDEX_DOWNLOADER_SOURCE_ONLY:-0}" != 1 ]; then
  main "$@"
fi
