#!/bin/bash
# gen-iso.sh - Generate bootable ISO for Galactic Cherry Marvell Edition 98
#
# Produces a hybrid ISO image bootable from:
#   - CD/DVD (El Torito)
#   - USB drive (isohybrid MBR)
#   - UEFI systems
#
# Prerequisites: xorriso, grub-mkrescue, grub-pc-bin, grub-efi-amd64-bin, mtools
#
# Copyright (C) 2026 MEARVK LLC

set -e

ROOTFS_DIR="${1:-build/rootfs}"
OUTPUT="${2:-build/galactic-cherry-98.iso}"
EDITION="Galactic Cherry Marvell Edition 98"
KERNEL_VER="5.15.204"

# Validate inputs
if [ ! -d "$ROOTFS_DIR" ]; then
    echo "ERROR: Rootfs directory not found: $ROOTFS_DIR"
    echo "Run 'make rootfs-full' first to assemble the root filesystem."
    exit 1
fi

if [ ! -f "$ROOTFS_DIR/boot/vmlinuz-$KERNEL_VER" ]; then
    echo "ERROR: Kernel not found in $ROOTFS_DIR/boot/"
    echo "Run 'make kernel-install' first."
    exit 1
fi

# Check for required tools
for tool in xorriso mksquashfs grub-mkrescue; do
    if ! command -v "$tool" &>/dev/null; then
        echo "ERROR: Required tool '$tool' not found."
        echo "Install with: sudo apt install xorriso squashfs-tools grub-pc-bin grub-efi-amd64-bin mtools"
        exit 1
    fi
done

echo "╔══════════════════════════════════════════════════════════════╗"
echo "║  Building ISO: $EDITION"
echo "╚══════════════════════════════════════════════════════════════╝"
echo ""

# Create ISO staging area
ISO_DIR=$(mktemp -d)
trap "rm -rf $ISO_DIR" EXIT

mkdir -p "$ISO_DIR"/{boot/grub,live,isolinux,.disk}

# ─────────────────────────────────────────────────────────────────────────────
# 1. Create squashfs of the root filesystem (live system image)
# ─────────────────────────────────────────────────────────────────────────────
echo "[1/5] Creating squashfs filesystem image..."
mksquashfs "$ROOTFS_DIR" "$ISO_DIR/live/filesystem.squashfs" \
    -comp xz -Xbcj x86 -b 1M \
    -e boot/vmlinuz-* boot/initramfs* boot/grub \
    -no-progress 2>/dev/null

SQFS_SIZE=$(du -h "$ISO_DIR/live/filesystem.squashfs" | cut -f1)
echo "  Squashfs: $SQFS_SIZE"

# ─────────────────────────────────────────────────────────────────────────────
# 2. Copy kernel and initramfs to ISO boot directory
# ─────────────────────────────────────────────────────────────────────────────
echo "[2/5] Copying kernel and initramfs..."
cp "$ROOTFS_DIR/boot/vmlinuz-$KERNEL_VER" "$ISO_DIR/boot/vmlinuz"

if [ -f "$ROOTFS_DIR/boot/initramfs.img" ]; then
    cp "$ROOTFS_DIR/boot/initramfs.img" "$ISO_DIR/boot/initrd.img"
elif [ -f "build/initramfs.img" ]; then
    cp "build/initramfs.img" "$ISO_DIR/boot/initrd.img"
else
    echo "  WARNING: No initramfs found. Generating minimal one..."
    bash scripts/gen-initramfs.sh "" "$ROOTFS_DIR" "$ISO_DIR/boot/initrd.img"
fi

# ─────────────────────────────────────────────────────────────────────────────
# 3. Create GRUB configuration for the live ISO
# ─────────────────────────────────────────────────────────────────────────────
echo "[3/5] Creating GRUB boot configuration..."
cat > "$ISO_DIR/boot/grub/grub.cfg" << 'EOF'
# GRUB Configuration - Galactic Cherry Marvell Edition 98
# Live/Install Media

set default=0
set timeout=10

# Theme colors
set menu_color_normal=light-gray/black
set menu_color_highlight=magenta/black

