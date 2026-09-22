#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$REPO_ROOT/build/scout}"
command -v cmake >/dev/null 2>&1 || { echo "scout: cmake is required" >&2; exit 1; }
cmake -S "$REPO_ROOT/scout" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" --config Release --parallel "${JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)}"
echo "scout: executable is in $BUILD_DIR"
