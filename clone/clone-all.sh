#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$BASH_SOURCE")" && pwd)"
export SOURCE_REPO="${SOURCE_REPO:-mearvk/MirvkBuntu}"
export SOURCE_REF="${SOURCE_REF:-main}"
for script in 01-kernels.sh 02-file-systems.sh 03-sources.sh 04-packages.sh 05-userland.sh 06-user-interface.sh 07-gnome-source.sh 08-docs.sh; do
  printf '\n==> %s\n' "$script"
  bash "$SCRIPT_DIR/$script"
done
