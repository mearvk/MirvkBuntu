[CmdletBinding()]
param(
    [string]$Root = "user-interface\sources\vendor"
)

$ErrorActionPreference = "Stop"

if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    throw "git is required. Install Git for Windows and run this script again."
}

New-Item -ItemType Directory -Force -Path $Root | Out-Null

function Clone-Or-Update([string]$Name, [string]$Url) {
    $Destination = Join-Path $Root $Name
    if (Test-Path (Join-Path $Destination ".git")) {
        git -C $Destination fetch --tags --prune
        if ($LASTEXITCODE -ne 0) { throw "git fetch failed for $Name" }
        git -C $Destination pull --ff-only
        if ($LASTEXITCODE -ne 0) { throw "git pull failed for $Name" }
    } else {
        git clone --depth 1 $Url $Destination
        if ($LASTEXITCODE -ne 0) { throw "git clone failed for $Name" }
    }
}

Clone-Or-Update "cockpit" "https://github.com/cockpit-project/cockpit.git"
Clone-Or-Update "openbao" "https://github.com/openbao/openbao.git"
Clone-Or-Update "gitlab-foss" "https://gitlab.com/gitlab-org/gitlab-foss.git"

Write-Host "UI sources available under: $Root"
