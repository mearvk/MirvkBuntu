#!/usr/bin/env bash
# Bootstrap Chromium depot_tools and verify that its GN resolver can run.
set -euo pipefail

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
DEPOT_TOOLS=${DEPOT_TOOLS:-"$SCRIPT_DIR/depot_tools"}

fail() { printf 'ERROR: %s\n' "$*" >&2; exit 1; }
command -v git >/dev/null 2>&1 || fail "git is required"

if [ ! -d "$DEPOT_TOOLS/.git" ]; then
    [ ! -e "$DEPOT_TOOLS" ] || fail "DEPOT_TOOLS exists but is not a git checkout: $DEPOT_TOOLS"
    git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git "$DEPOT_TOOLS"
fi

export PATH="$DEPOT_TOOLS:$PATH"

# The depot_tools gn launcher resolves a managed GN binary. A duplicated
# depot_tools PATH entry is not a second GN installation.
if [ -x "$DEPOT_TOOLS/update_depot_tools" ]; then
    (cd "$DEPOT_TOOLS" && ./update_depot_tools)
fi
if [ -x "$DEPOT_TOOLS/ensure_bootstrap" ]; then
    (cd "$DEPOT_TOOLS" && ./ensure_bootstrap)
fi

command -v gclient >/dev/null 2>&1 || fail "gclient is unavailable from depot_tools"
command -v autoninja >/dev/null 2>&1 || fail "autoninja is unavailable from depot_tools"
command -v gn >/dev/null 2>&1 || fail "GN resolver is unavailable from depot_tools"

GN_BIN=$(command -v gn)
if ! GN_VERSION=$(gn --version 2>&1); then
    printf 'GN resolver: %s\n' "$GN_BIN" >&2
    printf 'GN invocation failed:\n%s\n' "$GN_VERSION" >&2
    printf '%s\n' "Manual fallback: bootstrap/update depot_tools and inspect its managed tool directory." >&2
    exit 1
fi

printf 'depot_tools: %s\n' "$DEPOT_TOOLS"
printf 'GN resolver: %s\n' "$GN_BIN"
printf 'GN version: %s\n' "$GN_VERSION"
printf 'autoninja: %s\n' "$(command -v autoninja)"
