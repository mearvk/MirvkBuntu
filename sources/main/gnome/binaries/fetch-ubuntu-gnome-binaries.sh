#!/usr/bin/env bash
set -euo pipefail

# Ubuntu Resolute 26.04 LTS, amd64 GNOME runtime baseline.
# These packages are downloaded from the official Ubuntu archive and verified
# against the package metadata published by packages.ubuntu.com.

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
BASE_URL="https://archive.ubuntu.com/ubuntu"

fetch_and_verify() {
    local relpath="$1"
    local filename="$2"
    local sha256="$3"

    echo "Fetching ${filename}"
    curl --fail --location --retry 3 --output "${SCRIPT_DIR}/${filename}.tmp" \
        "${BASE_URL}/${relpath}/${filename}"

    echo "Verifying ${filename}"
    printf '%s  %s\n' "${sha256}" "${SCRIPT_DIR}/${filename}.tmp" | sha256sum --check --strict
    mv -f "${SCRIPT_DIR}/${filename}.tmp" "${SCRIPT_DIR}/${filename}"
}

fetch_and_verify \
    "pool/main/g/gnome-shell" \
    "gnome-shell_50.1-0ubuntu1_amd64.deb" \
    "64d5f86b3ecc431b353ba032b46aa114d3b46588f9c6c57206bdb2f4e1bf04e"

fetch_and_verify \
    "pool/universe/m/mutter" \
    "mutter_50.1-0ubuntu2_amd64.deb" \
    "4bac98f9644667e7df8e1bcf5ac9a00216e3011eab4d59e113ae404f1d8d4315"

fetch_and_verify \
    "pool/main/n/nautilus" \
    "nautilus_50.0-0ubuntu2_amd64.deb" \
    "f5c1f469b2a28a7524f86d8c4848d9819f781ff09bbc22530a7cdfec6cedc2ef"

echo
echo "GNOME binary baseline populated in: ${SCRIPT_DIR}"
echo "Note: runtime dependencies are intentionally resolved by the ISO build system."
