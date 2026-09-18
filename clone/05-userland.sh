#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$BASH_SOURCE")" && pwd)"

# Userland is sourced from the maintained Ubuntu.Determinant.Beta.Restricted
# reference repository rather than MirvkBuntu itself.
export SOURCE_REPO="${SOURCE_REPO:-mearvk/Ubuntu.Determinant.Beta.Restricted}"
export SOURCE_REF="${SOURCE_REF:-main}"

source "$SCRIPT_DIR/common-download.sh"
clone_tree "userland" "$SCRIPT_DIR/../userland"
