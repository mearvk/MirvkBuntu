#!/usr/bin/env bash
set -euo pipefail
umask 022
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
DEST="$ROOT/gnome-source/gtk"
URL="https://github.com/GNOME/gtk.git"
REF="main"
DEPTH="${GNOME_CLONE_DEPTH:-20}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
command -v git >/dev/null || { echo 'ERROR: git is required' >&2; exit 1; }
git clone --depth "$DEPTH" --branch "$REF" --single-branch "$URL" "$TMP/gtk"
git -C "$TMP/gtk" fsck --no-progress
test -f "$TMP/gtk/meson.build" || { echo 'ERROR: incomplete GTK source tree' >&2; exit 1; }
test -f "$TMP/gtk/README.md" || { echo 'ERROR: missing GTK README' >&2; exit 1; }
rm -rf "$DEST/.git"
cp -a "$TMP/gtk/." "$DEST/"
rm -rf "$DEST/.git"
printf 'Imported verified GTK source at clone depth %s into %s\n' "$DEPTH" "$DEST"
printf '%s\n' 'Compile/install safety: use a dedicated DESTDIR or prefix; do not run generated binaries as root; review install manifests.'
