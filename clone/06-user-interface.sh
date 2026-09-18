#!/usr/bin/env bash
set -u
SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common-download.sh"
clone_tree "user-interface" "$SCRIPT_DIR/../user-interface"
