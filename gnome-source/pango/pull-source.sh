#!/usr/bin/env bash
set -euo pipefail
umask 022
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
DEST="$ROOT/gnome-source/pango"
URL="https://github.com/GNOME/pango.git"
REF="main"
DEPTH="${GNOME_CLONE_DEPTH:-20}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

command -v git >/dev/null || { echo 'ERROR: git is required' >&2; exit 1; }
git clone --depth "$DEPTH" --branch "$REF" --single-branch "$URL" "$TMP/pango"
git -C "$TMP/pango" fsck --no-progress

test -f "$TMP/pango/meson.build" || { echo 'ERROR: incomplete Pango source tree' >&2; exit 1; }
test -f "$TMP/pango/README.md" || { echo 'ERROR: missing Pango README' >&2; exit 1; }

rm -rf "$DEST/.git"
cp -a "$TMP/pango/." "$DEST/"
rm -rf "$DEST/.git"
printf 'Imported verified Pango source at clone depth %s into %s\n' "$DEPTH" "$DEST"
printf '%s\n' 'Compile/install safety: use a dedicated DESTDIR or prefix under the OS staging tree; do not run generated binaries as root; review install manifests before installation.'
