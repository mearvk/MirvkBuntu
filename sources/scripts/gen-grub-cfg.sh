#!/bin/bash
# gen-grub-cfg.sh - Generate GRUB configuration for Galactic Cherry Marvell Edition 98
#
# Copyright (C) 2026 MEARVK LLC

set -e

ROOTFS_DIR="${1:-build/rootfs}"
ROOT_UUID="${2:-PLACEHOLDER-UUID}"
OUTPUT="$ROOTFS_DIR/boot/grub/grub.cfg"

mkdir -p "$(dirname "$OUTPUT")"

cat > "$OUTPUT" << EOF
# GRUB Configuration
# Galactic Cherry Marvell Edition 98
# Ubuntu Determinant Alpha RS - Kernel 5.15.204
# Generated: $(date -Iseconds)

set default=0
set timeout=5

# Colors
set menu_color_normal=light-gray/black
set menu_color_highlight=magenta/black

insmod gfxterm
insmod vbe
set gfxmode=auto
terminal_output gfxterm

menuentry "Galactic Cherry Marvell Edition 98" {
    linux /boot/vmlinuz-5.15.204 root=UUID=$ROOT_UUID ro quiet splash loglevel=3
    initrd /boot/initramfs.img
}

menuentry "Galactic Cherry Marvell Edition 98 (Verbose)" {
    linux /boot/vmlinuz-5.15.204 root=UUID=$ROOT_UUID ro loglevel=7
    initrd /boot/initramfs.img
}

menuentry "Galactic Cherry Marvell Edition 98 (Recovery)" {
    linux /boot/vmlinuz-5.15.204 root=UUID=$ROOT_UUID ro single
    initrd /boot/initramfs.img
}

menuentry "Galactic Cherry Marvell Edition 98 (Emergency Shell)" {
    linux /boot/vmlinuz-5.15.204 root=UUID=$ROOT_UUID ro init=/bin/sh
    initrd /boot/initramfs.img
}
EOF

echo "GRUB config written to: $OUTPUT"
if [ "$ROOT_UUID" = "PLACEHOLDER-UUID" ]; then
    echo "NOTE: Replace PLACEHOLDER-UUID with actual root partition UUID"
    echo "  Use: blkid /dev/sdXN | grep -oP 'UUID=\"\\K[^\"]+'"
fi
