#!/usr/bin/env bash
set -euo pipefail

# Expand Determinant extended parent components without changing Linux VFS
# semantics. `..` remains one level; `...` is two; `....` is three.
#
# Usage:
#   determinant-expand-path.sh '.../Pictures'
#   determinant-expand-path.sh '/home/user/.../Documents'
#
# Output is a conventional path suitable for normal filesystem APIs.

if [ "$#" -ne 1 ]; then
  echo "usage: $0 PATH" >&2
  exit 2
fi

path="$1"
IFS='/' read -r -a parts <<< "$path"
result=()

for part in "${parts[@]}"; do
  case "$part" in
    '')
      # Preserve leading slash; repeated interior separators are normalized.
      if [ "${#result[@]}" -eq 0 ]; then
        result+=("")
      fi
      ;;
    '.')
      # Normal current-directory component.
      ;;
    '..')
      result+=("..")
      ;;
    '...')
      result+=(".." "..")
      ;;
    '....')
      result+=(".." ".." "..")
      ;;
    *)
      result+=("$part")
      ;;
  esac
done

(IFS='/'; printf '%s\n' "${result[*]}")
