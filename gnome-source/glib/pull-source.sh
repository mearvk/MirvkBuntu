#!/usr/bin/env bash
set -euo pipefail
umask 022

# GLib source acquisition for MirvkBuntu.
# Canonical build input is gnome-source/glib/source/ (never the module root).
#
# Pinning: set GLIB_REF (tag/branch/commit) to pin a version, e.g.
#   GLIB_REF=2.80.4 bash pull-source.sh
# Default is the upstream default branch. GNOME_CLONE_DEPTH controls shallow
# depth; set GNOME_CLONE_DEPTH=0 for a full clone (required for some commit refs).

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
MODULE="glib"
DEST="$ROOT/gnome-source/$MODULE"
SOURCE="$DEST/source"
URL="${GLIB_URL:-https://github.com/GNOME/glib.git}"
REF="${GLIB_REF:-main}"
DEPTH="${GNOME_CLONE_DEPTH:-20}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

command -v git >/dev/null || { echo 'ERROR: git is required' >&2; exit 1; }

# Clone into an isolated temp dir. A pinned commit SHA needs a full clone.
if [ "$DEPTH" = "0" ]; then
  git clone "$URL" "$TMP/$MODULE"
  git -C "$TMP/$MODULE" checkout --quiet "$REF"
else
  git clone --depth "$DEPTH" --branch "$REF" --single-branch "$URL" "$TMP/$MODULE" 2>/dev/null \
    || { git clone "$URL" "$TMP/$MODULE"; git -C "$TMP/$MODULE" checkout --quiet "$REF"; }
fi
git -C "$TMP/$MODULE" fsck --no-progress

test -f "$TMP/$MODULE/meson.build" || { echo 'ERROR: incomplete GLib source tree' >&2; exit 1; }
test -f "$TMP/$MODULE/README.md" || { echo 'ERROR: missing GLib README' >&2; exit 1; }

# Record the exact ref that was vendored.
RESOLVED="$(git -C "$TMP/$MODULE" rev-parse HEAD)"
# Never vendor upstream .git metadata.
rm -rf "$TMP/$MODULE/.git"

# Atomically replace source/ so re-runs are clean and cannot hit "cannot
# overwrite non-directory": build the new tree beside the old, then swap.
rm -rf "$SOURCE.new"
mkdir -p "$SOURCE.new"
cp -a "$TMP/$MODULE/." "$SOURCE.new/"
rm -rf "$SOURCE"
mv "$SOURCE.new" "$SOURCE"

printf '%s %s\n' "$RESOLVED" "$REF" > "$DEST/$MODULE.source-ref"
printf 'Imported verified GLib source (ref %s -> %s) into %s\n' "$REF" "$RESOLVED" "$SOURCE"
printf '%s\n' 'Compile/install safety: use a dedicated DESTDIR or prefix under the OS staging tree; do not run generated binaries as root; review install manifests before installation.'
