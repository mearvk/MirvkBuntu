#!/usr/bin/env bash
set -euo pipefail

# ISO acceptance gate for repository-source-driven MirvkBuntu releases.
# This validates the generated ISO and build records; it does not accept an
# existing Ubuntu ISO as an input.

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(CDPATH= cd -- "${SCRIPT_DIR}/.." && pwd)"

die(){ printf 'release-validate: ERROR: %s\n' "$*" >&2; exit 1; }
log(){ printf 'release-validate: %s\n' "$*"; }

ISO="${1:-}"
WORK="${2:-}"
EDITION="${3:-limited}"
[ -f "$ISO" ] || die "ISO not found: $ISO"
[ -d "$WORK" ] || die "build work directory not found: $WORK"

command -v file >/dev/null 2>&1 || die "required command not found: file"
command -v sha256sum >/dev/null 2>&1 || die "required command not found: sha256sum"
command -v xorriso >/dev/null 2>&1 || die "required command not found: xorriso"

FILE_TYPE="$(file -b "$ISO")"
printf '%s\n' "$FILE_TYPE" | grep -Eiq 'ISO 9660|bootable|DOS/MBR' || \
  die "generated file is not recognized as an ISO image: $FILE_TYPE"

TOC="$(mktemp)"
trap 'rm -f "$TOC"' EXIT
xorriso -indev "$ISO" -toc 2>"$TOC" >/dev/null || die "xorriso could not read the generated ISO"
grep -Eiq 'ISO volume|El Torito|boot catalog' "$TOC" || die "generated ISO lacks recognizable ISO/boot catalog metadata"

SOURCE_MANIFEST="$WORK/config/includes.chroot/etc/mirvkbuntu/source.sha256"
EDITION_CONF="$WORK/config/includes.chroot/etc/mirvkbuntu/edition.conf"
[ -f "$SOURCE_MANIFEST" ] || die "MirvkBuntu source SHA-256 manifest missing"
[ -s "$SOURCE_MANIFEST" ] || die "MirvkBuntu source SHA-256 manifest is empty"

if [ "$EDITION" = "limited" ]; then
  [ -f "$EDITION_CONF" ] || die "limited edition metadata missing"
  grep -q '^MIRVKBUNTU_EXISTING_ISO_INPUT=false$' "$EDITION_CONF" || \
    die "limited edition does not explicitly declare existing ISO input=false"
  grep -q '^MIRVKBUNTU_DEB_ARCHIVES_ALLOWED=true$' "$EDITION_CONF" || \
    die "limited edition does not explicitly allow APT .deb acquisition"
fi

# The limited build path itself must not regress toward the old remaster model.
if grep -nE 'quick-remaster|SOURCE_ISO|ubuntu.*\.iso|\.iso.*input' "$REPO_ROOT/build/build-limited.sh" >/dev/null 2>&1; then
  die "limited build script contains an existing-ISO/remaster input reference"
fi

[ -s "$ISO" ] || die "generated ISO is empty"
SHA_FILE="${ISO}.sha256"
[ -f "$SHA_FILE" ] || die "ISO SHA-256 sidecar missing: $SHA_FILE"
[ -f "${ISO}.release" ] || die "release manifest sidecar missing: ${ISO}.release"
EXPECTED="$(awk '{print $1}' "$SHA_FILE")"
ACTUAL="$(sha256sum "$ISO" | awk '{print $1}')"
[ "$EXPECTED" = "$ACTUAL" ] || die "ISO SHA-256 mismatch"

log "accepted edition=$EDITION"
log "ISO=$ISO"
log "sha256=$ACTUAL"
log "source manifest=$SOURCE_MANIFEST"
