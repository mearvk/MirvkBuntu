#!/usr/bin/env bash
set -euo pipefail

# SPDX-License-Identifier: GPL-2.0
# Ubuntu Determinant GNOME Desktop installer.
# GNOME is the production default; MATE remains available separately.
# Ubuntu White Edition is an additive visual/icon overlay, not a GNOME fork.

if [ "$(id -u)" -ne 0 ]; then
  echo "ERROR: Must run as root (or inside chroot)." >&2
  exit 1
fi
command -v apt-get >/dev/null 2>&1 || { echo "ERROR: apt-get not found." >&2; exit 1; }
export DEBIAN_FRONTEND=noninteractive

GNOME_THEME="${GNOME_THEME:-ubuntu-white}"
UBUNTU_WHITE_ICONS="${UBUNTU_WHITE_ICONS:-1}"
UBUNTU_WHITE_CSS="${UBUNTU_WHITE_CSS:-1}"
THEME_INSTALL_URL="${UBUNTU_WHITE_THEME_INSTALL_URL:-https://raw.githubusercontent.com/mearvk/Ubuntu.Determinant.Beta.Restricted/main/scripts/install-ubuntu-white-theme.sh}"

case "${GNOME_THEME,,}" in
  ubuntu-white|white|determinant)
    APPLY_WHITE=1
    ;;
  stock|upstream)
    APPLY_WHITE=0
    ;;
  *)
    echo "ERROR: Unknown GNOME_THEME='${GNOME_THEME}'. Use ubuntu-white or stock." >&2
    exit 2
    ;;
esac

if [ ! -f /etc/apt/sources.list ] && [ ! -f /etc/apt/sources.list.d/ubuntu.sources ]; then
  cat > /etc/apt/sources.list <<'SOURCES'
deb http://archive.ubuntu.com/ubuntu noble main restricted universe multiverse
deb http://archive.ubuntu.com/ubuntu noble-updates main restricted universe multiverse
deb http://security.ubuntu.com/ubuntu noble-security main restricted universe multiverse
SOURCES
fi

apt-get update -qq

echo "=== Installing GNOME Desktop ==="
apt-get install -y --no-install-recommends \
  gnome-shell gnome-session mutter nautilus gnome-settings-daemon \
  gnome-control-center gnome-terminal gnome-system-monitor \
  gnome-software gnome-tweaks gnome-shell-extensions \
  gdm3 \
  xserver-xorg-core xserver-xorg-input-libinput \
  mesa-utils libgl1-mesa-dri dbus-x11 x11-xserver-utils x11-utils \
  fonts-ubuntu fonts-dejavu-core fonts-noto-core fonts-noto-color-emoji \
  network-manager network-manager-gnome \
  pipewire pipewire-pulse wireplumber pavucontrol \
  plymouth plymouth-theme-ubuntu-text

# GNOME uses GDM. Keep the display manager choice explicit so a previous
# LightDM/MATE installation does not silently remain the active session.
install -d /etc/systemd/system
printf '%s\n' '/usr/sbin/gdm3' > /etc/X11/default-display-manager
systemctl set-default graphical.target 2>/dev/null || \
  ln -sf /lib/systemd/system/graphical.target /etc/systemd/system/default.target
systemctl enable gdm3 2>/dev/null || \
  ln -sf /lib/systemd/system/gdm3.service /etc/systemd/system/display-manager.service
systemctl disable lightdm 2>/dev/null || true

install -d /etc/gdm3
cat > /etc/gdm3/custom.conf <<'GDM'
[daemon]
# Ubuntu Determinant defaults to GNOME; users may select another installed
# session from the GDM session chooser.
GDM

glib-compile-schemas /usr/share/glib-2.0/schemas/ 2>/dev/null || true

if [ "$APPLY_WHITE" = "1" ]; then
  echo "=== Applying Ubuntu White Edition overlay ==="
  if ! command -v curl >/dev/null 2>&1; then
    echo "ERROR: curl is required for the Ubuntu White Edition overlay." >&2
    exit 1
  fi
  curl --fail --location --retry 3 -sS -o /tmp/install-ubuntu-white-theme.sh "$THEME_INSTALL_URL"
  chmod 755 /tmp/install-ubuntu-white-theme.sh
  UBUNTU_WHITE_ICONS="$UBUNTU_WHITE_ICONS" UBUNTU_WHITE_CSS="$UBUNTU_WHITE_CSS" \
    /tmp/install-ubuntu-white-theme.sh
  rm -f /tmp/install-ubuntu-white-theme.sh
fi

# Keep existing Determinant wallpaper assets available to GNOME.
if [ -d /usr/share/backgrounds/galactic-cherry ]; then
  install -d /usr/share/backgrounds/ubuntu-determinant
  cp -a /usr/share/backgrounds/galactic-cherry/. /usr/share/backgrounds/ubuntu-determinant/ 2>/dev/null || true
fi

echo "=== GNOME Desktop installed ==="
echo "  Shell:           GNOME Shell"
echo "  WM/compositor:   Mutter"
echo "  Files:           Nautilus"
echo "  Login:           GDM3"
echo "  Theme:           ${GNOME_THEME}"
echo "  White icons:     ${UBUNTU_WHITE_ICONS}"
echo "  White CSS:       ${UBUNTU_WHITE_CSS}"
