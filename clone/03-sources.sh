#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$BASH_SOURCE")" && pwd)"
source "$SCRIPT_DIR/common-download.sh"
for path in main modules libraries scripts; do
  clone_tree "$path" "$SCRIPT_DIR/../sources/$path"
done
