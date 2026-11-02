#requires -Version 5.1
<#
.SYNOPSIS
    ClaudeX 便携版一键部署（Windows 原生 PowerShell 版）

.DESCRIPTION
    下载并安装 ClaudeX 与 Claude Code 到本脚本所在目录，生成便携配置。
    不依赖 Git Bash、perl 或外部解压工具；只使用 Windows 自带的
    PowerShell 和（可选的）curl.exe。

.EXAMPLE
    .\setup-portable.ps1

.EXAMPLE
    .\setup-portable.ps1 -NoClaude

.EXAMPLE
    .\setup-portable.ps1 -Update
#>
[CmdletBinding()]
param(
    [switch]$Update,
    [switch]$NoClaudex,
    [switch]$NoClaude,
    [string]$Repo = 'JPTV-21/Claudex',
    [string]$ClaudeRepo = 'anthropics/claude-code',
    [switch]$Help
)

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

function Write-Step { param([string]$Message) Write-Host "==> $Message" -ForegroundColor Cyan }
function Write-Ok   { param([string]$Message) Write-Host "  ok $Message" -ForegroundColor Green }
function Write-Warn { param([string]$Message) Write-Host "warning: $Message" -ForegroundColor Yellow }
function Write-Err  { param([string]$Message) Write-Host "error: $Message" -ForegroundColor Red }

function Show-Usage {
    @'
用法: powershell -ExecutionPolicy Bypass -File setup-portable.ps1 [选项]

一键部署便携版 ClaudeX + Claude Code 到本脚本所在目录。
本版本使用 Windows 原生 PowerShell，不需要 Git Bash。

选项:
  -Update               强制重新下载 ClaudeX 与 Claude Code
  -NoClaudex            跳过 ClaudeX 的下载/更新
  -NoClaude             跳过 Claude Code 的下载/更新
  -Repo OWNER/REPO      指定 ClaudeX 仓库（默认 JPTV-21/Claudex）
  -ClaudeRepo OWNER/REPO
                        指定 Claude Code 仓库（默认 anthropics/claude-code）
  -Help                 显示本帮助

示例:
  .\setup-portable.ps1
  .\setup-portable.ps1 -NoClaude
  .\setup-portable.ps1 -Update
'@ | Write-Host
}

function Get-NormalizedArch {
    $raw = if ($env:PROCESSOR_ARCHITEW6432) {
        $env:PROCESSOR_ARCHITEW6432
    } else {
        $env:PROCESSOR_ARCHITECTURE
    }

    switch ($raw.ToUpperInvariant()) {
        'AMD64' { return 'x64' }
        'ARM64' { return 'arm64' }
        'X86'   { throw '不支持 32 位 Windows；ClaudeX 只提供 x64 和 arm64 版本。' }
        default { throw "无法识别的 CPU 架构：$raw" }
    }
}

function Get-CurlPath {
    $cmd = Get-Command curl.exe -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    return $null
}

function Invoke-DownloadWithRetry {
    param(
        [Parameter(Mandatory = $true)][string]$Url,
        [Parameter(Mandatory = $true)][string]$Destination,
        [Parameter(Mandatory = $true)][string]$Label,
        [hashtable]$Headers = @{},
        [int]$Attempts = 3
    )

    $curl = Get-CurlPath
    for ($i = 1; $i -le $Attempts; $i++) {
        try {
            if ($curl) {
                $curlArgs = @(
                    '-fL', '-sS',
                    '--retry', '2', '--retry-delay', '1',
                    '--connect-timeout', '30',
                    '--speed-limit', '1024', '--speed-time', '120',
                    '-C', '-',
                    '-H', 'User-Agent: claudex-setup'
                )
                foreach ($key in $Headers.Keys) {
                    $curlArgs += @('-H', "${key}: $($Headers[$key])")
                }
                $curlArgs += @('-o', $Destination, $Url)
                & $curl @curlArgs
                if ($LASTEXITCODE -eq 0) { return }
                # curl 33: ranges unsupported; curl 22: HTTP error such as 416
                # when the partial file is already complete. Restart cleanly.
                if (($LASTEXITCODE -eq 33 -or $LASTEXITCODE -eq 22) -and
                    (Test-Path -LiteralPath $Destination)) {
                    Remove-Item -LiteralPath $Destination -Force -ErrorAction SilentlyContinue
                }
                throw "curl exit $LASTEXITCODE"
            }

            # Fallback without curl. Invoke-WebRequest cannot resume partial files.
            if (Test-Path -LiteralPath $Destination) {
                Remove-Item -LiteralPath $Destination -Force -ErrorAction SilentlyContinue
            }
            Invoke-WebRequest -Uri $Url -OutFile $Destination -UseBasicParsing -TimeoutSec 120
            return
        } catch {
            if ($i -lt $Attempts) {
                Write-Warn "$Label 下载中断（尝试 $i/$Attempts），2 秒后继续…"
                Start-Sleep -Seconds 2
            }
        }
    }

    throw "$Label 下载失败：$Url"
}

