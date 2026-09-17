#!/usr/bin/env bash
set -euo pipefail

# MirvkBuntu: deliver the complete gnome-source tree
# from Ubuntu.Determinant.Beta.Restricted into a relative directory.
#
# Usage:
#   ./fetch-gnome-source.sh
#   ./fetch-gnome-source.sh ./sources/work/gnome-source
#
# No GitHub credentials are required for this public repository.

REPO_OWNER="mearvk"
REPO_NAME="Ubuntu.Determinant.Beta.Restricted"
REF="main"

DEST="${1:-./sources/work/gnome-source}"
TMP="${DEST}.tmp"
ZIP="${DEST}.zip"

command -v curl >/dev/null 2>&1 || {
    echo "ERROR: curl is required." >&2
    exit 1
}

command -v unzip >/dev/null 2>&1 || {
    echo "ERROR: unzip is required." >&2
    exit 1
}

rm -rf "$TMP" "$ZIP"
mkdir -p "$DEST"

URL="https://github.com/${REPO_OWNER}/${REPO_NAME}/archive/refs/heads/${REF}.zip"

echo "Downloading public source archive..."
echo "Repository: ${REPO_OWNER}/${REPO_NAME}"
echo "Reference:  ${REF}"
echo "Destination: ${DEST}"

curl \
    --fail \
    --silent \
    --show-error \
    --location \
    --retry 3 \
    --retry-delay 2 \
    --connect-timeout 15 \
    --max-time 0 \
    --output "$ZIP" \
    "$URL"

mkdir -p "$TMP"

unzip -q "$ZIP" -d "$TMP"

ROOT="$(find "$TMP" -mindepth 1 -maxdepth 1 -type d -name "${REPO_NAME}-*" -print -quit)"

if [[ -z "$ROOT" ]]; then
    echo "ERROR: repository archive root was not found." >&2
    rm -rf "$TMP" "$ZIP"
    exit 1
fi

if [[ ! -d "$ROOT/gnome-source" ]]; then
    echo "ERROR: gnome-source/ was not found in the reference repository." >&2
    rm -rf "$TMP" "$ZIP"
    exit 1
fi

rm -rf "$DEST"
mkdir -p "$DEST"

cp -a "$ROOT/gnome-source/." "$DEST/"

rm -rf "$TMP" "$ZIP"

echo
echo "GNOME source delivery complete."
echo "Delivered to: $DEST"
echo
echo "Contents:"
find "$DEST" -maxdepth 2 -type f -print | sort
