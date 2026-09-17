#!/usr/bin/env bash
# Chromium White Edition top-level build entry point.
set -euo pipefail

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
DEPOT_TOOLS=${DEPOT_TOOLS:-"$SCRIPT_DIR/depot_tools"}
export DEPOT_TOOLS
export PATH="$DEPOT_TOOLS:$PATH"

# The depot_tools checkout may contain the GN resolver before its managed
# bootstrap has completed. Initialize it before invoking the build wrapper.
if [ ! -f "$DEPOT_TOOLS/python3_bin_reldir.txt" ]; then
    [ -d "$DEPOT_TOOLS/.git" ] || "$SCRIPT_DIR/install-gn.sh"
    [ -x "$DEPOT_TOOLS/gclient" ] || "$SCRIPT_DIR/install-gn.sh"
    (cd "$DEPOT_TOOLS" && ./gclient --version >/dev/null 2>&1 || true)
fi

# Refresh depot_tools when its updater is available. Do not require an exact
# number of GN entries: standalone GN installations are valid too.
if [ -x "$DEPOT_TOOLS/update_depot_tools" ]; then
    (cd "$DEPOT_TOOLS" && ./update_depot_tools) || true
fi

# Resolve GN after bootstrap. Running gn --version is the authoritative test.
if ! command -v gn >/dev/null 2>&1 || ! gn --version >/dev/null 2>&1; then
    "$SCRIPT_DIR/install-gn.sh"
    export PATH="$DEPOT_TOOLS:$PATH"
fi

exec "$SCRIPT_DIR/http-3.0/build-white-edition.sh" "$@"
