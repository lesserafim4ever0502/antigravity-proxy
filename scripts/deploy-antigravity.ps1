# Deploy built Antigravity-Proxy artifacts to Antigravity app directories.

[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [string]$OutputDir = (Join-Path $PSScriptRoot "..\output"),
    [string]$IdeDir = "E:\download\Antigravity",
    [string]$CliDir = "$env:LOCALAPPDATA\agy\bin",
    [string]$DesktopDir = "$env:LOCALAPPDATA\Programs\antigravity",
    [switch]$IncludeDesktop,
    [switch]$Force
)

$ErrorActionPreference = "Stop"

function Resolve-FullPath {
    param([Parameter(Mandatory = $true)][string]$Path)
    return [System.IO.Path]::GetFullPath((Resolve-Path -LiteralPath $Path).Path)
}

function Get-NormalizedPath {
    param([Parameter(Mandatory = $true)][string]$Path)
    return [System.IO.Path]::GetFullPath($Path)
}

function Copy-ProxyArtifact {
    param(
        [Parameter(Mandatory = $true)][string]$Source,
        [Parameter(Mandatory = $true)][string]$Destination
    )

    if (Test-Path -LiteralPath $Destination) {
        $timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
        $backup = "$Destination.$timestamp.bak"
        Copy-Item -LiteralPath $Destination -Destination $backup -Force
        Write-Host "Backed up: $Destination -> $backup"
    }

    Copy-Item -LiteralPath $Source -Destination $Destination -Force
    Write-Host "Copied: $Source -> $Destination"
}

function Deploy-ToTarget {
    param(
        [Parameter(Mandatory = $true)][string]$TargetDir,
        [Parameter(Mandatory = $true)][string]$ExeName,
        [Parameter(Mandatory = $true)][string]$DllSource,
        [Parameter(Mandatory = $true)][string]$ConfigSource
    )

    if (-not (Test-Path -LiteralPath $TargetDir)) {
        Write-Warning "Skip missing directory: $TargetDir"
        return
    }

    $exePath = Join-Path $TargetDir $ExeName
    if (-not (Test-Path -LiteralPath $exePath)) {
        Write-Warning "Skip $TargetDir because $ExeName was not found"
        return
    }

    $dllDest = Join-Path $TargetDir "version.dll"
    $configDest = Join-Path $TargetDir "config.json"
    $targetFull = Resolve-FullPath -Path $TargetDir

    if (-not $Force) {
        $answer = Read-Host "Deploy to $targetFull ? This will back up and overwrite version.dll/config.json if present. Type YES to continue"
        if ($answer -ne "YES") {
            Write-Host "Skipped: $targetFull"
            return
        }
    }

    if ($PSCmdlet.ShouldProcess($targetFull, "Deploy version.dll and config.json")) {
        Copy-ProxyArtifact -Source $DllSource -Destination $dllDest
        Copy-ProxyArtifact -Source $ConfigSource -Destination $configDest
    }
}

function Deploy-CliLauncher {
    param(
        [Parameter(Mandatory = $true)][string]$TargetDir,
        [Parameter(Mandatory = $true)][string]$LauncherSource
    )

    if (-not (Test-Path -LiteralPath $TargetDir)) {
        Write-Warning "Skip missing CLI directory: $TargetDir"
        return
    }

    $launcherDest = Join-Path $TargetDir "agy-proxy-launcher.exe"
    $shortAliasDest = Join-Path $TargetDir "agyp.exe"
    if ($PSCmdlet.ShouldProcess($TargetDir, "Deploy agy-proxy-launcher.exe")) {
        Copy-ProxyArtifact -Source $LauncherSource -Destination $launcherDest
        Copy-ProxyArtifact -Source $LauncherSource -Destination $shortAliasDest
    }
}

$outputFull = Get-NormalizedPath -Path $OutputDir
$dllSource = Join-Path $outputFull "version.dll"
$configSource = Join-Path $outputFull "config.json"
$launcherSource = Join-Path $outputFull "agy-proxy-launcher.exe"

if (-not (Test-Path -LiteralPath $dllSource)) {
    throw "Missing artifact: $dllSource. Run .\build.ps1 first."
}
if (-not (Test-Path -LiteralPath $configSource)) {
    throw "Missing artifact: $configSource. Run .\build.ps1 first."
}
if (-not (Test-Path -LiteralPath $launcherSource)) {
    throw "Missing artifact: $launcherSource. Run .\build.ps1 first."
}

Deploy-ToTarget -TargetDir $IdeDir -ExeName "Antigravity IDE.exe" -DllSource $dllSource -ConfigSource $configSource
Deploy-ToTarget -TargetDir $CliDir -ExeName "agy.exe" -DllSource $dllSource -ConfigSource $configSource
Deploy-CliLauncher -TargetDir $CliDir -LauncherSource $launcherSource

if ($IncludeDesktop) {
    Deploy-ToTarget -TargetDir $DesktopDir -ExeName "Antigravity.exe" -DllSource $dllSource -ConfigSource $configSource
} else {
    Write-Host "Desktop Antigravity 2.0 deployment skipped. Pass -IncludeDesktop to deploy there for diagnostics."
}
