#!/usr/bin/env bash
set -euo pipefail

# MirvkBuntu Quick Limited Edition.
#
# Repository-source-driven ISO build. It NEVER consumes or downloads an existing
# Ubuntu ISO. Ubuntu/Debian package archives are allowed and expected: live-build
# and APT acquire .deb packages needed for the first desktop experience.
#
# The edition is intentionally limited: the MirvkBuntu native build gate runs,
# while heavyweight optional native desktop applications can remain deferred.

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/common-build.sh"

require_commands live-build lb
require_mirvkbuntu_source
prepare_dirs

# Quick/limited defaults. Callers can override these explicitly.
export BUILD_SKIP_GNOME="${BUILD_SKIP_GNOME:-1}"
export BUILD_SKIP_CHROMIUM="${BUILD_SKIP_CHROMIUM:-1}"

run_native_build

WORK_DIR="${BUILD_ROOT}/limited"
rm -rf "$WORK_DIR"
mkdir -p "$WORK_DIR"
cd "$WORK_DIR"

lb clean --purge >/dev/null 2>&1 || true

lb config \
  --distribution "$UBUNTU_SUITE" \
  --architectures "$ARCH" \
  --binary-images iso-hybrid \
  --debian-installer live \
  --apt-recommends true \
  --archive-areas "main restricted universe multiverse" \
  --iso-application "MirvkBuntu Limited" \
  --iso-publisher "MEARVK LLC" \
  --iso-volume "$ISO_LABEL-LIMITED" \
  --linux-packages none

# Standard .deb acquisition is part of this build. No pre-existing ISO is used.
write_package_list "$WORK_DIR" mirvkbuntu-limited
cat > "$WORK_DIR/config/package-lists/mirvkbuntu-limited-desktop.list.chroot" <<'EOF'
# MirvkBuntu Quick Limited Edition — first desktop experience
ubuntu-desktop-minimal
gnome-shell
gnome-session
gdm3
gnome-control-center
gnome-terminal
nautilus
gnome-text-editor
network-manager
xserver-xorg
mesa-utils
live-boot
live-config
live-config-systemd
EOF

stage_mirvkbuntu_source "$WORK_DIR"
write_mirvkbuntu_manifest "$WORK_DIR"
stage_native_outputs "$WORK_DIR"

# Apply the repository's Ubuntu White desktop treatment when its assets are
# present. The theme is optional to the build, but the Limited edition should
# present the intended white/dark-grey first-desktop experience.
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
mkdir -p "$INC/etc/dconf/db/local.d" "$INC/etc/dconf/profile"
printf 'user-db:user\nsystem-db:local\n' > "$INC/etc/dconf/profile/user"
cat > "$INC/etc/dconf/db/local.d/00-mirvkbuntu-theme" <<'DCONF'
[org/gnome/desktop/interface]
gtk-theme='MirvkBuntu-White'
color-scheme='default'
DCONF

mkdir -p "$WORK_DIR/config/includes.chroot/etc/mirvkbuntu"

cat > "$WORK_DIR/config/includes.chroot/etc/mirvkbuntu/edition.conf" <<EOF
MIRVKBUNTU_EDITION=limited
MIRVKBUNTU_BUILD_MODEL=repository-source-driven
MIRVKBUNTU_EXISTING_ISO_INPUT=false
MIRVKBUNTU_DEB_ARCHIVES_ALLOWED=true
MIRVKBUNTU_OPTIONAL_GNOME_NATIVE_BUILD_SKIPPED=$BUILD_SKIP_GNOME
MIRVKBUNTU_OPTIONAL_CHROMIUM_NATIVE_BUILD_SKIPPED=$BUILD_SKIP_CHROMIUM
EOF

run_live_build "$WORK_DIR"

ISO_SOURCE="$(find_iso "$WORK_DIR")"
mkdir -p "$OUTPUT_ROOT"
FINAL_ISO="$OUTPUT_ROOT/MirvkBuntu-limited-${ARCH}.iso"
mv -f "$ISO_SOURCE" "$FINAL_ISO"

write_release_manifest "$FINAL_ISO" "$WORK_DIR" "limited"
validate_release_iso "$FINAL_ISO" "$WORK_DIR" "limited"

printf 'build: created %s\n' "$FINAL_ISO"
publish_iso "$FINAL_ISO"
