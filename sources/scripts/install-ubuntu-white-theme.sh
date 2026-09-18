#!/usr/bin/env bash
set -euo pipefail

# Ubuntu White Edition is an additive visual overlay. It does not replace
# GNOME, Mutter, Nautilus, or the upstream Ubuntu source packages.
#
# Environment:
#   UBUNTU_WHITE_ICONS=1|0       Install set-002 icons (default: 1)
#   UBUNTU_WHITE_CSS=1|0         Install GTK/GNOME Shell CSS (default: 1)
#   UBUNTU_WHITE_ICON_BASE_URL   Override the set-002 raw source URL

if [ "$(id -u)" -ne 0 ]; then
  echo "ERROR: Ubuntu White theme installer must run as root." >&2
  exit 1
fi
command -v curl >/dev/null 2>&1 || { echo "ERROR: curl is required." >&2; exit 1; }

ROOT="/"
RAW_BASE="${UBUNTU_WHITE_RAW_BASE_URL:-https://raw.githubusercontent.com/mearvk/Ubuntu.Determinant.Beta.Restricted/main}"
ICON_BASE="${UBUNTU_WHITE_ICON_BASE_URL:-${RAW_BASE}/images/desktop-icons/set-002}"
INSTALL_ICONS="${UBUNTU_WHITE_ICONS:-1}"
INSTALL_CSS="${UBUNTU_WHITE_CSS:-1}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

if [ "$INSTALL_ICONS" = "1" ]; then
  THEME="/usr/share/icons/Ubuntu-White"
  mkdir -p "$THEME/48x48/places" "$THEME/48x48/apps" "$THEME/48x48/mimetypes" "$THEME/48x48/status"
  for n in $(seq -w 1 12); do
    curl --fail --location --retry 3 -sS -o "$TMP/icon-${n}.png" "$ICON_BASE/icon-${n}.png"
  done

  # set-002 is the approved Ubuntu White Edition quality reference set.
  # The first two assets provide the default folder/folder-open treatment;
  # the complete set remains available as named application artwork.
  install -m 644 "$TMP/icon-001.png" "$THEME/48x48/places/folder.png"
  install -m 644 "$TMP/icon-002.png" "$THEME/48x48/places/folder-open.png"
  install -m 644 "$TMP/icon-003.png" "$THEME/48x48/places/user-home.png"
  install -m 644 "$TMP/icon-004.png" "$THEME/48x48/places/network-server.png"
  install -m 644 "$TMP/icon-005.png" "$THEME/48x48/places/drive-harddisk.png"
  for n in $(seq -w 1 12); do
    install -m 644 "$TMP/icon-${n}.png" "$THEME/48x48/apps/ubuntu-white-${n}.png"
  done

  cat > "$THEME/index.theme" <<'THEME'
[Icon Theme]
Name=Ubuntu White
Comment=Ubuntu Determinant Professional Ubuntu White Edition
Inherits=hicolor
Directories=48x48/places,48x48/apps,48x48/mimetypes,48x48/status

[48x48/places]
Size=48
Context=Places
Type=Fixed

[48x48/apps]
Size=48
Context=Applications
Type=Fixed

[48x48/mimetypes]
Size=48
Context=MimeTypes
Type=Fixed

[48x48/status]
Size=48
Context=Status
Type=Fixed
THEME
fi

if [ "$INSTALL_CSS" = "1" ]; then
  GTK_THEME="/usr/share/themes/Ubuntu-White"
  mkdir -p "$GTK_THEME/gtk-3.0" "$GTK_THEME/gtk-4.0" "/usr/share/gnome-shell/theme"
  curl --fail --location --retry 3 -sS -o "$GTK_THEME/gtk-3.0/gtk.css" "$RAW_BASE/main/gnome/theme/gtk/gtk.css"
  curl --fail --location --retry 3 -sS -o "$GTK_THEME/gtk-3.0/settings.ini" "$RAW_BASE/main/gnome/theme/gtk/settings.ini"
  cp -f "$GTK_THEME/gtk-3.0/gtk.css" "$GTK_THEME/gtk-4.0/gtk.css"
  curl --fail --location --retry 3 -sS -o "/usr/share/gnome-shell/theme/ubuntu-white.css" "$RAW_BASE/main/gnome/theme/shell/gnome-shell.css"
fi

mkdir -p /etc/dconf/db/ubuntu-white.d
cat > /etc/dconf/db/ubuntu-white.d/00-defaults <<'DCONF'
[org/gnome/desktop/interface]
color-scheme='prefer-light'
gtk-theme='Ubuntu-White'
icon-theme='Ubuntu-White'
font-name='Sans 10'
DCONF

dconf update 2>/dev/null || true

glib-compile-schemas /usr/share/glib-2.0/schemas/ 2>/dev/null || true

echo "=== Ubuntu White Edition overlay installed ==="
echo "  Icon source: images/desktop-icons/set-002"
echo "  Icons enabled: ${INSTALL_ICONS}"
echo "  CSS enabled:   ${INSTALL_CSS}"
