#!/usr/bin/env bash
set -euo pipefail
umask 022
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
DEST="$ROOT/gnome-source/glib"
URL="https://github.com/GNOME/glib.git"
REF="main"
DEPTH="${GNOME_CLONE_DEPTH:-20}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

# Safety: clone into an isolated temporary directory; never execute upstream files.
# Refuse an incomplete/invalid clone before changing the vendored destination.
# The source tree is copied only after Git verifies the requested checkout.
command -v git >/dev/null || { echo 'ERROR: git is required' >&2; exit 1; }
git clone --depth "$DEPTH" --branch "$REF" --single-branch "$URL" "$TMP/glib"
git -C "$TMP/glib" fsck --no-progress

test -f "$TMP/glib/meson.build" || { echo 'ERROR: incomplete GLib source tree' >&2; exit 1; }
test -f "$TMP/glib/README.md" || { echo 'ERROR: missing GLib README' >&2; exit 1; }

# Never copy a .git directory or replace arbitrary absolute paths.
rm -rf "$DEST/.git"
cp -a "$TMP/glib/." "$DEST/"
rm -rf "$DEST/.git"
printf 'Imported verified GLib source at clone depth %s into %s\n' "$DEPTH" "$DEST"
printf '%s\n' 'Compile/install safety: use a dedicated DESTDIR or prefix under the OS staging tree; do not run generated binaries as root; review install manifests before installation.'
