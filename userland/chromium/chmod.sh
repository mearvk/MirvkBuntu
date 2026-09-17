#!/usr/bin/env bash
# Mark repository shell scripts executable for local Ubuntu White Edition builds.
# This is intentionally idempotent and safe to run repeatedly.
set -euo pipefail

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT_DIR="${ROOT_DIR:-$SCRIPT_DIR/../..}"

printf 'Scanning for shell scripts under: %s\n' "$ROOT_DIR"

count=0
while IFS= read -r -d '' script; do
    chmod +x "$script"
    printf 'chmod +x %s\n' "${script#"$ROOT_DIR"/}"
    count=$((count + 1))
done < <(find "$ROOT_DIR" -type f -name '*.sh' -print0)

printf 'Executable shell scripts: %d\n' "$count"
