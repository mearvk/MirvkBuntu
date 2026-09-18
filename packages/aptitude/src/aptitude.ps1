[CmdletBinding()]
param(
    [Parameter(Position=0)] [ValidateSet('inspect','plan','apply','verify')] [string]$Command,
    [Parameter(Position=1)] [string]$Artifact
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Require-Artifact([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "artifact not found: $Path"
    }
}

function Get-ArtifactInfo([string]$Path) {
    Require-Artifact $Path
    $item = Get-Item -LiteralPath $Path
    $hash = Get-FileHash -LiteralPath $Path -Algorithm SHA256
    Write-Output "artifact=$($item.FullName)"
    Write-Output "sha256=$($hash.Hash.ToLowerInvariant())"
    Write-Output "size=$($item.Length)"
    $extension = $item.Extension.ToLowerInvariant()
    if ($extension -eq '.exe') { Write-Output 'format=PE32/PE32+' }
    elseif ($extension -eq '.msi') { Write-Output 'format=Windows Installer package' }
    elseif ($extension -eq '.ps1') { Write-Output 'format=PowerShell script' }
    else { Write-Output "format=$($item.Extension.TrimStart('.'))" }
}

function Get-Surfaces {
    Write-Output 'host=Windows'
    Write-Output "version=$([System.Environment]::OSVersion.Version)"
    Write-Output "arch=$([System.Runtime.InteropServices.RuntimeInformation]::OSArchitecture)"
    foreach ($surface in @('winget','choco','scoop','java','javac','powershell','pwsh','schtasks','sc.exe')) {
        if (Get-Command $surface -ErrorAction SilentlyContinue) { Write-Output "surface.$surface=available" }
        else { Write-Output "surface.$surface=unavailable" }
    }
    if (Get-Command Get-Service -ErrorAction SilentlyContinue) { Write-Output 'surface.windows_service_manager=available' }
    if (Get-Command Register-ScheduledTask -ErrorAction SilentlyContinue) { Write-Output 'surface.task_scheduler=available' }
    if (Test-Path 'HKLM:\Software\Microsoft\Windows\CurrentVersion\App Paths') { Write-Output 'surface.app_paths=available' }
    if (Test-Path 'HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall') { Write-Output 'surface.uninstall_registry=available' }
}

function Inspect([string]$Path) {
    Write-Output '--- ARTIFACT ---'
    Get-ArtifactInfo $Path
    Write-Output '--- HOST SURFACES ---'
    Get-Surfaces
}

function Plan([string]$Path) {
    Write-Output 'APTITUDE INSTALL PLAN (WINDOWS)'
    Write-Output '=============================='
    Inspect $Path
    Write-Output ''
    Write-Output 'proposed_operations='
    Write-Output '  1. verify artifact identity and SHA-256'
    Write-Output '  2. verify PE/ABI/architecture compatibility'
    Write-Output '  3. resolve required DLL/runtime dependencies'
    Write-Output '  4. select a per-user or machine installation target'
    Write-Output '  5. propose PATH/JAVA_HOME changes when applicable'
    Write-Output '  6. detect Windows Service and Task Scheduler integration opportunities'
    Write-Output '  7. request explicit authorization for machine-wide or privileged changes'
    Write-Output '  8. stage files atomically'
    Write-Output '  9. verify executable, registry, and installed metadata'
    Write-Output ' 10. retain an installation evidence record'
    Write-Output ''
    Write-Output 'decision=dry-run; no system state changed'
}

function Apply([string]$Path) {
    Plan $Path
    Write-Output ''
    Write-Output 'apply_status=not_applied'
    Write-Output 'reason=prototype refuses implicit privileged or persistent system changes'
    Write-Output 'next=review the plan and implement a signed Windows adapter for the desired surface'
}

function Verify([string]$Name) {
    $command = Get-Command $Name -ErrorAction SilentlyContinue
    if (-not $command) { Write-Output "not_found=$Name"; exit 1 }
    Write-Output "verified=$Name"
    try { & $command.Source --version 2>&1 | Select-Object -First 2 } catch { }
}

switch ($Command) {
    'inspect' { if (-not $Artifact) { throw 'artifact is required' }; Inspect $Artifact }
    'plan'    { if (-not $Artifact) { throw 'artifact is required' }; Plan $Artifact }
    'apply'   { if (-not $Artifact) { throw 'artifact is required' }; Apply $Artifact }
    'verify'  { if (-not $Artifact) { throw 'name is required' }; Verify $Artifact }
    default {
        Write-Output 'Aptitude — context-aware installer for Windows'
        Write-Output 'Usage: .\aptitude.ps1 inspect ARTIFACT | plan ARTIFACT | apply ARTIFACT | verify NAME'
        exit 2
    }
}
