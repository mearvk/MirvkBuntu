#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$BASH_SOURCE")" && pwd)"

# Userland is sourced from the maintained Ubuntu.Determinant.Beta.Restricted
# reference repository rather than MirvkBuntu itself.
#
# This stage deliberately OVERRIDES any inherited SOURCE_REPO. clone-all.sh
# exports SOURCE_REPO=mearvk/MirvkBuntu for every stage, so a "${SOURCE_REPO:-...}"
# default would never take effect here and userland would be pulled from the
# wrong repository. Allow an explicit override only via USERLAND_SOURCE_REPO.
export SOURCE_REPO="${USERLAND_SOURCE_REPO:-mearvk/Ubuntu.Determinant.Beta.Restricted}"
export SOURCE_REF="${USERLAND_SOURCE_REF:-main}"

source "$SCRIPT_DIR/common-download.sh"
clone_tree "userland" "$SCRIPT_DIR/../userland"
