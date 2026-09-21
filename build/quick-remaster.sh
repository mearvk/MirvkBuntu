#!/usr/bin/env bash
set -euo pipefail

# MirvkBuntu quick ISO remaster (path A).
#
# Turns a stock Ubuntu desktop ISO into a MirvkBuntu variant WITHOUT compiling
# a kernel or any source tree. It:
#   1. Extracts the ISO and the live filesystem (filesystem.squashfs).
#   2. Enters the live filesystem via chroot and installs the packages listed
#      in packages/basic-packages.txt.
#   3. Applies the ubuntu-white/ theme assets and MirvkBuntu branding.
#   4. Repacks the squashfs and rebuilds a bootable hybrid ISO with xorriso,
#      preserving the original ISO's boot records (BIOS + UEFI).
#
# This is the fast path to a bootable variant. For the full compile-from-source
# distribution, use build/build-desktop.sh instead.
#
# Requirements: run as root; host tooling from build/prerequisites.sh remaster.
#
# Usage:
#   sudo bash build/quick-remaster.sh /path/to/ubuntu-24.04-desktop-amd64.iso
#   sudo bash build/quick-remaster.sh ubuntu.iso -o build/output/MirvkBuntu.iso

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(CDPATH= cd -- "${SCRIPT_DIR}/.." && pwd)"

BUILD_ROOT="${BUILD_ROOT:-${REPO_ROOT}/build/work/remaster}"
OUTPUT_ROOT="${OUTPUT_ROOT:-${REPO_ROOT}/build/output}"
ISO_LABEL="${ISO_LABEL:-MIRVKBUNTU}"
ISO_APPLICATION="${ISO_APPLICATION:-MirvkBuntu}"
ISO_PUBLISHER="${ISO_PUBLISHER:-MEARVK LLC}"
PACKAGE_MANIFEST="${PACKAGE_MANIFEST:-${REPO_ROOT}/packages/basic-packages.txt}"
THEME_DIR="${THEME_DIR:-${REPO_ROOT}/ubuntu-white}"
# When 1, skip the chroot apt step (theme/branding only). Useful offline.
SKIP_PACKAGES="${SKIP_PACKAGES:-0}"

die(){ printf 'remaster: ERROR: %s\n' "$*" >&2; exit 1; }
log(){ printf 'remaster: %s\n' "$*"; }

# --- argument parsing --------------------------------------------------------
SOURCE_ISO=""
OUTPUT_ISO=""
while [ $# -gt 0 ]; do
  case "$1" in
    -o|--output) OUTPUT_ISO="${2:-}"; shift 2 || die "--output requires a path" ;;
    -h|--help) sed -n '3,26p' "${BASH_SOURCE[0]}"; exit 0 ;;
    -*) die "unknown option: $1" ;;
    *) [ -z "$SOURCE_ISO" ] || die "only one source ISO may be given"; SOURCE_ISO="$1"; shift ;;
  esac
done

[ -n "$SOURCE_ISO" ] || die "usage: quick-remaster.sh <ubuntu.iso> [-o output.iso]"
[ -f "$SOURCE_ISO" ] || die "source ISO not found: $SOURCE_ISO"
SOURCE_ISO="$(readlink -f "$SOURCE_ISO")"
OUTPUT_ISO="${OUTPUT_ISO:-${OUTPUT_ROOT}/MirvkBuntu-remaster-amd64.iso}"

# --- host checks -------------------------------------------------------------
[ "$(id -u)" -eq 0 ] || die "must run as root (needed for chroot + mount); try: sudo bash build/quick-remaster.sh ..."
for c in xorriso unsquashfs mksquashfs rsync; do
  command -v "$c" >/dev/null 2>&1 || die "required command not found: $c (run build/prerequisites.sh remaster)"
done
[ -f "$PACKAGE_MANIFEST" ] || die "package manifest missing: $PACKAGE_MANIFEST"
[ -d "$THEME_DIR" ] || die "theme directory missing: $THEME_DIR"

