#!/bin/bash
# =============================================================================
# build-kernel-initramfs.sh — Build the kernel, install its modules into the
#                             rootfs, then (re)generate the initramfs.
#
# Ubuntu Determinant Alpha RS — Galactic Cherry Marvell Edition 98
# Copyright (C) 2026 MEARVK LLC
#
# This wraps the ordered sequence required for a bootable initramfs:
#
#   make kernel          # build bzImage + modules in the kernel tree
#   make kernel-install  # install modules into build/rootfs/lib/modules/<ver>
#   make initramfs       # gen-initramfs.sh copies those .ko into the initramfs
#
# The ordering matters: gen-initramfs.sh only *copies* modules from the rootfs
# (it never builds them). If 'make kernel-install' has not populated
# build/rootfs/lib/modules/<ver>/ first, the initramfs is built with zero
# modules ("Modules included: 0") and will fail to mount root at boot.
#
# Usage:
#   ./scripts/build-kernel-initramfs.sh              # run all three phases
#   ./scripts/build-kernel-initramfs.sh --dry-run    # show commands only
#   ./scripts/build-kernel-initramfs.sh --skip-kernel-build   # modules already built
#   ./scripts/build-kernel-initramfs.sh -h|--help
# =============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

# Kernel version and rootfs location, kept in sync with the Makefile
# (KERNEL_VER, ROOTFS_DIR). Override via the environment if needed.
KERNEL_VER="${KERNEL_VER:-5.15.204}"
ROOTFS_DIR="${ROOTFS_DIR:-$PROJECT_DIR/build/rootfs}"

DRY_RUN=0
SKIP_KERNEL_BUILD=0

# Colors (disabled when stdout is not a terminal)
if [ -t 1 ]; then
    RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; CYAN='\033[0;36m'; NC='\033[0m'
else
    RED=''; GREEN=''; YELLOW=''; CYAN=''; NC=''
fi

for arg in "$@"; do
    case "$arg" in
        --dry-run)           DRY_RUN=1 ;;
        --skip-kernel-build) SKIP_KERNEL_BUILD=1 ;;
        -h|--help)
            sed -n '2,32p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
            exit 0
            ;;
        *)
            echo "Unknown argument: $arg (try --help)" >&2
            exit 1
            ;;
    esac
done

fail() { echo -e "${RED}ERROR:${NC} $*" >&2; exit 1; }

run() {
    echo -e "${GREEN}[RUN]${NC} $*"
    if [ "$DRY_RUN" -eq 0 ]; then
        "$@"
    fi
}

count_rootfs_modules() {
    find "$ROOTFS_DIR/lib/modules" -name '*.ko' 2>/dev/null | wc -l | tr -d ' '
}

echo -e "${CYAN}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║  Kernel + Initramfs build (Galactic Cherry Marvell 98)     ║${NC}"
echo -e "${CYAN}╚══════════════════════════════════════════════════════════════╝${NC}"
echo "  Project:    $PROJECT_DIR"
echo "  Kernel ver: $KERNEL_VER"
echo "  Rootfs:     $ROOTFS_DIR"
[ "$DRY_RUN" -eq 1 ] && echo -e "  ${YELLOW}(dry run — no commands executed)${NC}"
echo ""

cd "$PROJECT_DIR"

# --- Phase 1: build the kernel (bzImage + modules) ---------------------------
if [ "$SKIP_KERNEL_BUILD" -eq 1 ]; then
    echo -e "${YELLOW}[SKIP]${NC} make kernel (--skip-kernel-build)"
else
    echo -e "${CYAN}── Phase 1/3: make kernel ──${NC}"
    run make kernel
fi
echo ""

# --- Phase 2: install modules + kernel into the rootfs -----------------------
echo -e "${CYAN}── Phase 2/3: make kernel-install ──${NC}"
run make kernel-install
echo ""

# Verify modules actually landed in the rootfs before building the initramfs,
# so the "Modules included: 0" failure surfaces here instead of at boot time.
if [ "$DRY_RUN" -eq 0 ]; then
    MOD_DIR="$ROOTFS_DIR/lib/modules/$KERNEL_VER"
    if [ ! -d "$MOD_DIR" ]; then
        # Fall back to whatever version dir modules_install actually created.
        ALT="$(find "$ROOTFS_DIR/lib/modules" -maxdepth 1 -mindepth 1 -type d 2>/dev/null | head -n1)"
        [ -n "$ALT" ] && fail "expected module dir $MOD_DIR not found, but $ALT exists — KERNEL_VER may not match the kernel's release string (CONFIG_LOCALVERSION?). Set KERNEL_VER=$(basename "$ALT") and rerun."
        fail "no module directory under $ROOTFS_DIR/lib/modules — 'make kernel-install' did not install modules."
    fi
    NMOD="$(count_rootfs_modules)"
    if [ "$NMOD" -eq 0 ]; then
        fail "0 kernel modules installed in $MOD_DIR — cannot build a bootable initramfs."
    fi
    echo -e "${GREEN}[OK]${NC} $NMOD kernel module(s) present in rootfs."
fi
echo ""

# --- Phase 3: (re)generate the initramfs -------------------------------------
echo -e "${CYAN}── Phase 3/3: make initramfs ──${NC}"
run make initramfs
echo ""

echo -e "${GREEN}Done.${NC}"
if [ "$DRY_RUN" -eq 0 ]; then
    echo "  Initramfs: $PROJECT_DIR/build/initramfs.img"
    echo "  Next: 'make grub' then 'make iso' (or 'make rootfs-full')."
fi
