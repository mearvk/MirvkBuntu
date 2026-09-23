#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/common-build.sh"
require_commands live-build lb
require_mirvkbuntu_source
prepare_dirs
run_native_build
WORK_DIR="${BUILD_ROOT}/desktop"
rm -rf "$WORK_DIR"
mkdir -p "$WORK_DIR"
cd "$WORK_DIR"
lb clean --purge >/dev/null 2>&1 || true
lb config --distribution "$UBUNTU_SUITE" --architectures "$ARCH" --binary-images iso-hybrid --debian-installer live --apt-recommends true --archive-areas "main restricted universe multiverse" --iso-application "MirvkBuntu" --iso-publisher "MEARVK LLC" --iso-volume "$ISO_LABEL-DESKTOP" --linux-packages none
write_package_list "$WORK_DIR" mirvkbuntu-desktop
stage_mirvkbuntu_source "$WORK_DIR"
write_mirvkbuntu_manifest "$WORK_DIR"
stage_native_outputs "$WORK_DIR"
run_live_build "$WORK_DIR"
ISO_SOURCE="$(find_iso "$WORK_DIR")"
mkdir -p "$OUTPUT_ROOT"
mv -f "$ISO_SOURCE" "$OUTPUT_ROOT/MirvkBuntu-desktop-${ARCH}.iso"
printf 'build: created %s\n' "$OUTPUT_ROOT/MirvkBuntu-desktop-${ARCH}.iso"
publish_iso "$OUTPUT_ROOT/MirvkBuntu-desktop-${ARCH}.iso"
write_release_manifest "$OUTPUT_ROOT/MirvkBuntu-desktop-${ARCH}.iso" "$WORK_DIR" "full"
validate_release_iso "$OUTPUT_ROOT/MirvkBuntu-desktop-${ARCH}.iso" "$WORK_DIR" "full"
