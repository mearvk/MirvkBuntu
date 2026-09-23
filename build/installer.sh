#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${MIRVKBUNTU_INSTALLER_BUILD_ROOT:-$REPO_ROOT/build/installer}" }
INSTALLER_OUT="${MIRVKBUNTU_INSTALLER_OUTPUT_ROOT:-$REPO_ROOT/build/output/installer}"

command -v cmake >/dev/null 2>&1 || { echo "installer: cmake is required" >&2; exit 1; }
command -v cpack >/dev/null 2>&1 || { echo "installer: cpack is required" >&2; exit 1; }

cmake -S "$REPO_ROOT/installer" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" --config Release --parallel "${JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)}"
rm -rf "$INSTALLER_OUT"
mkdir -p "$INSTALLER_OUT/packages"

if [ -f "$BUILD_DIR/mirvkbuntu-installer" ]; then
  cp -f "$BUILD_DIR/mirvkbuntu-installer" "$INSTALLER_OUT/mirvkbuntu-installer"
elif [ -f "$BUILD_DIR/Release/mirvkbuntu-installer.exe" ]; then
  cp -f "$BUILD_DIR/Release/mirvkbuntu-installer.exe" "$INSTALLER_OUT/mirvkbuntu-installer.exe"
else
  echo "installer: built executable was not found in $BUILD_DIR" >&2
  exit 1
fi

cpack --config "$BUILD_DIR/CPackConfig.cmake" -C Release
shopt -s nullglob
for pkg in "$BUILD_DIR"/MirvkBuntu-Installer-*; do
  cp -f "$pkg" "$INSTALLER_OUT/packages/"
done
shopt -u nullglob

echo "installer: binary output is $INSTALLER_OUT"
echo "installer: package outputs are $INSTALLER_OUT/packages"
