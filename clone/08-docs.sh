#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$BASH_SOURCE")" && pwd)"
source "$SCRIPT_DIR/common-download.sh"
# MirvkBuntu currently uses docs/ as the tracked documentation root.
clone_tree "docs" "$SCRIPT_DIR/../docs"
