#!/bin/bash
# gen-initramfs.sh - Generate initramfs for Galactic Cherry Marvell Edition 98
#
# Copyright (C) 2026 MEARVK LLC

set -e

KERNEL_DIR="${1:-kernels/linux-5.15.204/linux-5.15.204}"
ROOTFS_DIR="${2:-build/rootfs}"
OUTPUT="${3:-build/initramfs.img}"

INITRAMFS_DIR=$(mktemp -d)
trap "rm -rf $INITRAMFS_DIR" EXIT

echo "=== Generating initramfs for Galactic Cherry Marvell Edition 98 ==="

# Create directory structure
mkdir -p "$INITRAMFS_DIR"/{bin,sbin,etc,proc,sys,dev,tmp,lib/modules,usr/bin,usr/sbin,newroot}

# Copy busybox or minimal init tools from rootfs
if [ -f "$ROOTFS_DIR/bin/busybox" ]; then
    cp "$ROOTFS_DIR/bin/busybox" "$INITRAMFS_DIR/bin/"
    for cmd in sh mount umount switch_root mkdir cat echo ls sleep insmod; do
        ln -sf busybox "$INITRAMFS_DIR/bin/$cmd"
    done
elif command -v busybox &>/dev/null; then
    cp "$(command -v busybox)" "$INITRAMFS_DIR/bin/"
    for cmd in sh mount umount switch_root mkdir cat echo ls sleep insmod; do
        ln -sf busybox "$INITRAMFS_DIR/bin/$cmd"
    done
else
    echo "WARNING: busybox not found. Initramfs will need manual init setup."
    # Copy minimal binaries from host
    for bin in /bin/sh /bin/mount /bin/umount /bin/mkdir /bin/cat /bin/echo /bin/ls /bin/sleep; do
        [ -f "$bin" ] && cp "$bin" "$INITRAMFS_DIR/bin/"
    done
fi

# Copy kernel modules
KERNEL_VER="5.15.204"
MOD_DIR="$ROOTFS_DIR/lib/modules/$KERNEL_VER"
if [ -d "$MOD_DIR" ]; then
    mkdir -p "$INITRAMFS_DIR/lib/modules/$KERNEL_VER"
    find "$MOD_DIR" -name '*.ko' | while read mod; do
        modname=$(basename "$mod")
        case "$modname" in
            # Custom extensions
            eperm.ko|negamane.ko|usbswap.ko|usbdma_fast.ko|\
            white_ethics.ko|cpuboost.ko|user_ko.ko|epmp.ko|hpm.ko)
                cp "$mod" "$INITRAMFS_DIR/lib/modules/$KERNEL_VER/"
                ;;
            # Storage drivers needed for root mount
            ahci.ko|libahci.ko|sd_mod.ko|ext4.ko|mbcache.ko|jbd2.ko|\
            crc32c_generic.ko|crc32c_intel.ko|scsi_mod.ko|libata.ko)
                cp "$mod" "$INITRAMFS_DIR/lib/modules/$KERNEL_VER/"
                ;;
            # USB for early device detection
            xhci-hcd.ko|xhci-pci.ko|usb-storage.ko|ehci-hcd.ko|ehci-pci.ko)
                cp "$mod" "$INITRAMFS_DIR/lib/modules/$KERNEL_VER/"
                ;;
        esac
    done
fi

# Create init script
cat > "$INITRAMFS_DIR/init" << 'INIT_EOF'
#!/bin/sh
# Galactic Cherry Marvell Edition 98 - Early Init

mount -t proc none /proc
mount -t sysfs none /sys
mount -t devtmpfs none /dev

echo ""
echo "  ╔══════════════════════════════════════════════╗"
echo "  ║  Galactic Cherry Marvell Edition 98         ║"
echo "  ║  Ubuntu Determinant Alpha RS                ║"
echo "  ║  Kernel 5.15.204                            ║"
echo "  ╚══════════════════════════════════════════════╝"
echo ""

# Load custom kernel modules
KVER="5.15.204"
MODDIR="/lib/modules/$KVER"
if [ -d "$MODDIR" ]; then
    # Load storage drivers first
    for mod in scsi_mod libata ahci libahci sd_mod crc32c_generic crc32c_intel ext4 mbcache jbd2; do
        [ -f "$MODDIR/${mod}.ko" ] && insmod "$MODDIR/${mod}.ko" 2>/dev/null
    done
    # Load USB
    for mod in xhci-hcd xhci-pci ehci-hcd ehci-pci usb-storage; do
        [ -f "$MODDIR/${mod}.ko" ] && insmod "$MODDIR/${mod}.ko" 2>/dev/null
    done
    # Load custom extensions
    for mod in eperm negamane white_ethics cpuboost user_ko epmp hpm usbswap usbdma_fast; do
        if [ -f "$MODDIR/${mod}.ko" ]; then
            insmod "$MODDIR/${mod}.ko" 2>/dev/null && echo "  [+] $mod"
        fi
    done
fi

# Parse kernel command line for root device
ROOT_DEV=""
ROOT_FSTYPE="ext4"
INIT="/sbin/init"
for arg in $(cat /proc/cmdline); do
    case "$arg" in
        root=*)      ROOT_DEV="${arg#root=}" ;;
        rootfstype=*) ROOT_FSTYPE="${arg#rootfstype=}" ;;
        init=*)      INIT="${arg#init=}" ;;
    esac
done

# Handle UUID root specification
case "$ROOT_DEV" in
    UUID=*|PARTUUID=*)
        # Wait for /dev/disk/by-uuid to populate
        sleep 1
        UUID_VAL="${ROOT_DEV#*=}"
        if [ -L "/dev/disk/by-uuid/$UUID_VAL" ]; then
            ROOT_DEV=$(readlink -f "/dev/disk/by-uuid/$UUID_VAL")
        elif [ -L "/dev/disk/by-partuuid/$UUID_VAL" ]; then
            ROOT_DEV=$(readlink -f "/dev/disk/by-partuuid/$UUID_VAL")
        fi
        ;;
esac

if [ -n "$ROOT_DEV" ]; then
    # Wait for root device
    count=0
    while [ ! -b "$ROOT_DEV" ] && [ $count -lt 50 ]; do
        sleep 0.1
        count=$((count + 1))
    done

    if [ -b "$ROOT_DEV" ]; then
        echo "  Mounting root: $ROOT_DEV ($ROOT_FSTYPE)"
        mount -t "$ROOT_FSTYPE" -o ro "$ROOT_DEV" /newroot
        
        # Verify init exists
        if [ -x "/newroot$INIT" ]; then
            umount /proc /sys /dev 2>/dev/null
            exec switch_root /newroot "$INIT"
        else
            echo "ERROR: $INIT not found on root filesystem"
            exec /bin/sh
        fi
    else
        echo "ERROR: Root device $ROOT_DEV not found after 5s"
        exec /bin/sh
    fi
else
    echo "ERROR: No root= parameter on kernel command line"
    echo "Dropping to emergency shell..."
    exec /bin/sh
fi
INIT_EOF
chmod 755 "$INITRAMFS_DIR/init"

# Create the cpio archive
mkdir -p "$(dirname "$OUTPUT")"
(cd "$INITRAMFS_DIR" && find . | cpio -H newc -o --quiet | gzip -9) > "$OUTPUT"

SIZE=$(du -h "$OUTPUT" | cut -f1)
echo ""
echo "Initramfs created: $OUTPUT ($SIZE)"
echo "Modules included: $(find "$INITRAMFS_DIR/lib/modules" -name '*.ko' 2>/dev/null | wc -l)"
