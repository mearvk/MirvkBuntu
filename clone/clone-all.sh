#!/usr/bin/env bash
set -u
SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
export SOURCE_REPO="${SOURCE_REPO:-mearvk/Ubuntu.Determinant.Beta.Restricted}"
export SOURCE_REF="${SOURCE_REF:-main}"
status=0
for script in 01-kernels.sh 02-file-systems.sh 03-sources.sh 04-packages.sh 05-userland.sh 06-user-interface.sh 07-gnome-source.sh 08-docs.sh; do
  printf '\n==> %s\n' "$script"
  bash "$SCRIPT_DIR/$script" || status=1
done
exit "$status"
