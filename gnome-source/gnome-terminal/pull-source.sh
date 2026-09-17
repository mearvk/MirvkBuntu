#!/usr/bin/env bash
set -euo pipefail

# Ubuntu Determinant — GNOME Terminal source acquisition.
# Final canonical source location: gnome-source/gnome-terminal/source/

VERSION="${GNOME_TERMINAL_VERSION:-3.60.0}"
ARCHIVE="gnome-terminal-${VERSION}.tar.xz"
SOURCE_URL="${GNOME_TERMINAL_SOURCE_URL:-https://download.gnome.org/sources/gnome-terminal/${VERSION%.*}/${ARCHIVE}}"
CHECKSUM_URL="${GNOME_TERMINAL_CHECKSUM_URL:-${SOURCE_URL}.sha256sum}"
EXPECTED_SHA256="${GNOME_TERMINAL_SHA256:-}"
ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
SOURCE_DIR="$ROOT_DIR/source"
TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

command -v curl >/dev/null 2>&1 || { echo "ERROR: curl is required." >&2; exit 1; }
command -v sha256sum >/dev/null 2>&1 || { echo "ERROR: sha256sum is required." >&2; exit 1; }
command -v tar >/dev/null 2>&1 || { echo "ERROR: tar is required." >&2; exit 1; }

curl --fail --location --retry 3 --proto '=https' --tlsv1.2 --output "$TMP_DIR/$ARCHIVE" "$SOURCE_URL"
if [ -z "$EXPECTED_SHA256" ]; then
  curl --fail --location --retry 3 --proto '=https' --tlsv1.2 --output "$TMP_DIR/checksum.txt" "$CHECKSUM_URL"
  EXPECTED_SHA256="$(awk -v archive="$ARCHIVE" '$0 ~ archive {print $1; exit}' "$TMP_DIR/checksum.txt")"
fi
[[ "$EXPECTED_SHA256" =~ ^[0-9a-fA-F]{64}$ ]] || { echo "ERROR: no valid SHA-256 checksum was obtained." >&2; exit 1; }
printf '%s  %s\n' "$EXPECTED_SHA256" "$TMP_DIR/$ARCHIVE" | sha256sum --check --strict

tar -xJf "$TMP_DIR/$ARCHIVE" -C "$TMP_DIR"
EXTRACTED="$TMP_DIR/gnome-terminal-${VERSION}"
test -f "$EXTRACTED/meson.build" || { echo "ERROR: incomplete GNOME Terminal source tree." >&2; exit 1; }
rm -rf "$SOURCE_DIR"
mkdir -p "$SOURCE_DIR"
cp -a "$EXTRACTED/." "$SOURCE_DIR/"
ACTUAL_SHA256="$(sha256sum "$TMP_DIR/$ARCHIVE" | awk '{print $1}')"
printf '%s  %s\n' "$ACTUAL_SHA256" "$ARCHIVE" > "$ROOT_DIR/gnome-terminal-${VERSION}.sha256"
cat > "$ROOT_DIR/SOURCE-INFO.txt" <<EOF
Project: gnome-terminal
Version: ${VERSION}
Source URL: ${SOURCE_URL}
SHA256: ${ACTUAL_SHA256}
Canonical source directory: source/
Determinant modifications belong outside source/ as patches/offsets.
EOF
printf 'Installed GNOME Terminal %s into %s\n' "$VERSION" "$SOURCE_DIR"