# --- workspace ---------------------------------------------------------------
ISO_EXTRACT="${BUILD_ROOT}/iso"
SQUASH_MNT="${BUILD_ROOT}/squashfs"     # read-only loop mount of the squashfs
ROOTFS="${BUILD_ROOT}/rootfs"           # writable copy of the live filesystem
MOUNTED=()

cleanup(){
  local m
  # Unmount chroot bind mounts and the squashfs in reverse order; ignore errors.
  for (( idx=${#MOUNTED[@]}-1 ; idx>=0 ; idx-- )); do
    m="${MOUNTED[$idx]}"
    mountpoint -q "$m" && umount -lf "$m" 2>/dev/null || true
  done
}
trap cleanup EXIT INT TERM

log "source ISO : $SOURCE_ISO"
log "output ISO : $OUTPUT_ISO"
log "workspace  : $BUILD_ROOT"

rm -rf "$BUILD_ROOT"
mkdir -p "$ISO_EXTRACT" "$SQUASH_MNT" "$ROOTFS" "$(dirname "$OUTPUT_ISO")"

# --- 1. extract the ISO ------------------------------------------------------
log "extracting ISO contents with xorriso"
xorriso -osirrox on -indev "$SOURCE_ISO" -extract / "$ISO_EXTRACT" >/dev/null 2>&1 \
  || die "failed to extract ISO with xorriso"
# xorriso extracts read-only; make the tree writable so we can replace the squashfs.
chmod -R u+w "$ISO_EXTRACT"

# Locate the live filesystem squashfs (Ubuntu: casper/filesystem.squashfs;
# newer minimal-image layouts may nest it under casper/*.squashfs).
SQUASHFS_PATH="$(find "$ISO_EXTRACT/casper" -maxdepth 1 -name '*.squashfs' 2>/dev/null | sort | head -1 || true)"
[ -n "$SQUASHFS_PATH" ] || SQUASHFS_PATH="$(find "$ISO_EXTRACT" -name 'filesystem.squashfs' -print -quit 2>/dev/null || true)"
[ -n "$SQUASHFS_PATH" ] || die "could not locate a live filesystem squashfs inside the ISO (expected casper/*.squashfs)"
log "live filesystem: ${SQUASHFS_PATH#$ISO_EXTRACT/}"

# --- 2. unpack the live filesystem ------------------------------------------
log "unpacking live filesystem (this can take a few minutes)"
rm -rf "$ROOTFS"
unsquashfs -f -d "$ROOTFS" "$SQUASHFS_PATH" >/dev/null || die "unsquashfs failed"

# --- 3. chroot preparation ---------------------------------------------------
prepare_chroot(){
  local rootfs="$1" t
  cp -f /etc/resolv.conf "$rootfs/etc/resolv.conf" 2>/dev/null || true
  for t in /dev /dev/pts /proc /sys /run; do
    mkdir -p "$rootfs$t"
    mount --bind "$t" "$rootfs$t"
    MOUNTED+=( "$rootfs$t" )
  done
}

run_in_chroot(){ chroot "$ROOTFS" /usr/bin/env -i \
  HOME=/root PATH=/usr/sbin:/usr/bin:/sbin:/bin DEBIAN_FRONTEND=noninteractive \
  /bin/bash -c "$1"; }

# --- 4. install MirvkBuntu package set --------------------------------------
if [ "$SKIP_PACKAGES" != "1" ]; then
  log "preparing chroot and installing MirvkBuntu package set"
  prepare_chroot "$ROOTFS"

  # Build a space-separated, comment-free package list from the manifest.
  mapfile -t PKGS < <(awk '!/^[[:space:]]*#/ && NF {print $1}' "$PACKAGE_MANIFEST" | sort -u)
  [ "${#PKGS[@]}" -gt 0 ] || die "package manifest produced no packages: $PACKAGE_MANIFEST"
  log "installing ${#PKGS[@]} package(s) from manifest"

  if ! run_in_chroot "apt-get update"; then
    die "apt-get update failed inside chroot (network access required for the remaster's package step; re-run with SKIP_PACKAGES=1 to skip)"
  fi
  # Install best-effort: a missing package name should not abort the whole build.
  run_in_chroot "apt-get install -y --no-install-recommends ${PKGS[*]}" \
    || log "WARNING: some packages failed to install; continuing (inspect chroot log above)"
  run_in_chroot "apt-get clean" || true
else
  log "SKIP_PACKAGES=1 set; skipping the chroot apt step (theme/branding only)"
fi

# --- 5. apply ubuntu-white theme + branding ---------------------------------
log "applying ubuntu-white theme assets and MirvkBuntu branding"
THEME_DEST="$ROOTFS/usr/share/mirvkbuntu/ubuntu-white"
mkdir -p "$THEME_DEST"
rsync -a --delete "$THEME_DIR/" "$THEME_DEST/"

# Install the GTK stylesheet where a system-wide GTK theme override lives.
if [ -f "$THEME_DIR/gtk.css" ]; then
  mkdir -p "$ROOTFS/usr/share/themes/MirvkBuntu-White/gtk-3.0" \
           "$ROOTFS/usr/share/themes/MirvkBuntu-White/gtk-4.0"
  cp -f "$THEME_DIR/gtk.css" "$ROOTFS/usr/share/themes/MirvkBuntu-White/gtk-3.0/gtk.css"
  cp -f "$THEME_DIR/gtk.css" "$ROOTFS/usr/share/themes/MirvkBuntu-White/gtk-4.0/gtk.css"
fi

# Install icon sets (if present) into a MirvkBuntu icon location.
if [ -d "$THEME_DIR/icons" ]; then
  mkdir -p "$ROOTFS/usr/share/icons/MirvkBuntu-White"
  rsync -a "$THEME_DIR/icons/" "$ROOTFS/usr/share/icons/MirvkBuntu-White/"
fi

# OS branding: identify the variant in os-release without breaking Ubuntu's
# ID/ID_LIKE (so apt, snap, and third-party tooling keep working).
if [ -f "$ROOTFS/etc/os-release" ]; then
  # Drop any previous MirvkBuntu markers, then append fresh ones.
  sed -i '/^MIRVKBUNTU/d; /MirvkBuntu/d' "$ROOTFS/etc/os-release" || true
  {
    printf 'MIRVKBUNTU_VARIANT=1\n'
    printf 'PRETTY_NAME="MirvkBuntu (Ubuntu variant)"\n'
    printf 'VARIANT="MirvkBuntu"\n'
    printf 'VARIANT_ID=mirvkbuntu\n'
  } >> "$ROOTFS/etc/os-release"
fi
mkdir -p "$ROOTFS/etc/mirvkbuntu"
{
  printf 'MIRVKBUNTU_REMASTER=true\n'
  printf 'MIRVKBUNTU_BASE_ISO=%s\n' "$(basename "$SOURCE_ISO")"
  printf 'MIRVKBUNTU_THEME=ubuntu-white\n'
  printf 'MIRVKBUNTU_BUILD_DATE=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
} > "$ROOTFS/etc/mirvkbuntu/remaster.conf"

# Unmount chroot binds before repacking so the squashfs excludes them.
cleanup
MOUNTED=()

# --- 6. repack the squashfs --------------------------------------------------
log "repacking live filesystem into squashfs"
NEW_SQUASH="${BUILD_ROOT}/filesystem.squashfs.new"
rm -f "$NEW_SQUASH"
mksquashfs "$ROOTFS" "$NEW_SQUASH" -comp xz -noappend -no-progress \
  || die "mksquashfs failed"
mv -f "$NEW_SQUASH" "$SQUASHFS_PATH"

# Refresh the uncompressed size hint Ubuntu's installer reads, if present.
SIZE_FILE="$(dirname "$SQUASHFS_PATH")/filesystem.size"
if [ -f "$SIZE_FILE" ]; then
  du -sx --block-size=1 "$ROOTFS" | cut -f1 > "$SIZE_FILE" || true
fi

# Update the ISO's internal MD5 manifest so integrity checks pass, if present.
if [ -f "$ISO_EXTRACT/md5sum.txt" ]; then
  log "refreshing md5sum.txt"
  ( cd "$ISO_EXTRACT" && \
    find . -type f -not -path './md5sum.txt' -not -path './isolinux/boot.cat' -print0 \
      | xargs -0 md5sum > md5sum.txt ) || true
fi

# --- 7. rebuild the bootable ISO --------------------------------------------
# Boot records differ across Ubuntu ISO layouts (BIOS isolinux vs GRUB, and the
# exact EFI image path). The most robust and layout-independent approach is to
# let xorriso load the source ISO, REPLAY its existing El Torito / MBR / GPT
# boot setup verbatim, and only replace the changed files (the squashfs, and the
# refreshed manifests). This preserves BIOS + UEFI boot exactly as the source.
log "rebuilding bootable ISO by replaying the source boot records"
rm -f "$OUTPUT_ISO"

# Files we changed and must map back into the cloned image.
declare -a MAP_ARGS=( -map "$SQUASHFS_PATH" "/${SQUASHFS_PATH#"$ISO_EXTRACT"/}" )
if [ -f "$SIZE_FILE" ]; then
  MAP_ARGS+=( -map "$SIZE_FILE" "/${SIZE_FILE#"$ISO_EXTRACT"/}" )
fi
if [ -f "$ISO_EXTRACT/md5sum.txt" ]; then
  MAP_ARGS+=( -map "$ISO_EXTRACT/md5sum.txt" "/md5sum.txt" )
fi

if ! xorriso -indev "$SOURCE_ISO" \
             -outdev "$OUTPUT_ISO" \
             -boot_image any replay \
             -volid "${ISO_LABEL}" \
             -application_id "${ISO_APPLICATION}" \
             -publisher "${ISO_PUBLISHER}" \
             "${MAP_ARGS[@]}" \
             -commit 2> "${BUILD_ROOT}/xorriso.log"; then
  log "boot-record replay failed; see ${BUILD_ROOT}/xorriso.log"
  log "attempting a full re-authoring from the extracted tree (GRUB/EFI layout)"
  # Fallback for layouts where replay is unavailable: author afresh, cloning the
  # common Ubuntu GRUB hybrid boot images if they exist.
  declare -a MK=( -as mkisofs -V "${ISO_LABEL}" -A "${ISO_APPLICATION}"
                  -publisher "${ISO_PUBLISHER}" -r -J -joliet-long -iso-level 3 )
  if [ -f "$ISO_EXTRACT/boot/grub/i386-pc/eltorito.img" ]; then
    MK+=( -c boot.catalog
          -b boot/grub/i386-pc/eltorito.img -no-emul-boot -boot-load-size 4
          -boot-info-table --grub2-boot-info )
    [ -f "$ISO_EXTRACT/boot/grub/i386-pc/boot_hybrid.img" ] && \
      MK+=( --grub2-mbr "$ISO_EXTRACT/boot/grub/i386-pc/boot_hybrid.img" )
  fi
  if [ -f "$ISO_EXTRACT/EFI/boot/efiboot.img" ]; then
    MK+=( -eltorito-alt-boot -e EFI/boot/efiboot.img -no-emul-boot
          -append_partition 2 0xef "$ISO_EXTRACT/EFI/boot/efiboot.img" )
  fi
  MK+=( -o "$OUTPUT_ISO" "$ISO_EXTRACT" )
  xorriso "${MK[@]}" 2> "${BUILD_ROOT}/xorriso-fallback.log" \
    || die "ISO rebuild failed; see ${BUILD_ROOT}/xorriso.log and ${BUILD_ROOT}/xorriso-fallback.log"
fi

# Ensure the output is USB hybrid-bootable if isohybrid is available (the replay
# path usually preserves this already; harmless to reassert).
if command -v isohybrid >/dev/null 2>&1; then
  isohybrid --uefi "$OUTPUT_ISO" 2>/dev/null || isohybrid "$OUTPUT_ISO" 2>/dev/null || true
fi

log "done."
log "MirvkBuntu variant ISO: $OUTPUT_ISO"
[ -f "$OUTPUT_ISO" ] && log "size: $(du -h "$OUTPUT_ISO" | cut -f1)"
