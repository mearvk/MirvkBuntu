#!/usr/bin/env bash
set -euo pipefail

# MirvkBuntu basic kernel source acquisition.
# Reference carried forward from Ubuntu.Determinant.Beta.Restricted.

VERSION="${LINUX_VERSION:-5.15.204}"
BASE_URL="https://cdn.kernel.org/pub/linux/kernel/v5.x"
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
DEST="${ROOT}/sources/work/kernel"
ARCHIVE="${DEST}/linux-${VERSION}.tar.xz"
URL="${BASE_URL}/linux-${VERSION}.tar.xz"

mkdir -p "$DEST"

if [[ ! -f "$ARCHIVE" ]]; then
    command -v curl >/dev/null 2>&1 || { echo "curl is required" >&2; exit 1; }
    echo "Downloading Linux ${VERSION} from ${URL}"
    curl --fail --location --retry 3 --output "$ARCHIVE" "$URL"
else
    echo "Using existing $ARCHIVE"
fi

if [[ ! -d "${DEST}/linux-${VERSION}" ]]; then
    tar -C "$DEST" -xf "$ARCHIVE"
fi

echo "Linux source: ${DEST}/linux-${VERSION}"
