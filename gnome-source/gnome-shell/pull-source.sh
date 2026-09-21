#!/usr/bin/env bash
set -euo pipefail
umask 022

# GNOME Shell source acquisition for MirvkBuntu.
# Canonical build input is gnome-source/gnome-shell/source/ (never the module root).
#
# Pinning: set GNOME_SHELL_REF (tag/branch/commit) to pin a version, e.g.
#   GNOME_SHELL_REF=<tag> bash pull-source.sh
# Default is the upstream default branch. GNOME_CLONE_DEPTH controls shallow
# depth; set GNOME_CLONE_DEPTH=0 for a full clone (required for some commit refs).

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
MODULE="gnome-shell"
DEST="$ROOT/gnome-source/$MODULE"
SOURCE="$DEST/source"
URL="${GNOME_SHELL_URL:-https://github.com/GNOME/gnome-shell.git}"
REF="${GNOME_SHELL_REF:-main}"
DEPTH="${GNOME_CLONE_DEPTH:-20}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

command -v git >/dev/null || { echo 'ERROR: git is required' >&2; exit 1; }

if [ "$DEPTH" = "0" ]; then
  git clone "$URL" "$TMP/$MODULE"
  git -C "$TMP/$MODULE" checkout --quiet "$REF"
else
  git clone --depth "$DEPTH" --branch "$REF" --single-branch "$URL" "$TMP/$MODULE" 2>/dev/null \
    || { git clone "$URL" "$TMP/$MODULE"; git -C "$TMP/$MODULE" checkout --quiet "$REF"; }
fi
git -C "$TMP/$MODULE" fsck --no-progress

test -f "$TMP/$MODULE/meson.build" || { echo 'ERROR: incomplete GNOME Shell source tree' >&2; exit 1; }

RESOLVED="$(git -C "$TMP/$MODULE" rev-parse HEAD)"
rm -rf "$TMP/$MODULE/.git"

rm -rf "$SOURCE.new"
mkdir -p "$SOURCE.new"
cp -a "$TMP/$MODULE/." "$SOURCE.new/"
rm -rf "$SOURCE"
mv "$SOURCE.new" "$SOURCE"

printf '%s %s\n' "$RESOLVED" "$REF" > "$DEST/$MODULE.source-ref"
printf 'Imported verified GNOME Shell source (ref %s -> %s) into %s\n' "$REF" "$RESOLVED" "$SOURCE"
printf '%s\n' 'Compile/install safety: use a dedicated DESTDIR or prefix; do not run generated binaries as root; review install manifests.'
