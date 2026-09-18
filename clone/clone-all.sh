#!/usr/bin/env bash
set -u
SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
# The MirvkBuntu repository is the authoritative source tree used by the
# native build. Override SOURCE_REPO explicitly only when bootstrapping from
# another repository is intentional.
export SOURCE_REPO="${SOURCE_REPO:-mearvk/MirvkBuntu}"
export SOURCE_REF="${SOURCE_REF:-main}"
status=0
for script in 01-kernels.sh 02-file-systems.sh 03-sources.sh 04-packages.sh 05-userland.sh 06-user-interface.sh 07-gnome-source.sh 08-docs.sh; do
  printf '\n==> %s\n' "$script"
  bash "$SCRIPT_DIR/$script" || status=1
done
exit "$status"
