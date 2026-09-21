#!/usr/bin/env bash
set -euo pipefail

# Chromium native build wrapper (MirvkBuntu).
#
# This is a thin wrapper that delegates to the authoritative release build at
# build/chromium/build-chromium.sh. The MirvkBuntu native gate
# (build/native-build.sh -> build_other_native_projects) invokes this script
# with DESTDIR / JOBS / MIRVKBUNTU_RELEASE set, and expects a release build
# staged into DESTDIR. Keeping the logic in one place avoids a second,
# conflicting (debug) build pass.
#
# Legacy "White Edition" developer build:
#   The previous debug build wrapper is still available for interactive
#   development via:  ./build.sh white-edition [check|gen|build|test]
#
# Usage:
#   ./build.sh                       # release build (honors DESTDIR/JOBS)
#   DESTDIR=/rootfs JOBS=8 ./build.sh
#   ./build.sh white-edition build   # legacy debug developer build

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(CDPATH= cd -- "${SCRIPT_DIR}/../.." && pwd)"
AUTHORITATIVE="${REPO_ROOT}/build/chromium/build-chromium.sh"

if [ "${1:-}" = "white-edition" ]; then
  shift
  exec "${SCRIPT_DIR}/http-3.0/build-white-edition.sh" "$@"
fi

[ -x "$AUTHORITATIVE" ] || { printf 'chromium build.sh: ERROR: missing %s\n' "$AUTHORITATIVE" >&2; exit 1; }

# Delegate. DESTDIR/JOBS/PREFIX/etc. pass through the environment unchanged.
exec "$AUTHORITATIVE" "$@"
