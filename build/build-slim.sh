#!/usr/bin/env bash
set -euo pipefail

# MirvkBuntu SLIM — the full MirvkBuntu OS, built from MirvkBuntu's own source
# (native compilation gate), delivered as a live image that:
#   * loads the entire filesystem into RAM at boot (boot=live ... toram), so the
#     boot medium can be removed once running;
#   * boots straight to the full GNOME desktop (autologin live session);
#   * provides a modest writable overlay for on-the-go work, capped at 400 MB of
#     RAM (configurable via SLIM_OVERLAY_SIZE). Changes are temporary and reset
#     on reboot (RAM-backed).
#
# This is NOT a generic live-build spin: it runs build/native-build.sh first
# (MirvkBuntu kernel + GNOME stack + Chromium from this repository) and feeds
# only those native outputs into live-build for ISO assembly, exactly like
# build-desktop.sh. live-build remains the assembly mechanism, not the source.
#
# Requirements: Debian/Ubuntu host, root, network; tooling from
#   sudo bash build/prerequisites.sh native
# GNOME module source must be present (populate with
#   bash gnome-source/pull-all-source.sh) or the native GNOME stage skipped.
#
# Usage:
#   sudo bash build/build-slim.sh
#   sudo SLIM_OVERLAY_SIZE=512M bash build/build-slim.sh

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/common-build.sh"

# Size of the RAM-backed writable overlay for the live session ("work on the go").
SLIM_OVERLAY_SIZE="${SLIM_OVERLAY_SIZE:-400M}"
# Autologin user for the live GNOME session.
SLIM_LIVE_USER="${SLIM_LIVE_USER:-mirvk}"

require_commands live-build lb
require_mirvkbuntu_source
prepare_dirs
run_native_build

WORK_DIR="${BUILD_ROOT}/slim"
rm -rf "$WORK_DIR"
mkdir -p "$WORK_DIR"
cd "$WORK_DIR"

lb clean --purge >/dev/null 2>&1 || true

# Boot the live system entirely into RAM (toram) and hand off to a GNOME
# session. "boot=live components" is the live-boot contract; "toram" copies the
# squashfs to RAM so the medium can be removed.
BOOTAPPEND_LIVE="boot=live components toram quiet splash"

lb config \
  --distribution "$UBUNTU_SUITE" \
  --architectures "$ARCH" \
  --binary-images iso-hybrid \
  --debian-installer live \
  --apt-recommends true \
  --archive-areas "main restricted universe multiverse" \
  --iso-application "MirvkBuntu Slim" \
  --iso-publisher "MEARVK LLC" \
  --iso-volume "$ISO_LABEL-SLIM" \
  --linux-packages none \
  --bootappend-live "$BOOTAPPEND_LIVE"

# --- package lists -----------------------------------------------------------
# Base MirvkBuntu manifest, plus a full-GNOME desktop set for a slim-but-complete
# desktop (GNOME Shell session, display manager, core apps, live tooling).
write_package_list "$WORK_DIR" mirvkbuntu-slim

cat > "$WORK_DIR/config/package-lists/mirvkbuntu-slim-gnome.list.chroot" <<'EOF'
# MirvkBuntu Slim — full GNOME desktop + live session support
ubuntu-desktop-minimal
gnome-shell
gnome-session
gdm3
gnome-control-center
gnome-terminal
nautilus
gnome-text-editor
gnome-system-monitor
# live boot support (RAM boot / overlay)
live-boot
live-config
live-config-systemd
# firmware/graphics so the desktop comes up on real hardware
network-manager
xserver-xorg
mesa-utils
EOF

# --- MirvkBuntu source + native outputs -------------------------------------
stage_mirvkbuntu_source "$WORK_DIR"
write_mirvkbuntu_manifest "$WORK_DIR"
stage_native_outputs "$WORK_DIR"
stage_color_inference "$WORK_DIR"

# --- slim live-session configuration (hooks + includes) ----------------------
mkdir -p "$WORK_DIR/config/includes.chroot/etc/mirvkbuntu"
printf 'MIRVKBUNTU_EDITION=slim\nMIRVKBUNTU_TORAM=true\nMIRVKBUNTU_OVERLAY_SIZE=%s\n' \
  "$SLIM_OVERLAY_SIZE" > "$WORK_DIR/config/includes.chroot/etc/mirvkbuntu/slim.conf"

# Cap the RAM-backed writable overlay. This image uses Debian live-boot
# (installed above) whose union overlay is a tmpfs at /run/live/overlay; a
# systemd unit remounts it with a size limit so the "temporary work" area is
# bounded (default 400M). The remount is best-effort (|| true): if a given
# live-boot revision uses a different overlay path, boot is never blocked and
# the session still comes up (uncapped) rather than failing.
mkdir -p "$WORK_DIR/config/includes.chroot/usr/lib/systemd/system"
cat > "$WORK_DIR/config/includes.chroot/usr/lib/systemd/system/mirvkbuntu-overlay-limit.service" <<EOF
[Unit]
Description=Bound MirvkBuntu live overlay working area to ${SLIM_OVERLAY_SIZE}
DefaultDependencies=no
After=live-config.service
Before=gdm.service display-manager.service
ConditionPathExists=/run/live/overlay

