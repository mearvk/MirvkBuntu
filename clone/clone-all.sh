#!/usr/bin/env bash
set -u

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$BASH_SOURCE")" && pwd)"
export SOURCE_REPO="${SOURCE_REPO:-mearvk/MirvkBuntu}"
export SOURCE_REF="${SOURCE_REF:-main}"

failed=0

for script in 01-kernels.sh 02-file-systems.sh 03-sources.sh 04-packages.sh 05-userland.sh 06-user-interface.sh 07-gnome-source.sh 08-docs.sh; do
  printf '\n==> %s\n' "$script"

  if bash "$SCRIPT_DIR/$script"; then
    printf '==> %s: completed\n' "$script"
  else
    status=$?
    printf '==> %s: FAILED (exit %s); continuing to next stage\n' "$script" "$status" >&2
    failed=1
  fi
done

if (( failed != 0 )); then
  printf '\nclone: one or more stages failed; all eight stages were attempted.\n' >&2
  exit 1
fi

printf '\nclone: all eight stages completed successfully.\n'
exit 0
