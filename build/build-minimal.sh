#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/common-build.sh"
require_commands live-build lb
require_mirvkbuntu_source
prepare_dirs
WORK_DIR="${BUILD_ROOT}/minimal"
rm -rf "$WORK_DIR"
mkdir -p "$WORK_DIR"
cd "$WORK_DIR"
lb clean --purge >/dev/null 2>&1 || true
lb config --distribution "$UBUNTU_SUITE" --architectures "$ARCH" --binary-images iso-hybrid --debian-installer live --apt-recommends false --archive-areas "main restricted universe multiverse" --iso-application "MirvkBuntu" --iso-publisher "MEARVK LLC" --iso-volume "$ISO_LABEL-MINIMAL"
write_package_list "$WORK_DIR" mirvkbuntu-minimal
stage_mirvkbuntu_source "$WORK_DIR"
write_mirvkbuntu_manifest "$WORK_DIR"
ISO_SOURCE="$(run_live_build "$WORK_DIR")"
ISO_SOURCE="$(find_iso "$WORK_DIR")"
mkdir -p "$OUTPUT_ROOT"
mv -f "$ISO_SOURCE" "$OUTPUT_ROOT/MirvkBuntu-minimal-${ARCH}.iso"
printf 'build: created %s\n' "$OUTPUT_ROOT/MirvkBuntu-minimal-${ARCH}.iso"
publish_iso "$OUTPUT_ROOT/MirvkBuntu-minimal-${ARCH}.iso"
