#!/bin/bash
# SPDX-License-Identifier: GPL-2.0
#
# recombine-sources.sh — Recombine split Ubuntu source ISOs
#
# The ubuntu.slaves.black/ directory contains the Ubuntu 22.04.3 LTS
# source media, split into ~20 MB chunks to fit within Git hosting
# file size limits. This script recombines them into usable ISO images
# that can be mounted or extracted for local compilation.
#
# Source discs:
#   1/ → Ubuntu 22.04.3 LTS Source Disc 1 (~4.4 GB)
#   2/ → Ubuntu 22.04.3 LTS Source Disc 2 (~4.5 GB)
#   3/ → Ubuntu 22.04.3 LTS Source Disc 3 (~1.3 GB)
#   4/ → Ubuntu 22.04.3 LTS Source Disc 4 (~1.4 GB)
#   5/ → (empty marker)
#
# These contain the full source code for all packages in Ubuntu 22.04.3,
# enabling offline compilation without network access.
#
# Usage:
#   ./recombine-sources.sh                  Recombine all discs
#   ./recombine-sources.sh --disc 1         Recombine disc 1 only
#   ./recombine-sources.sh --mount 1        Recombine and mount disc 1
#   ./recombine-sources.sh --extract 1 DIR  Recombine and extract disc 1 to DIR
#   ./recombine-sources.sh --verify         Verify recombined ISOs
#   ./recombine-sources.sh --list 1         List contents of disc 1 (after recombine)
#   ./recombine-sources.sh --clean          Remove recombined ISOs
#
# Copyright (C) 2026 MEARVK LLC
# Author: Maximilian Eric Alexander Rupplin von Keffikon

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SOURCE_DIR="${SCRIPT_DIR}/ubuntu.slaves.black"
OUTPUT_DIR="${SCRIPT_DIR}/build/source-isos"
MOUNT_BASE="/mnt/ubuntu-source"

# ============================================================
# Functions
# ============================================================

die() {
    echo "ERROR: $1" >&2
    exit 1
}

info() {
    echo "=== $1"
}

