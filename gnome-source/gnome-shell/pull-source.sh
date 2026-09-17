#!/usr/bin/env bash
set -euo pipefail
umask 022
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
DEST="$ROOT/gnome-source/gnome-shell"
URL="https://github.com/GNOME/gnome-shell.git"
REF="main"
DEPTH="${GNOME_CLONE_DEPTH:-20}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
command -v git >/dev/null || { echo 'ERROR: git is required' >&2; exit 1; }
git clone --depth "$DEPTH" --branch "$REF" --single-branch "$URL" "$TMP/gnome-shell"
git -C "$TMP/gnome-shell" fsck --no-progress
test -f "$TMP/gnome-shell/meson.build" || { echo 'ERROR: incomplete GNOME Shell source tree' >&2; exit 1; }
test -f "$TMP/gnome-shell/README.md" || { echo 'ERROR: missing GNOME Shell README' >&2; exit 1; }
rm -rf "$DEST/.git"
cp -a "$TMP/gnome-shell/." "$DEST/"
rm -rf "$DEST/.git"
printf 'Imported verified GNOME Shell source at clone depth %s into %s\n' "$DEPTH" "$DEST"
printf '%s\n' 'Compile/install safety: use a dedicated DESTDIR or prefix; do not run generated binaries as root; review install manifests.'
