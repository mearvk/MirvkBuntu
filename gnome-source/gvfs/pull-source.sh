#!/usr/bin/env bash
set -euo pipefail
umask 022

# GVfs source acquisition for MirvkBuntu.
# Canonical build input is gnome-source/gvfs/source/.
#
# Two modes, in priority order:
#   1. Git (default here): clone GNOME's mirror into source/. Pin with GVFS_REF
#      (tag/branch/commit); default is the upstream default branch.
#   2. Release tarball: set GVFS_VERSION to fetch a pinned official release from
#      download.gnome.org (verified against its published sha256sum). Selected
#      automatically when GVFS_VERSION is set and GVFS_REF is not.
#
# GNOME_CLONE_DEPTH controls git shallow depth (0 = full clone, needed for some
# commit refs).

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
MODULE="gvfs"
SOURCE="$ROOT_DIR/source"
URL="${GVFS_URL:-https://github.com/GNOME/gvfs.git}"
REF="${GVFS_REF:-}"
VERSION="${GVFS_VERSION:-}"
DEPTH="${GNOME_CLONE_DEPTH:-20}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

command -v tar >/dev/null 2>&1 || { echo "ERROR: tar is required." >&2; exit 1; }

atomic_replace_source(){  # $1 = a directory whose contents become source/
  rm -rf "$SOURCE.new"
  mkdir -p "$SOURCE.new"
  cp -a "$1/." "$SOURCE.new/"
  rm -rf "$SOURCE"
  mv "$SOURCE.new" "$SOURCE"
}

# Mode 2: pinned release tarball (only when a version is set and no git ref).
if [ -n "$VERSION" ] && [ -z "$REF" ]; then
  command -v curl >/dev/null 2>&1 || { echo "ERROR: curl is required for tarball mode." >&2; exit 1; }
  command -v sha256sum >/dev/null 2>&1 || { echo "ERROR: sha256sum is required." >&2; exit 1; }
  ARCHIVE="gvfs-${VERSION}.tar.xz"
  SOURCE_URL="${GVFS_SOURCE_URL:-https://download.gnome.org/sources/gvfs/${VERSION%.*}/${ARCHIVE}}"
  CHECKSUM_URL="${GVFS_CHECKSUM_URL:-${SOURCE_URL}.sha256sum}"
  EXPECTED_SHA256="${GVFS_SHA256:-}"
  curl --fail --location --retry 3 --proto '=https' --tlsv1.2 --output "$TMP/$ARCHIVE" "$SOURCE_URL"
  if [ -z "$EXPECTED_SHA256" ] && curl --fail --location --retry 3 --proto '=https' --tlsv1.2 --output "$TMP/checksum.txt" "$CHECKSUM_URL"; then
    EXPECTED_SHA256="$(awk -v f="$ARCHIVE" '$0 ~ f {print $1; exit}' "$TMP/checksum.txt")"
  fi
  [[ "$EXPECTED_SHA256" =~ ^[0-9a-fA-F]{64}$ ]] || { echo "ERROR: no valid SHA-256 for $ARCHIVE." >&2; exit 1; }
  printf '%s  %s\n' "$EXPECTED_SHA256" "$TMP/$ARCHIVE" | sha256sum --check --strict
  tar -xJf "$TMP/$ARCHIVE" -C "$TMP"
  EXTRACTED="$TMP/gvfs-${VERSION}"
  test -f "$EXTRACTED/meson.build" || { echo "ERROR: incomplete GVfs source tree." >&2; exit 1; }
  atomic_replace_source "$EXTRACTED"
  printf '%s %s\n' "$EXPECTED_SHA256" "gvfs-${VERSION}.tar.xz" > "$ROOT_DIR/${MODULE}.source-ref"
  printf 'Imported GVfs %s (tarball) into %s\n' "$VERSION" "$SOURCE"
  exit 0
fi

# Mode 1: git clone into source/ (default).
command -v git >/dev/null 2>&1 || { echo "ERROR: git is required." >&2; exit 1; }
REF="${REF:-main}"
if [ "$DEPTH" = "0" ]; then
  git clone "$URL" "$TMP/$MODULE"
  git -C "$TMP/$MODULE" checkout --quiet "$REF"
else
  git clone --depth "$DEPTH" --branch "$REF" --single-branch "$URL" "$TMP/$MODULE" 2>/dev/null \
    || { git clone "$URL" "$TMP/$MODULE"; git -C "$TMP/$MODULE" checkout --quiet "$REF"; }
fi
git -C "$TMP/$MODULE" fsck --no-progress
test -f "$TMP/$MODULE/meson.build" || { echo "ERROR: incomplete GVfs source tree." >&2; exit 1; }
RESOLVED="$(git -C "$TMP/$MODULE" rev-parse HEAD)"
rm -rf "$TMP/$MODULE/.git"
atomic_replace_source "$TMP/$MODULE"
printf '%s %s\n' "$RESOLVED" "$REF" > "$ROOT_DIR/${MODULE}.source-ref"
printf 'Imported verified GVfs source (ref %s -> %s) into %s\n' "$REF" "$RESOLVED" "$SOURCE"