function Get-ReleaseInfo {
    param([string]$Repository)

    $curl = Get-CurlPath
    if (-not $curl) {
        throw '找不到 curl.exe；请使用 Windows 10 1803 或更高版本。'
    }

    $out = Join-Path $script:WorkDir ("release-" + [guid]::NewGuid().ToString('N').Substring(0, 6) + '.json')
    $url = "https://api.github.com/repos/$Repository/releases/latest"
    $token = if ($env:GITHUB_TOKEN) { $env:GITHUB_TOKEN } else { $env:GH_TOKEN }

    $curlArgs = @(
        '-fsSL', '--retry', '3', '--retry-delay', '1',
        '--connect-timeout', '20', '--max-time', '60',
        '-H', 'User-Agent: claudex-setup',
        '-H', 'Accept: application/vnd.github+json'
    )
    if ($token) { $curlArgs += @('-H', "Authorization: Bearer $token") }
    $curlArgs += @('-o', $out, $url)

    & $curl @curlArgs
    if ($LASTEXITCODE -ne 0) {
        throw "无法获取 $Repository 的 release 信息（GitHub API 不可达）。"
    }

    return (Get-Content -LiteralPath $out -Raw -Encoding UTF8 | ConvertFrom-Json)
}

function Get-ReleaseAsset {
    param($Release, [string]$Name)

    if (-not $Release -or -not $Release.assets) { return $null }
    return $Release.assets |
        Where-Object { $_.name -ieq $Name } |
        Select-Object -First 1
}

