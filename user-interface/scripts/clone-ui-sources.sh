#!/usr/bin/env bash
set -euo pipefail

ROOT="${1:-user-interface/sources/vendor}"
mkdir -p "$ROOT"

command -v git >/dev/null 2>&1 || { echo "git is required" >&2; exit 1; }

clone_or_update() {
  local name="$1" url="$2"
  local dest="$ROOT/$name"
  if [ -d "$dest/.git" ]; then
    git -C "$dest" fetch --tags --prune
    git -C "$dest" pull --ff-only
  else
    git clone --depth 1 "$url" "$dest"
  fi
done
}

clone_or_update cockpit https://github.com/cockpit-project/cockpit.git
clone_or_update openbao https://github.com/openbao/openbao.git
clone_or_update gitlab-foss https://gitlab.com/gitlab-org/gitlab-foss.git

echo "UI sources available under: $ROOT"
