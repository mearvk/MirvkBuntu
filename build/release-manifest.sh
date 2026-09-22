#!/usr/bin/env bash
set -euo pipefail

# Produce a release record for an ISO assembled from the MirvkBuntu checkout.

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(CDPATH= cd -- "${SCRIPT_DIR}/.." && pwd)"

die(){ printf 'release-manifest: ERROR: %s\n' "$*" >&2; exit 1; }

ISO="${1:-}"
WORK="${2:-}"
EDITION="${3:-limited}"
OUT="${4:-}"

[ -f "$ISO" ] || die "ISO not found: $ISO"
[ -d "$WORK" ] || die "build work directory not found: $WORK"
[ -n "$OUT" ] || OUT="${ISO}.release"
mkdir -p "$(dirname "$OUT")"

COMMIT="unknown"
if command -v git >/dev/null 2>&1 && git -C "$REPO_ROOT" rev-parse --verify HEAD >/dev/null 2>&1; then
  COMMIT="$(git -C "$REPO_ROOT" rev-parse HEAD)"
fi

ISO_SHA="$(sha256sum "$ISO" | awk '{print $1}')"
SOURCE_MANIFEST="$WORK/config/includes.chroot/etc/mirvkbuntu/source.sha256"
DEPENDENCY_CACHE="$REPO_ROOT/build/work/dependency-cache.txt"
COMPONENT_INVENTORY="$REPO_ROOT/build/work/component-inventory.txt"

{
  echo "MIRVKBUNTU_RELEASE_MANIFEST=1"
  echo "MIRVKBUNTU_EDITION=$EDITION"
  echo "MIRVKBUNTU_SOURCE_REPOSITORY=mearvk/MirvkBuntu"
  echo "MIRVKBUNTU_SOURCE_COMMIT=$COMMIT"
  echo "MIRVKBUNTU_UBUNTU_SUITE=${UBUNTU_SUITE:-noble}"
  echo "MIRVKBUNTU_ARCH=${ARCH:-amd64}"
  echo "MIRVKBUNTU_EXISTING_ISO_INPUT=false"
  echo "MIRVKBUNTU_DEB_ARCHIVES_ALLOWED=true"
  echo "MIRVKBUNTU_ISO_SHA256=$ISO_SHA"
  echo "MIRVKBUNTU_SOURCE_MANIFEST_PRESENT=$([ -f "$SOURCE_MANIFEST" ] && echo true || echo false)"
  echo "MIRVKBUNTU_DEPENDENCY_CACHE_PRESENT=$([ -f "$DEPENDENCY_CACHE" ] && echo true || echo false)"
  echo "MIRVKBUNTU_COMPONENT_INVENTORY_PRESENT=$([ -f "$COMPONENT_INVENTORY" ] && echo true || echo false)"
  echo "MIRVKBUNTU_BUILD_WORK=$WORK"
  echo "MIRVKBUNTU_PACKAGE_ARCHIVES=Ubuntu/Debian .deb repositories via APT"
  echo "MIRVKBUNTU_ASSEMBLER=live-build"
} > "$OUT"

printf 'release-manifest: wrote %s\n' "$OUT"
printf '%s  %s\n' "$ISO_SHA" "$(basename "$ISO")" > "${ISO}.sha256"
if [ -f "$COMPONENT_INVENTORY" ]; then
  cp -f "$COMPONENT_INVENTORY" "${ISO}.components"
fi
if [ -f "$REPO_ROOT/build/work/dependency-resolved.txt" ]; then
  cp -f "$REPO_ROOT/build/work/dependency-resolved.txt" "${ISO}.packages"
fi
