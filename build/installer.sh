#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$REPO_ROOT/build/installer}"

command -v cmake >/dev/null 2>&1 || { echo "installer: cmake is required" >&2; exit 1; }
command -v cpack >/dev/null 2>&1 || { echo "installer: cpack is required" >&2; exit 1; }

cmake -S "$REPO_ROOT/installer" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" --config Release --parallel "${JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)}"
cpack --config "$BUILD_DIR/CPackConfig.cmake" -C Release

echo "installer: packages are in $BUILD_DIR"
