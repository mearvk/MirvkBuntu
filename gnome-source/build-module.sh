#!/usr/bin/env bash
set -euo pipefail

# Common local builder for GNOME modules. It detects the upstream build target
# instead of assuming a layout, and fails on ambiguous or unusual structures.
# Usage: ./build-module.sh <module> [extra build arguments...]
ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
MODULE="${1:-}"
shift || true
[ -n "$MODULE" ] || { echo "Usage: $0 <module> [args...]" >&2; exit 2; }
SRC="$ROOT_DIR/$MODULE/source"
[ -d "$SRC" ] || { echo "ERROR: missing source/: $SRC" >&2; exit 1; }

found=()
for f in meson.build configure.ac configure CMakeLists.txt setup.py pyproject.toml; do
  [ -e "$SRC/$f" ] && found+=("$f")
done
[ "${#found[@]}" -gt 0 ] || { echo "ERROR: no recognized build target in $SRC" >&2; exit 1; }
[ "${#found[@]}" -lt 2 ] || { echo "ERROR: unusual/ambiguous build structure in $SRC: ${found[*]}" >&2; exit 1; }

BUILD="$ROOT_DIR/$MODULE/build-local"
mkdir -p "$BUILD"
if [ -e "$SRC/meson.build" ]; then
  command -v meson >/dev/null || { echo "ERROR: meson is required" >&2; exit 1; }
  meson setup "$BUILD" "$SRC" --buildtype=release --prefix=/usr "$@"
  meson compile -C "$BUILD"
elif [ -e "$SRC/configure.ac" ] || [ -x "$SRC/configure" ]; then
  [ -x "$SRC/configure" ] || { echo "ERROR: configure.ac exists but configure is missing; bootstrap source first" >&2; exit 1; }
  "$SRC/configure" --prefix=/usr "$@"
  make -j"$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)"
elif [ -e "$SRC/CMakeLists.txt" ]; then
  command -v cmake >/dev/null || { echo "ERROR: cmake is required" >&2; exit 1; }
  cmake -S "$SRC" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr "$@"
  cmake --build "$BUILD" --parallel
else
  echo "ERROR: recognized Python project requires module-specific packaging/build handling" >&2
  exit 1
fi

echo "Build completed for $MODULE."