function Save-ReleaseAsset {
    param($Asset, [string]$Destination, [string]$Label)

    if (-not $Asset) { throw "$Label：release 中找不到该资产。" }

    # Prefer the API endpoint: it redirects straight to the asset host and
    # works even when github.com itself is unreachable.
    if ($Asset.url) {
        try {
            Invoke-DownloadWithRetry -Url $Asset.url -Destination $Destination -Label $Label `
                -Headers @{ 'Accept' = 'application/octet-stream' }
            return
        } catch {
            Write-Warn "$Label：API 下载失败，尝试直链。"
        }
    }

    if ($Asset.browser_download_url) {
        Invoke-DownloadWithRetry -Url $Asset.browser_download_url -Destination $Destination -Label $Label
        return
    }

    throw "$Label：没有可用的下载地址。"
}

function Test-AssetHash {
    param(
        $SumsAsset,
        [string]$Asset,
        [string]$Path
    )

    if (-not $SumsAsset) {
        Write-Warn "$Asset：release 中没有 SHASUMS256.txt，跳过校验。"
        return
    }

    $sumFile = Join-Path $script:WorkDir 'SHASUMS256.txt'
    try {
        Save-ReleaseAsset -Asset $SumsAsset -Destination $sumFile -Label "$Asset 的校验文件"
    } catch {
        Write-Warn "$Asset：无法获取 SHASUMS256.txt，跳过校验。"
        return
    }

    $line = Get-Content -LiteralPath $sumFile |
        Where-Object { $_ -match "\s\*?$([regex]::Escape($Asset))\s*$" } |
        Select-Object -First 1
    if (-not $line) {
        Write-Warn "$Asset：SHASUMS256.txt 中没有该文件，跳过校验。"
        return
    }

    $expected = ($line -split '\s+')[0].ToLowerInvariant()
    $actual = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actual -ne $expected) {
        throw "$Asset SHA-256 校验失败`n  期望: $expected`n  实际: $actual"
    }
    Write-Ok "$Asset：SHA-256 校验通过"
}

function Install-ClaudeX {
    $target = Join-Path $script:Root 'ClaudeX.exe'

    if ((Test-Path -LiteralPath $target) -and -not $Update) {
        Write-Ok "ClaudeX 已存在，跳过下载（使用 -Update 强制更新）"
        return
    }

    $release = Get-ReleaseInfo -Repository $Repo
    $assetName = "ClaudeX-windows-$script:Arch.zip"
    $asset = Get-ReleaseAsset -Release $release -Name $assetName
    if (-not $asset) { throw "release 中没有 $assetName。" }

    $zip = Join-Path $script:WorkDir $assetName
    $extractDir = Join-Path $script:WorkDir 'claudex-extract'

    Write-Step "下载 ClaudeX $($release.tag_name)（$assetName）"
    Save-ReleaseAsset -Asset $asset -Destination $zip -Label 'ClaudeX'
    if (-not (Test-Path -LiteralPath $zip)) { throw 'ClaudeX 下载文件不存在。' }
    Test-AssetHash -SumsAsset (Get-ReleaseAsset -Release $release -Name 'SHASUMS256.txt') `
        -Asset $assetName -Path $zip

    if (Test-Path -LiteralPath $extractDir) {
        Remove-Item -LiteralPath $extractDir -Recurse -Force
    }
    Expand-Archive -LiteralPath $zip -DestinationPath $extractDir -Force

    $exe = Get-ChildItem -LiteralPath $extractDir -Recurse -File |
        Where-Object { $_.Name -ieq 'ClaudeX.exe' } |
        Select-Object -First 1
    if (-not $exe) { throw 'ClaudeX 压缩包中找不到 ClaudeX.exe。' }

    Copy-Item -LiteralPath $exe.FullName -Destination $target -Force
    Write-Ok "ClaudeX 已安装：$target"
}

function Install-ClaudeCode {
    $destDir = Join-Path $script:Root "bin\$script:Arch\windows"
    $dest = Join-Path $destDir 'claude.exe'

    if ((Test-Path -LiteralPath $dest) -and -not $Update) {
        Write-Ok "Claude Code 已存在，跳过下载（使用 -Update 强制更新）"
        return
    }

    $claudeAssetName = if ($script:Arch -eq 'arm64') {
        'claude-win32-arm64.zip'
    } else {
        'claude-win32-x64.zip'
    }

    $release = Get-ReleaseInfo -Repository $ClaudeRepo
    $asset = Get-ReleaseAsset -Release $release -Name $claudeAssetName
    if (-not $asset) { throw "release 中没有 $claudeAssetName。" }

    $zip = Join-Path $script:WorkDir $claudeAssetName
    $extractDir = Join-Path $script:WorkDir 'claude-extract'

    Write-Step "下载 Claude Code $($release.tag_name)（$claudeAssetName）"
    Save-ReleaseAsset -Asset $asset -Destination $zip -Label 'Claude Code'
    if (-not (Test-Path -LiteralPath $zip)) { throw 'Claude Code 下载文件不存在。' }
    Test-AssetHash -SumsAsset (Get-ReleaseAsset -Release $release -Name 'SHASUMS256.txt') `
        -Asset $claudeAssetName -Path $zip

    if (Test-Path -LiteralPath $extractDir) {
        Remove-Item -LiteralPath $extractDir -Recurse -Force
    }
    Expand-Archive -LiteralPath $zip -DestinationPath $extractDir -Force

    $exe = Get-ChildItem -LiteralPath $extractDir -Recurse -File |
        Where-Object { $_.Name -ieq 'claude.exe' } |
        Select-Object -First 1
    if (-not $exe) { throw 'Claude Code 压缩包中找不到 claude.exe。' }

    New-Item -ItemType Directory -Force -Path $destDir | Out-Null
    Copy-Item -LiteralPath $exe.FullName -Destination $dest -Force
    Write-Ok "Claude Code 已安装：$dest"
}

function Write-LauncherConfig {
    $configPath = Join-Path $script:Root 'claudex-config.json'
    $claudePath = "bin/$script:Arch/windows/claude*"

    $cfg = $null
    if (Test-Path -LiteralPath $configPath) {
        try {
            $cfg = Get-Content -LiteralPath $configPath -Raw -Encoding UTF8 | ConvertFrom-Json
        } catch {
            $backup = "$configPath.invalid.$(Get-Date -Format 'yyyyMMdd-HHmmss')"
            Move-Item -LiteralPath $configPath -Destination $backup -Force
            Write-Warn "现有配置无法解析，已备份到 $backup"
            $cfg = $null
        }
    }

    if (-not $cfg) {
        $cfg = [pscustomobject]@{
            version              = 1
            theme                = 'claudex-theme.json'
            claudePath           = $claudePath
            claudeExclude        = @('ClaudeX*')
            launchCommand        = [pscustomobject]@{
                windows = 'set "CLAUDE_CONFIG_DIR={claudeConfigDir}" && set "TEMP={tempDir}" && set "TMP={tempDir}" && set "TMPDIR={tempDir}" && {claude}'
                posix   = "CLAUDE_CONFIG_DIR='{claudeConfigDir}' TEMP='{tempDir}' TMP='{tempDir}' TMPDIR='{tempDir}' {claude}"
            }
            claudeConfigDir      = 'data'
            tempDir              = 'data/tmp'
            logFile              = 'claudex.log'
            tabOnlySectionSwitch = $false
            projects             = @()
        }
    } else {
        Copy-Item -LiteralPath $configPath -Destination "$configPath.before-setup" -Force
        $cfg | Add-Member -NotePropertyName claudePath -NotePropertyValue $claudePath -Force
        $cfg | Add-Member -NotePropertyName claudeConfigDir -NotePropertyValue 'data' -Force
        $cfg | Add-Member -NotePropertyName tempDir -NotePropertyValue 'data/tmp' -Force
    }

    $json = $cfg | ConvertTo-Json -Depth 20
    # nlohmann/json rejects a UTF-8 BOM, so write without one.
    $utf8NoBom = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($configPath, $json, $utf8NoBom)
    Write-Ok "配置已写入：$configPath"
}

function Show-Summary {
    $launcher = Join-Path $script:Root 'ClaudeX.exe'
    $claude = Join-Path $script:Root "bin\$script:Arch\windows\claude.exe"
    $config = Join-Path $script:Root 'claudex-config.json'

    Write-Host ''
    Write-Host '================ 完成 ================' -ForegroundColor Green
    Write-Host "  平台:        Windows $script:Arch"
    Write-Host "  脚本目录:    $script:Root"
    Write-Host ('  ClaudeX:     ' + $(if (Test-Path -LiteralPath $launcher) { $launcher } else { '未安装' }))
    Write-Host ('  Claude Code: ' + $(if (Test-Path -LiteralPath $claude) { $claude } else { '未安装' }))
    Write-Host "  配置文件:    $config"
    Write-Host "    claudeConfigDir = data"
    Write-Host "    claudePath      = bin/$script:Arch/windows/claude*"
    if (Test-Path -LiteralPath $launcher) {
        Write-Host ''
        Write-Host "启动方式：$launcher"
    }
}

$script:Root = $null
$script:Arch = $null
$script:WorkDir = $null

try {
    if ($Help) {
        Show-Usage
        exit 0
    }

    $script:Root = if ($PSScriptRoot) { $PSScriptRoot } else { (Get-Location).Path }
    $script:Arch = Get-NormalizedArch
    $script:WorkDir = Join-Path ([System.IO.Path]::GetTempPath()) ("claudex-setup-" + [guid]::NewGuid().ToString('N').Substring(0, 8))

    New-Item -ItemType Directory -Force -Path $script:WorkDir | Out-Null

    Write-Host '便携版 Claude Code 一键部署（PowerShell）' -ForegroundColor White
    Write-Host "  脚本目录: $script:Root"
    Write-Host "  检测到:   Windows $script:Arch"
    Write-Host ''

    if (-not $NoClaudex) { Install-ClaudeX }
    if (-not $NoClaude) { Install-ClaudeCode }
    Write-LauncherConfig
    Show-Summary
} catch {
    Write-Err $_.Exception.Message
    exit 1
} finally {
    if ($script:WorkDir -and (Test-Path -LiteralPath $script:WorkDir)) {
        Remove-Item -LiteralPath $script:WorkDir -Recurse -Force -ErrorAction SilentlyContinue
    }
}