insmod all_video
insmod gfxterm
set gfxmode=auto
terminal_output gfxterm

menuentry "Galactic Cherry Marvell Edition 98 - Live System" {
    linux /boot/vmlinuz boot=live toram quiet splash
    initrd /boot/initrd.img
}

# ── Edition install entries ──────────────────────────────────────────────
# Both editions boot the SAME shared kernel/initrd/squashfs. The only
# difference is the edition= marker on the kernel command line, which the
# live installer (galactic-cherry-installer) reads from /proc/cmdline to
# preset the desktop and optional components:
#   edition=white -> Ubuntu White (GNOME + Ubuntu White theme/icon overlay)
#   edition=mate  -> MATE (Galactic Cherry Red theme)
menuentry "Install Ubuntu White Edition" {
    linux /boot/vmlinuz boot=live toram installer edition=white quiet splash
    initrd /boot/initrd.img
}

menuentry "Install MATE Edition (Galactic Cherry Red)" {
    linux /boot/vmlinuz boot=live toram installer edition=mate quiet splash
    initrd /boot/initrd.img
}

menuentry "Install to Disk (choose edition in installer)" {
    linux /boot/vmlinuz boot=live toram installer
    initrd /boot/initrd.img
}

menuentry "Galactic Cherry Marvell Edition 98 - Live (Verbose)" {
    linux /boot/vmlinuz boot=live toram loglevel=7
    initrd /boot/initrd.img
}

menuentry "Galactic Cherry Marvell Edition 98 - Live (Safe Graphics)" {
    linux /boot/vmlinuz boot=live toram nomodeset
    initrd /boot/initrd.img
}

menuentry "Boot from first hard disk" {
    chainloader (hd0)+1
}
EOF

# ─────────────────────────────────────────────────────────────────────────────
# 4. Write ISO metadata
# ─────────────────────────────────────────────────────────────────────────────
echo "[4/5] Writing ISO metadata..."

echo "Galactic Cherry Marvell Edition 98" > "$ISO_DIR/.disk/info"
echo "Ubuntu Determinant Alpha RS" >> "$ISO_DIR/.disk/info"
echo "Kernel: Linux $KERNEL_VER" >> "$ISO_DIR/.disk/info"
echo "Built: $(date -Iseconds)" >> "$ISO_DIR/.disk/info"

# Create filesystem manifest
if command -v dpkg-query &>/dev/null && [ -f "$ROOTFS_DIR/var/lib/dpkg/status" ]; then
    chroot "$ROOTFS_DIR" dpkg-query -W --showformat='${Package} ${Version}\n' \
        > "$ISO_DIR/live/filesystem.manifest" 2>/dev/null || true
fi

# ─────────────────────────────────────────────────────────────────────────────
# 5. Generate the ISO image with grub-mkrescue
# ─────────────────────────────────────────────────────────────────────────────
echo "[5/5] Generating hybrid ISO image..."

grub-mkrescue -o "$OUTPUT" "$ISO_DIR" \
    --product-name "Ubuntu Determinant Alpha RS" \
    --product-version "98" \
    -- -volid "GALACTIC_CHERRY_98" \
    2>/dev/null

# Verify
if [ -f "$OUTPUT" ]; then
    ISO_SIZE=$(du -h "$OUTPUT" | cut -f1)
    echo ""
    echo "╔══════════════════════════════════════════════════════════════╗"
    echo "║  ISO CREATED SUCCESSFULLY                                    ║"
    echo "╚══════════════════════════════════════════════════════════════╝"
    echo ""
    echo "  File:     $OUTPUT"
    echo "  Size:     $ISO_SIZE"
    echo "  Label:    GALACTIC_CHERRY_98"
    echo "  Boot:     BIOS (MBR) + UEFI"
    echo ""
    echo "  Write to USB:"
    echo "    sudo dd if=$OUTPUT of=/dev/sdX bs=4M status=progress && sync"
    echo ""
    echo "  Or burn to DVD:"
    echo "    xorriso -as cdrecord -v dev=/dev/sr0 $OUTPUT"
    echo ""
else
    echo "ERROR: ISO generation failed."
    exit 1
fi
