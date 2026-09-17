#!/usr/bin/env bash
set -euo pipefail
umask 022
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
DEST="$ROOT/gnome-source/gdk-pixbuf"
URL="https://github.com/GNOME/gdk-pixbuf.git"
REF="main"
DEPTH="${GNOME_CLONE_DEPTH:-20}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
command -v git >/dev/null || { echo 'ERROR: git is required' >&2; exit 1; }
git clone --depth "$DEPTH" --branch "$REF" --single-branch "$URL" "$TMP/gdk-pixbuf"
git -C "$TMP/gdk-pixbuf" fsck --no-progress
test -f "$TMP/gdk-pixbuf/meson.build" || { echo 'ERROR: incomplete GDK-Pixbuf source tree' >&2; exit 1; }
test -f "$TMP/gdk-pixbuf/README.md" || { echo 'ERROR: missing GDK-Pixbuf README' >&2; exit 1; }
rm -rf "$DEST/.git"
cp -a "$TMP/gdk-pixbuf/." "$DEST/"
rm -rf "$DEST/.git"
printf 'Imported verified GDK-Pixbuf source at clone depth %s into %s\n' "$DEPTH" "$DEST"
printf '%s\n' 'Compile/install safety: use a dedicated DESTDIR or prefix; do not run generated binaries as root; review install manifests.'
