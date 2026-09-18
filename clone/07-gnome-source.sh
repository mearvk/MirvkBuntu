#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$BASH_SOURCE")" && pwd)"
source "$SCRIPT_DIR/common-download.sh"
clone_tree "gnome-source" "$SCRIPT_DIR/../gnome-source"
