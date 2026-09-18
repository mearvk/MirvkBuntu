# Ubuntu White Edition Professional Installer — Windows adapter
# Intended for PowerShell 7+.
# The adapter prefers WSL2 and QEMU when available and does not write disks.

param(
  [Parameter(Mandatory=$true)]
  [ValidateSet('inspect','run-iso','vm')]
  [string]$Operation,
  [string]$Iso
)

$ErrorActionPreference = 'Stop'

function Has-Command([string]$Name) {
  return $null -ne (Get-Command $Name -ErrorAction SilentlyContinue)
}

switch ($Operation) {
  'inspect' {
    Write-Host "OS: Windows"
    Write-Host "Architecture: $env:PROCESSOR_ARCHITECTURE"
    Write-Host ("WSL: " + $(if (Has-Command 'wsl.exe') { 'available' } else { 'unavailable' }))
    Write-Host ("QEMU: " + $(if (Has-Command 'qemu-system-x86_64.exe') { 'available' } else { 'unavailable' }))
  }
  'run-iso' {
    if (-not $Iso -or -not (Test-Path -LiteralPath $Iso -PathType Leaf)) { throw "ISO not found." }
    if (-not (Has-Command 'qemu-system-x86_64.exe')) { throw "QEMU is required for ISO execution on Windows." }
    & qemu-system-x86_64.exe -m 4096 -cdrom $Iso -boot d
  }
  'vm' {
    if (-not $Iso -or -not (Test-Path -LiteralPath $Iso -PathType Leaf)) { throw "ISO not found." }
    if (Has-Command 'qemu-system-x86_64.exe') {
      & qemu-system-x86_64.exe -m 4096 -cdrom $Iso -boot d
    } elseif (Has-Command 'wsl.exe') {
      Write-Host 'WSL is available. ISO installation must be mediated by the Windows/WSL integration layer.'
    } else {
      throw 'No supported virtualization backend found.'
    }
  }
}