usage() {
    cat <<EOF
recombine-sources.sh — Recombine split Ubuntu source ISOs

Usage:
  $(basename "$0")                        Recombine all source discs (1-4)
  $(basename "$0") --disc N               Recombine disc N only (1-4)
  $(basename "$0") --mount N              Recombine + mount disc N at ${MOUNT_BASE}/N
  $(basename "$0") --extract N DIR        Recombine + extract disc N contents to DIR
  $(basename "$0") --verify               SHA-256 verify all recombined ISOs
  $(basename "$0") --list N               List files on disc N
  $(basename "$0") --status               Show current state (what's recombined/mounted)
  $(basename "$0") --clean                Remove recombined ISOs (free disk space)
  $(basename "$0") --help                 This message

Source structure:
  ubuntu.slaves.black/1/ubuntu_1_aa .. ubuntu_1_im   (221 chunks, ~4.4 GB)
  ubuntu.slaves.black/2/ubuntu_2_aa .. ubuntu_2_im   (221 chunks, ~4.5 GB)
  ubuntu.slaves.black/3/ubuntu_3_aa .. ubuntu_3_cj   (62 chunks, ~1.3 GB)
  ubuntu.slaves.black/4/ubuntu_4_aa .. ubuntu_4_cs   (71 chunks, ~1.4 GB)

Total source media: ~11.6 GB (Ubuntu 22.04.3 LTS complete source)

For local compilation:
  1. Recombine: ./recombine-sources.sh
  2. Mount:     ./recombine-sources.sh --mount 1
  3. Find source package: ls /mnt/ubuntu-source/1/
  4. Extract .dsc: dpkg-source -x package.dsc
  5. Build: dpkg-buildpackage -us -uc

EOF
    exit 0
}

# Recombine a single disc
recombine_disc() {
    local disc="$1"
    local disc_dir="${SOURCE_DIR}/${disc}"
    local output="${OUTPUT_DIR}/ubuntu-source-${disc}.iso"

    if [ ! -d "${disc_dir}" ]; then
        die "Disc directory not found: ${disc_dir}"
    fi

    # Count chunks
    local chunk_count
    chunk_count=$(ls -1 "${disc_dir}"/ubuntu_${disc}_* 2>/dev/null | wc -l)

    if [ "${chunk_count}" -eq 0 ]; then
        echo "  Disc ${disc}: no chunks found, skipping"
        return 0
    fi

    # Skip if already recombined and up-to-date
    if [ -f "${output}" ]; then
        local iso_mtime chunk_mtime
        iso_mtime=$(stat -c '%Y' "${output}")
        chunk_mtime=$(stat -c '%Y' "${disc_dir}"/ubuntu_${disc}_aa)
        if [ "${iso_mtime}" -ge "${chunk_mtime}" ]; then
            echo "  Disc ${disc}: already recombined (${output})"
            return 0
        fi
    fi

    mkdir -p "${OUTPUT_DIR}"

    echo "  Disc ${disc}: recombining ${chunk_count} chunks..."

    # Concatenate in sorted order (aa, ab, ac, ... az, ba, bb, ...)
    cat "${disc_dir}"/ubuntu_${disc}_* > "${output}"

    local size
    size=$(du -h "${output}" | awk '{print $1}')
    echo "  Disc ${disc}: ${output} (${size})"

    # Quick verification: check ISO magic
    if ! file "${output}" | grep -q "ISO 9660"; then
        echo "  WARNING: Disc ${disc} does not appear to be a valid ISO 9660 image"
        echo "           (may need sorted recombination — checking order)"
        rm -f "${output}"

        # Try with explicit LC_ALL=C sort for correct alphabetical order
        ls -1 "${disc_dir}"/ubuntu_${disc}_* | LC_ALL=C sort | xargs cat > "${output}"

        if ! file "${output}" | grep -q "ISO 9660"; then
            die "Disc ${disc}: recombination failed — not a valid ISO"
        fi
    fi

    echo "  Disc ${disc}: ✓ valid ISO 9660"
}

# Mount a disc ISO
mount_disc() {
    local disc="$1"
    local output="${OUTPUT_DIR}/ubuntu-source-${disc}.iso"
    local mount_point="${MOUNT_BASE}/${disc}"

    if [ ! -f "${output}" ]; then
        info "Recombining disc ${disc} first..."
        recombine_disc "${disc}"
    fi

    if [ ! -f "${output}" ]; then
        die "ISO not found: ${output}"
    fi

    if mountpoint -q "${mount_point}" 2>/dev/null; then
        echo "  Disc ${disc}: already mounted at ${mount_point}"
        return 0
    fi

    mkdir -p "${mount_point}"
    mount -o loop,ro "${output}" "${mount_point}"
    echo "  Disc ${disc}: mounted at ${mount_point}"
    echo "  Contents: $(ls "${mount_point}" | wc -l) entries"
}

# Extract disc contents
extract_disc() {
    local disc="$1"
    local dest="$2"
    local output="${OUTPUT_DIR}/ubuntu-source-${disc}.iso"

    if [ ! -f "${output}" ]; then
        info "Recombining disc ${disc} first..."
        recombine_disc "${disc}"
    fi

    mkdir -p "${dest}"

    info "Extracting disc ${disc} to ${dest}..."

    # Use bsdtar or 7z for ISO extraction without mounting
    if command -v bsdtar >/dev/null 2>&1; then
        bsdtar -xf "${output}" -C "${dest}"
    elif command -v 7z >/dev/null 2>&1; then
        7z x -o"${dest}" "${output}" >/dev/null
    else
        # Fallback: mount temporarily
        local tmp_mount
        tmp_mount=$(mktemp -d)
        mount -o loop,ro "${output}" "${tmp_mount}"
        cp -a "${tmp_mount}"/. "${dest}"/
        umount "${tmp_mount}"
        rmdir "${tmp_mount}"
    fi

    echo "  Extracted: $(find "${dest}" -type f | wc -l) files"
}

# Verify ISOs
verify_isos() {
    info "Verifying recombined ISOs..."
    for disc in 1 2 3 4; do
        local output="${OUTPUT_DIR}/ubuntu-source-${disc}.iso"
        if [ -f "${output}" ]; then
            local sha
            sha=$(sha256sum "${output}" | awk '{print $1}')
            local size
            size=$(du -h "${output}" | awk '{print $1}')
            local valid=""
            file "${output}" | grep -q "ISO 9660" && valid="✓ valid" || valid="✗ INVALID"
            echo "  Disc ${disc}: ${size}  ${valid}  sha256:${sha:0:16}..."
        else
            echo "  Disc ${disc}: not recombined"
        fi
    done
}

# List disc contents
list_disc() {
    local disc="$1"
    local output="${OUTPUT_DIR}/ubuntu-source-${disc}.iso"

    if [ ! -f "${output}" ]; then
        info "Recombining disc ${disc} first..."
        recombine_disc "${disc}"
    fi

    if command -v isoinfo >/dev/null 2>&1; then
        isoinfo -l -i "${output}" 2>/dev/null | head -60
    elif command -v bsdtar >/dev/null 2>&1; then
        bsdtar -tf "${output}" 2>/dev/null | head -60
    else
        echo "  Install genisoimage or bsdtar to list ISO contents"
        echo "  Or mount with: sudo $(basename "$0") --mount ${disc}"
    fi
}

# Show status
show_status() {
    info "Source ISO Status"
    echo ""
    echo "  Split chunks (ubuntu.slaves.black/):"
    for disc in 1 2 3 4; do
        local disc_dir="${SOURCE_DIR}/${disc}"
        if [ -d "${disc_dir}" ]; then
            local count size
            count=$(ls -1 "${disc_dir}"/ubuntu_${disc}_* 2>/dev/null | wc -l)
            size=$(du -sh "${disc_dir}" 2>/dev/null | awk '{print $1}')
            echo "    Disc ${disc}: ${count} chunks, ${size}"
        fi
    done
    echo ""
    echo "  Recombined ISOs (build/source-isos/):"
    for disc in 1 2 3 4; do
        local output="${OUTPUT_DIR}/ubuntu-source-${disc}.iso"
        if [ -f "${output}" ]; then
            local size
            size=$(du -h "${output}" | awk '{print $1}')
            echo "    Disc ${disc}: ${output} (${size})"
        else
            echo "    Disc ${disc}: not recombined"
        fi
    done
    echo ""
    echo "  Mounted:"
    for disc in 1 2 3 4; do
        local mount_point="${MOUNT_BASE}/${disc}"
        if mountpoint -q "${mount_point}" 2>/dev/null; then
            echo "    Disc ${disc}: ${mount_point}"
        fi
    done
    echo ""
}

# Clean recombined ISOs
clean_isos() {
    info "Cleaning recombined ISOs..."
    for disc in 1 2 3 4; do
        local mount_point="${MOUNT_BASE}/${disc}"
        if mountpoint -q "${mount_point}" 2>/dev/null; then
            umount "${mount_point}" 2>/dev/null || true
        fi
        local output="${OUTPUT_DIR}/ubuntu-source-${disc}.iso"
        if [ -f "${output}" ]; then
            rm -f "${output}"
            echo "  Removed: ${output}"
        fi
    done
    rmdir "${OUTPUT_DIR}" 2>/dev/null || true
    echo "  Done."
}

# ============================================================
# Main
# ============================================================

ACTION="all"
DISC=""
EXTRACT_DIR=""

while [ $# -gt 0 ]; do
    case "$1" in
        --disc)     ACTION="disc"; shift; DISC="${1:-}" ;;
        --mount)    ACTION="mount"; shift; DISC="${1:-}" ;;
        --extract)  ACTION="extract"; shift; DISC="${1:-}"; shift; EXTRACT_DIR="${1:-}" ;;
        --verify)   ACTION="verify" ;;
        --list)     ACTION="list"; shift; DISC="${1:-}" ;;
        --status)   ACTION="status" ;;
        --clean)    ACTION="clean" ;;
        --help|-h)  usage ;;
        *)          die "Unknown option: $1" ;;
    esac
    shift
