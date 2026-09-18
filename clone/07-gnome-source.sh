#!/usr/bin/env bash
set -u
SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common-download.sh"
clone_tree "gnome-source" "$SCRIPT_DIR/../gnome-source"