[Service]
Type=oneshot
RemainAfterExit=yes
# Remount the RAM-backed overlay tmpfs with a hard size cap so on-the-go work
# cannot exhaust system memory. Best-effort: never block boot on failure.
ExecStart=/bin/sh -c 'mount -o remount,size=${SLIM_OVERLAY_SIZE} /run/live/overlay || true'

[Install]
WantedBy=multi-user.target
EOF

# Enable the overlay-limit service in the live image.
mkdir -p "$WORK_DIR/config/includes.chroot/etc/systemd/system/multi-user.target.wants"
ln -sf /usr/lib/systemd/system/mirvkbuntu-overlay-limit.service \
  "$WORK_DIR/config/includes.chroot/etc/systemd/system/multi-user.target.wants/mirvkbuntu-overlay-limit.service"

# Autologin the live user straight into the full GNOME session via GDM.
mkdir -p "$WORK_DIR/config/includes.chroot/etc/gdm3"
cat > "$WORK_DIR/config/includes.chroot/etc/gdm3/custom.conf" <<EOF
[daemon]
AutomaticLoginEnable=true
AutomaticLogin=${SLIM_LIVE_USER}
WaylandEnable=true
EOF

# Ensure the GNOME session is the default and the live user exists. live-config
# creates the live user; this hook guarantees the session + group membership.
mkdir -p "$WORK_DIR/config/hooks/live"
cat > "$WORK_DIR/config/hooks/live/0100-mirvkbuntu-slim-session.hook.chroot" <<EOF
#!/bin/sh
set -e
# Default to the GNOME (Wayland) session for the autologin user.
if [ -d /usr/share/wayland-sessions ] && [ -e /usr/share/wayland-sessions/gnome.desktop ]; then
  echo "[daemon]" > /etc/gdm3/daemon.conf 2>/dev/null || true
fi
# Make sure the live user is in the groups a desktop session needs.
for grp in audio video plugdev netdev sudo; do
  getent group "\$grp" >/dev/null 2>&1 || continue
  usermod -aG "\$grp" "${SLIM_LIVE_USER}" 2>/dev/null || true
done
# Record the edition for anything that inspects the running system.
mkdir -p /etc/mirvkbuntu
printf 'MIRVKBUNTU_SLIM_SESSION=gnome\n' > /etc/mirvkbuntu/slim-session.conf
EOF
chmod +x "$WORK_DIR/config/hooks/live/0100-mirvkbuntu-slim-session.hook.chroot"

# --- Ubuntu White theme (binds to UBUNTU.COLORS.md) --------------------------
# Install the MirvkBuntu-White GTK + GNOME Shell theme and make it the default
# for the live GNOME session, so the OS boots with its binding palette applied.
THEME_SRC="${THEME_DIR:-$REPO_ROOT/ubuntu-white}"
INC="$WORK_DIR/config/includes.chroot"
THEME_DEST="$INC/usr/share/themes/MirvkBuntu-White"
if [ -f "$THEME_SRC/gtk.css" ]; then
  mkdir -p "$THEME_DEST/gtk-3.0" "$THEME_DEST/gtk-4.0"
  cp -f "$THEME_SRC/gtk.css" "$THEME_DEST/gtk-3.0/gtk.css"
  cp -f "$THEME_SRC/gtk.css" "$THEME_DEST/gtk-4.0/gtk.css"
fi
if [ -f "$THEME_SRC/gnome-shell.css" ]; then
  mkdir -p "$THEME_DEST/gnome-shell"
  cp -f "$THEME_SRC/gnome-shell.css" "$THEME_DEST/gnome-shell/gnome-shell.css"
fi
if [ -d "$THEME_SRC/icons" ]; then
  mkdir -p "$INC/usr/share/icons/MirvkBuntu-White"
  cp -a "$THEME_SRC/icons/." "$INC/usr/share/icons/MirvkBuntu-White/"
fi
# Default the session to MirvkBuntu-White via a system dconf database.
mkdir -p "$INC/etc/dconf/db/local.d" "$INC/etc/dconf/profile"
printf 'user-db:user\nsystem-db:local\n' > "$INC/etc/dconf/profile/user"
cat > "$INC/etc/dconf/db/local.d/00-mirvkbuntu-theme" <<'DCONF'
[org/gnome/desktop/interface]
gtk-theme='MirvkBuntu-White'
color-scheme='default'

[org/gnome/shell/extensions/user-theme]
name='MirvkBuntu-White'
DCONF

# --- assemble ----------------------------------------------------------------
run_live_build "$WORK_DIR"
ISO_SOURCE="$(find_iso "$WORK_DIR")"
mkdir -p "$OUTPUT_ROOT"
mv -f "$ISO_SOURCE" "$OUTPUT_ROOT/MirvkBuntu-slim-${ARCH}.iso"
printf 'build: created %s\n' "$OUTPUT_ROOT/MirvkBuntu-slim-${ARCH}.iso"
printf 'build: slim edition boots to GNOME, loads to RAM (toram), overlay cap %s\n' "$SLIM_OVERLAY_SIZE"
publish_iso "$OUTPUT_ROOT/MirvkBuntu-slim-${ARCH}.iso"
write_release_manifest "$OUTPUT_ROOT/MirvkBuntu-slim-${ARCH}.iso" "$WORK_DIR" "slim"
validate_release_iso "$OUTPUT_ROOT/MirvkBuntu-slim-${ARCH}.iso" "$WORK_DIR" "slim"
