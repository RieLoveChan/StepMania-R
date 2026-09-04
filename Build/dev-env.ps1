# dev-env.ps1 -- sets up a command-line MSVC + Windows SDK build
# environment (INCLUDE/LIB/PATH) for Ninja / `cmake --build` from a plain
# PowerShell prompt, without opening a Visual Studio "Developer" shell.
#
# Why this exists (DocsAgents/modernization-backlog.md item 14):
# `vcvars64.bat` alone only sets the MSVC toolchain's own INCLUDE/LIB; it
# does *not* set the Windows SDK paths (WindowsSdkDir ends up empty). The
# VS-generator build (`cmake -B build` with the default "Visual Studio 17
# 2022" generator + `cmake --build`, i.e. MSBuild) works anyway, because
# MSBuild resolves the SDK itself via .vcxproj properties. Ninja and raw
# `cl.exe`/`link.exe` invocations have no such fallback and need
# INCLUDE/LIB/PATH set by hand -- this script does that, instead of
# requiring everyone to reconstruct it from DocsAgents/baseline.md's
# "Environment gotchas" section.
#
# Usage: dot-source it so the env vars land in your current shell:
#   . .\Build\dev-env.ps1
#
# Discovers the VS install via vswhere.exe and the newest installed
# Windows 10/11 SDK version by listing Windows Kits\10\Include -- neither
# is hardcoded, so this doesn't go stale on a toolchain/SDK bump the way
# a copy-pasted path would.

$ErrorActionPreference = 'Stop'

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    throw "vswhere.exe not found at '$vswhere' -- is Visual Studio / Build Tools installed?"
}

$vsInstallPath = & $vswhere -latest -products * `
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
    -property installationPath
if (-not $vsInstallPath) {
    throw "vswhere found no VS install with the VC.Tools.x86.x64 component."
}

$vcvars64 = Join-Path $vsInstallPath 'VC\Auxiliary\Build\vcvars64.bat'
if (-not (Test-Path $vcvars64)) {
    throw "vcvars64.bat not found under '$vsInstallPath'."
}

# Run vcvars64.bat in a throwaway cmd.exe, then dump the resulting
# environment with `set` so we can import it into this PowerShell process.
# This is the standard way to source a .bat's env vars from PowerShell --
# there's no PowerShell-native equivalent to `call vcvars64.bat`. Full
# path because cmd.exe isn't guaranteed to be on PATH (it isn't on the
# maintainer's box -- DocsAgents/baseline.md "Environment gotchas").
$cmdExe = Join-Path $env:SystemRoot 'System32\cmd.exe'
$envDump = & $cmdExe /c "`"$vcvars64`" && set" 2>$null
foreach ($line in $envDump) {
    if ($line -match '^([^=]+)=(.*)$') {
        [System.Environment]::SetEnvironmentVariable($Matches[1], $Matches[2], 'Process')
    }
}

if (-not $env:WindowsSdkDir) {
    # vcvars64.bat didn't set it either (the actual bug this script works
    # around) -- fall back to the default Kits install location.
    $kitsRoot = 'C:\Program Files (x86)\Windows Kits\10'
    if (-not (Test-Path $kitsRoot)) {
        throw "Windows SDK not found at '$kitsRoot' and WindowsSdkDir wasn't set by vcvars64.bat."
    }

    $sdkVersion = Get-ChildItem (Join-Path $kitsRoot 'Include') -Directory |
        Where-Object { $_.Name -match '^\d+\.\d+\.\d+\.\d+$' } |
        Sort-Object { [version]$_.Name } -Descending |
        Select-Object -First 1 -ExpandProperty Name
    if (-not $sdkVersion) {
        throw "No versioned SDK found under '$kitsRoot\Include'."
    }

    $arch = 'x64'
    $env:WindowsSdkDir = "$kitsRoot\"
    $env:WindowsSDKVersion = "$sdkVersion\"
    $env:INCLUDE = @(
        $env:INCLUDE
        "$kitsRoot\Include\$sdkVersion\ucrt"
        "$kitsRoot\Include\$sdkVersion\um"
        "$kitsRoot\Include\$sdkVersion\shared"
        "$kitsRoot\Include\$sdkVersion\winrt"
        "$kitsRoot\Include\$sdkVersion\cppwinrt"
    ) -join ';'
    $env:LIB = @(
        $env:LIB
        "$kitsRoot\Lib\$sdkVersion\ucrt\$arch"
        "$kitsRoot\Lib\$sdkVersion\um\$arch"
    ) -join ';'
    $env:PATH = @(
        $env:PATH
        "$kitsRoot\bin\$sdkVersion\$arch"
    ) -join ';'

    Write-Host "Windows SDK $sdkVersion wired in by hand (vcvars64.bat left WindowsSdkDir empty)." -ForegroundColor Yellow
} else {
    Write-Host "Windows SDK $($env:WindowsSDKVersion) already set by vcvars64.bat -- nothing to patch." -ForegroundColor Green
}

Write-Host "MSVC + Windows SDK dev environment ready (VS install: $vsInstallPath)." -ForegroundColor Green
