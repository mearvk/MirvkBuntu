$ErrorActionPreference = 'Stop'
$Root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
& (Join-Path $PSScriptRoot 'install-native.ps1')
if (Get-Command java -ErrorAction SilentlyContinue -and Get-Command mvn -ErrorAction SilentlyContinue) {
  Write-Host 'building JavaFX master installer'
  Push-Location (Join-Path $Root 'installer')
  try { & mvn '-B' 'clean' 'package'; if ($LASTEXITCODE -ne 0) { throw 'JavaFX installer build failed' } }
  finally { Pop-Location }
} else { Write-Host 'Java/Maven not available; native installation completed.' }
Write-Host 'Master installation completed.'
