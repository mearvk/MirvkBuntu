#!/usr/bin/env bash
set -u
SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common-download.sh"
for path in main modules libraries scripts; do clone_tree "$path" "$SCRIPT_DIR/../sources/$path" || exit 1; done
