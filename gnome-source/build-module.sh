#!/usr/bin/env bash
set -euo pipefail

# Common local builder for GNOME modules.
# Canonical source is <module>/source; all generated output is outside source/.
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
DESTDIR="${DESTDIR:-}"
PREFIX="${PREFIX:-/usr}"
mkdir -p "$BUILD"

if [ -e "$SRC/meson.build" ]; then
  command -v meson >/dev/null || { echo "ERROR: meson is required" >&2; exit 1; }
  rm -rf "$BUILD"
  meson setup "$BUILD" "$SRC" --buildtype=release --prefix="$PREFIX" "$@"
  meson compile -C "$BUILD"
  if [ "${INSTALL:-0}" = "1" ]; then
    if [ -n "$DESTDIR" ]; then mkdir -p "$DESTDIR"; DESTDIR="$DESTDIR" meson install -C "$BUILD"; else meson install -C "$BUILD"; fi
  fi
elif [ -e "$SRC/configure.ac" ] || [ -x "$SRC/configure" ]; then
  [ -x "$SRC/configure" ] || { echo "ERROR: configure.ac exists but configure is missing; bootstrap source first" >&2; exit 1; }
  rm -rf "$BUILD"
  mkdir -p "$BUILD"
  cd "$BUILD"
  "$SRC/configure" --prefix="$PREFIX" "$@"
  make -j"${JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)}"
  if [ "${INSTALL:-0}" = "1" ]; then
    if [ -n "$DESTDIR" ]; then make DESTDIR="$DESTDIR" install; else make install; fi
  fi
elif [ -e "$SRC/CMakeLists.txt" ]; then
  command -v cmake >/dev/null || { echo "ERROR: cmake is required" >&2; exit 1; }
  rm -rf "$BUILD"
  cmake -S "$SRC" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PREFIX" "$@"
  cmake --build "$BUILD" --parallel "${JOBS:-}"
  if [ "${INSTALL:-0}" = "1" ]; then
    if [ -n "$DESTDIR" ]; then cmake --install "$BUILD" --prefix "$PREFIX" --strip --component Unspecified -- DESTDIR="$DESTDIR"; else cmake --install "$BUILD"; fi
  fi
else
  echo "ERROR: recognized Python project requires module-specific packaging/build handling" >&2
  exit 1
fi

echo "Build completed for $MODULE."
