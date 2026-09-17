#!/usr/bin/env bash
set -euo pipefail

# Ubuntu Determinant — glib-networking source acquisition.
# The canonical final source boundary is gnome-source/glib-networking/source/.

VERSION="${GLIB_NETWORKING_VERSION:-2.80.1}"
ARCHIVE="glib-networking-${VERSION}.tar.xz"
SOURCE_BASE="${GLIB_NETWORKING_SOURCE_BASE:-https://download.gnome.org/sources/glib-networking}"
SOURCE_URL="${GLIB_NETWORKING_SOURCE_URL:-${SOURCE_BASE}/${VERSION%.*}/${ARCHIVE}}"
CHECKSUM_URL="${GLIB_NETWORKING_CHECKSUM_URL:-${SOURCE_BASE}/${VERSION%.*}/glib-networking-${VERSION}.sha256sum}"
SHA256="${GLIB_NETWORKING_SHA256:-}"

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
SOURCE_DIR="${ROOT_DIR}/source"
TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

command -v curl >/dev/null 2>&1 || { echo "ERROR: curl is required." >&2; exit 1; }
command -v sha256sum >/dev/null 2>&1 || { echo "ERROR: sha256sum is required." >&2; exit 1; }
command -v tar >/dev/null 2>&1 || { echo "ERROR: tar is required." >&2; exit 1; }

printf '=== Downloading glib-networking %s ===\n' "$VERSION"
printf 'Source: %s\n' "$SOURCE_URL"
curl --fail --location --retry 3 --proto '=https' --tlsv1.2 --output "$TMP_DIR/$ARCHIVE" "$SOURCE_URL"

if [ -z "$SHA256" ]; then
  curl --fail --location --retry 3 --proto '=https' --tlsv1.2 --output "$TMP_DIR/checksum.txt" "$CHECKSUM_URL"
  SHA256="$(awk -v file="$ARCHIVE" '$2 == file || $2 == "*" file {print $1; exit}' "$TMP_DIR/checksum.txt")"
fi

if ! [[ "$SHA256" =~ ^[0-9a-fA-F]{64}$ ]]; then
  echo "ERROR: Could not obtain a valid SHA-256 for $ARCHIVE." >&2
  exit 1
fi

printf '%s  %s\n' "$SHA256" "$TMP_DIR/$ARCHIVE" | sha256sum --check --strict
ACTUAL_SHA256="$(sha256sum "$TMP_DIR/$ARCHIVE" | awk '{print $1}')"

rm -rf "$TMP_DIR/source"
mkdir -p "$TMP_DIR/source"
tar -xJf "$TMP_DIR/$ARCHIVE" -C "$TMP_DIR/source"
EXTRACTED="$TMP_DIR/source/glib-networking-${VERSION}"
test -d "$EXTRACTED" || { echo "ERROR: expected extracted source directory not found." >&2; exit 1; }
test -f "$EXTRACTED/meson.build" || { echo "ERROR: incomplete glib-networking source tree." >&2; exit 1; }

rm -rf "$SOURCE_DIR"
mkdir -p "$SOURCE_DIR"
cp -a "$EXTRACTED/." "$SOURCE_DIR/"
printf '%s\n' "$ACTUAL_SHA256" > "$ROOT_DIR/glib-networking-${VERSION}.sha256"
cat > "$ROOT_DIR/SOURCE-INFO.txt" <<EOF
Project: glib-networking
Version: ${VERSION}
Source URL: ${SOURCE_URL}
SHA256: ${ACTUAL_SHA256}
Canonical source directory: source/
Determinant modifications belong outside source/ as patches/offsets.
EOF

printf '=== glib-networking source installed in %s ===\n' "$SOURCE_DIR"