done

# Validate disc number where required
if [ -n "${DISC}" ]; then
    case "${DISC}" in
        1|2|3|4) ;;
        *) die "Invalid disc number: ${DISC} (must be 1-4)" ;;
    esac
fi

case "${ACTION}" in
    all)
        info "Recombining all Ubuntu 22.04.3 LTS source discs..."
        echo ""
        for disc in 1 2 3 4; do
            recombine_disc "${disc}"
        done
        echo ""
        info "All discs recombined. Output: ${OUTPUT_DIR}/"
        echo ""
        echo "  To mount: sudo $(basename "$0") --mount 1"
        echo "  To extract: $(basename "$0") --extract 1 ./source-disc-1/"
        echo "  To verify: $(basename "$0") --verify"
        ;;
    disc)
        [ -z "${DISC}" ] && die "--disc requires a disc number (1-4)"
        info "Recombining disc ${DISC}..."
        recombine_disc "${DISC}"
        ;;
    mount)
        [ -z "${DISC}" ] && die "--mount requires a disc number (1-4)"
        [ "$(id -u)" -ne 0 ] && die "Mounting requires root. Use: sudo $(basename "$0") --mount ${DISC}"
        mount_disc "${DISC}"
        ;;
    extract)
        [ -z "${DISC}" ] && die "--extract requires a disc number and directory"
        [ -z "${EXTRACT_DIR}" ] && die "--extract requires a destination directory"
        extract_disc "${DISC}" "${EXTRACT_DIR}"
        ;;
    verify)
        verify_isos
        ;;
    list)
        [ -z "${DISC}" ] && die "--list requires a disc number (1-4)"
        list_disc "${DISC}"
        ;;
    status)
        show_status
        ;;
    clean)
        clean_isos
        ;;
esac
