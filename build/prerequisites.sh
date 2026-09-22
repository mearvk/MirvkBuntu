#!/usr/bin/env bash
set -euo pipefail

# MirvkBuntu build prerequisites installer.
#
# Installs the host tooling required by the two supported build paths:
#   * build/build-limited.sh   -- Quick Limited repository-source-driven path
#   * build/build-desktop.sh   -- native live-build path
#
# This is idempotent: already-installed packages are skipped by apt. It must be
# run on a Debian/Ubuntu host with root privileges and network access to the
# Ubuntu package archive.
#
# Usage:
#   sudo bash build/prerequisites.sh              # install everything
#   sudo bash build/prerequisites.sh remaster     # only the remaster path (A)
#   sudo bash build/prerequisites.sh native       # only the native path (B)

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

die(){ printf 'prerequisites: ERROR: %s\n' "$*" >&2; exit 1; }
log(){ printf 'prerequisites: %s\n' "$*"; }

MODE="${1:-all}"
case "$MODE" in
  all|remaster|native) ;;
  *) die "unknown mode: $MODE (expected: all | remaster | native)" ;;
esac

# --- host sanity checks ------------------------------------------------------
command -v apt-get >/dev/null 2>&1 || \
  die "apt-get not found; this installer targets Debian/Ubuntu hosts only"

if [ "$(id -u)" -ne 0 ]; then
  die "must run as root (use: sudo bash build/prerequisites.sh $MODE)"
fi

# Tooling shared by both paths.
COMMON_PACKAGES=(
  ca-certificates
  curl
  wget
  xz-utils
  rsync
)

# Legacy remaster tooling: unpack/repack an existing Ubuntu ISO. This is not used by Limited/source-driven builds.
REMASTER_PACKAGES=(
  squashfs-tools      # unsquashfs / mksquashfs
  xorriso             # ISO (re)authoring
  isolinux            # BIOS boot records for hybrid ISOs
  syslinux-utils      # isohybrid
  genisoimage         # mkisofs fallback / manifest tooling
)

# Native live-build path (B): full debootstrap + live-build assembly, plus the
# native compilation gate (kernel/GNOME/Chromium) in build/native-build.sh.
NATIVE_PACKAGES=(
  live-build
  debootstrap
  squashfs-tools
  xorriso

  # --- generic build toolchain -------------------------------------------
  build-essential
  dpkg-dev
  fakeroot
  pkg-config
  meson
  ninja-build
  cmake
  git
  python3
  python3-pip
  python3-setuptools

  # --- Linux kernel build (bindeb-pkg) -----------------------------------
  bc
  bison
  flex
  libelf-dev
  libssl-dev
  kmod
  cpio
  zstd
  dwarves            # pahole, required by modern kernels for BTF
  rsync
  libncurses-dev     # menuconfig / config tooling

  # --- GObject introspection + docs tooling used across GNOME ------------
  gobject-introspection
  libgirepository1.0-dev
  gettext
  libxml2-utils
  python3-gi
  sassc

  # --- GLib -------------------------------------------------------------
  libpcre2-dev
  libffi-dev
  zlib1g-dev
  libmount-dev
  libselinux1-dev

  # --- cairo / pango / gdk-pixbuf ---------------------------------------
  libfontconfig1-dev
  libfreetype-dev
  libharfbuzz-dev
  libpng-dev
  libjpeg-dev
  libtiff-dev
  libpixman-1-dev
  libfribidi-dev

  # --- GTK --------------------------------------------------------------
  libepoxy-dev
  libxkbcommon-dev
  libwayland-dev
  wayland-protocols
  libgraphene-1.0-dev
  libgstreamer-plugins-base1.0-dev
  libcups2-dev
  libcolord-dev

  # --- X / GL used by GTK, mutter --------------------------------------
  libx11-dev
  libxext-dev
  libxi-dev
  libxrandr-dev
  libxcursor-dev
  libxdamage-dev
  libxfixes-dev
  libxcomposite-dev
  libxkbfile-dev
  libgl1-mesa-dev
  libegl1-mesa-dev
  libgbm-dev
  libdrm-dev

  # --- mutter / gnome-shell --------------------------------------------
  libwayland-egl-backend-dev
  libinput-dev
  libudev-dev
  libsystemd-dev
  libgnome-desktop-3-dev
  libgudev-1.0-dev
  libpolkit-gobject-1-dev
  gjs
  libgjs-dev
  libstartup-notification0-dev
  libxcb1-dev
  libxcb-randr0-dev

  # --- glib-networking / gvfs ------------------------------------------
  libgnutls28-dev
  libproxy-dev
  libgcr-4-dev
  libsecret-1-dev
  libsoup-3.0-dev
)

packages=( "${COMMON_PACKAGES[@]}" )
case "$MODE" in
  remaster) packages+=( "${REMASTER_PACKAGES[@]}" ) ;;
  native)   packages+=( "${NATIVE_PACKAGES[@]}" ) ;;
  all)      packages+=( "${REMASTER_PACKAGES[@]}" "${NATIVE_PACKAGES[@]}" ) ;;
esac

# Deduplicate while preserving order.
declare -A seen=()
unique_packages=()
for pkg in "${packages[@]}"; do
  [ -n "${seen[$pkg]:-}" ] && continue
  seen[$pkg]=1
  unique_packages+=( "$pkg" )
done

log "mode: $MODE"
log "installing ${#unique_packages[@]} package(s): ${unique_packages[*]}"

export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y --no-install-recommends "${unique_packages[@]}"

# --- verification ------------------------------------------------------------
# Confirm the tools each selected path actually invokes are now present.
verify_commands(){
  local missing=() c
  for c in "$@"; do
    command -v "$c" >/dev/null 2>&1 || missing+=( "$c" )
  done
  if [ "${#missing[@]}" -gt 0 ]; then
    die "expected commands still missing after install: ${missing[*]}"
  fi
}

case "$MODE" in
  remaster) verify_commands unsquashfs mksquashfs xorriso ;;
  native)   verify_commands lb live-build debootstrap unsquashfs mksquashfs xorriso make gcc cpack ;;
  all)      verify_commands unsquashfs mksquashfs xorriso lb live-build debootstrap make gcc cpack ;;
esac

log "all required tooling is installed and on PATH."
case "$MODE" in
  remaster) log "next (legacy only): sudo bash ${SCRIPT_DIR}/quick-remaster.sh <ubuntu.iso>" ;;
  native)   log "next (Limited): sudo bash ${SCRIPT_DIR}/build-limited.sh" ;;
  all)      log "next (Limited): sudo bash ${SCRIPT_DIR}/build-limited.sh"
            log "next (desktop): sudo bash ${SCRIPT_DIR}/build-desktop.sh" ;;
esac
