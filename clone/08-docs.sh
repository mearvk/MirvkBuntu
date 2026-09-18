#!/usr/bin/env bash
set -u
SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common-download.sh"
for path in markdown wiki; do clone_tree "$path" "$SCRIPT_DIR/../docs/$path" || exit 1; done
