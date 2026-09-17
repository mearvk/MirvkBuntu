#!/usr/bin/env bash
set -euo pipefail

# Ubuntu Determinant — Orca source acquisition
# Downloads a pinned official GNOME release archive without Git credentials.

VERSION="${ORCA_VERSION:-51.0}"
SERIES="${VERSION%%.*}"
ARCHIVE="orca-${VERSION}.tar.xz"
BASE_URL="${ORCA_SOURCE_BASE_URL:-https://download.gnome.org/sources/orca/${SERIES}}"
SOURCE_URL="${ORCA_SOURCE_URL:-${BASE_URL}/${ARCHIVE}}"
CHECKSUM_URL="${ORCA_CHECKSUM_URL:-${BASE_URL}/orca-${VERSION}.sha256sum}"

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
UPSTREAM_DIR="${ROOT_DIR}/upstream"
TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

command -v curl >/dev/null 2>&1 || { echo "ERROR: curl is required." >&2; exit 1; }
command -v sha256sum >/dev/null 2>&1 || { echo "ERROR: sha256sum is required." >&2; exit 1; }
command -v tar >/dev/null 2>&1 || { echo "ERROR: tar is required." >&2; exit 1; }

mkdir -p "$UPSTREAM_DIR"

printf '=== Downloading Orca %s ===\n' "$VERSION"
printf 'Source: %s\n' "$SOURCE_URL"

curl --fail --location --retry 3 --proto '=https' --tlsv1.2 \
  --output "$TMP_DIR/$ARCHIVE" "$SOURCE_URL"

printf 'Checksum: %s\n' "$CHECKSUM_URL"
curl --fail --location --retry 3 --proto '=https' --tlsv1.2 \
  --output "$TMP_DIR/orca.sha256sum" "$CHECKSUM_URL"

EXPECTED_LINE="$(grep -E "(^|[[:space:]])${ARCHIVE}([[:space:]]|$)" "$TMP_DIR/orca.sha256sum" | head -n 1 || true)"
if [ -z "$EXPECTED_LINE" ]; then
  echo "ERROR: published checksum for $ARCHIVE was not found." >&2
  exit 1
fi

printf '%s\n' "$EXPECTED_LINE" | (cd "$TMP_DIR" && sha256sum --check --strict)
ACTUAL_SHA256="$(sha256sum "$TMP_DIR/$ARCHIVE" | awk '{print $1}')"
printf 'SHA256: %s\n' "$ACTUAL_SHA256"

rm -rf "$TMP_DIR/source"
mkdir -p "$TMP_DIR/source"
tar -xJf "$TMP_DIR/$ARCHIVE" -C "$TMP_DIR/source"

EXTRACTED="$TMP_DIR/source/orca-${VERSION}"
if [ ! -d "$EXTRACTED" ]; then
  echo "ERROR: archive did not contain expected directory orca-${VERSION}" >&2
  exit 1
fi

rm -rf "$UPSTREAM_DIR/orca-${VERSION}"
cp -a "$EXTRACTED" "$UPSTREAM_DIR/orca-${VERSION}"
printf '%s\n' "$ACTUAL_SHA256" > "$UPSTREAM_DIR/orca-${VERSION}.sha256"
cp "$TMP_DIR/orca.sha256sum" "$UPSTREAM_DIR/orca-${VERSION}.sha256sum"
cat > "$UPSTREAM_DIR/SOURCE-INFO.txt" <<EOF
Project: Orca
Version: ${VERSION}
Source URL: ${SOURCE_URL}
Checksum URL: ${CHECKSUM_URL}
SHA256: ${ACTUAL_SHA256}
Acquired: $(date -u '+%Y-%m-%dT%H:%M:%SZ')

This directory contains pristine upstream source. Determinant modifications
must be maintained outside this directory as patches/offsets.
EOF

printf '=== Orca source installed ===\n'
printf '  %s\n' "$UPSTREAM_DIR/orca-${VERSION}"
