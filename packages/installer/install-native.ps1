$ErrorActionPreference = 'Stop'
$Root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Prefix = if ($env:PREFIX) { $env:PREFIX } else { Join-Path $env:ProgramFiles 'UbuntuDeterminant\bin' }
$Cc = if ($env:CC) { $env:CC } else { 'cc' }
New-Item -ItemType Directory -Force -Path $Prefix | Out-Null

$XmcDir = Join-Path $Root 'tools\xmc'
if (Test-Path (Join-Path $XmcDir 'Makefile')) {
  Write-Host 'building xmc'
  & make -C $XmcDir clean all
  if ($LASTEXITCODE -ne 0) { throw 'xmc build failed' }
  & make -C $XmcDir PREFIX=$Prefix install
  if ($LASTEXITCODE -ne 0) { throw 'xmc install failed' }
}

function Build-Tool([string]$Name) {
  $Dir = Join-Path $Root "tools\$Name"
  $Source = Join-Path $Dir "$Name.c"
  if (-not (Test-Path $Source)) { Write-Host "skip: $Name source not present"; return }
  $Output = Join-Path $Prefix "$Name.exe"
  Write-Host "building $Name"
  & $Cc '-O2' '-Wall' '-Wextra' '-Werror' '-std=c11' '-o' $Output $Source
  if ($LASTEXITCODE -ne 0) { throw "build failed: $Name" }
}

Build-Tool 'limit'
Build-Tool 'size'
Build-Tool 'ctrmsctl'

$GccDir = Join-Path $Root 'tools\gcc'
Copy-Item (Join-Path $GccDir 'download-gcc.sh') $Prefix -Force
Copy-Item (Join-Path $GccDir 'extract-gcc.sh') $Prefix -Force
Write-Host "Native tools installed to $Prefix"
