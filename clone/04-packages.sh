#!/usr/bin/env bash
set -u
SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common-download.sh"
for path in aptitude installer securejdk-installer; do clone_tree "$path" "$SCRIPT_DIR/../packages/$path" || exit 1; done
