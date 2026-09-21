#!/usr/bin/env bash
set -euo pipefail

# MirvkBuntu native-build bootstrap (path B).
#
# One entry point that prepares the inputs the native live-build pipeline needs,
# then runs it:
#   1. Fetches real Linux kernel source into kernels/ (via kernels/git.sh) unless
#      compilable source is already present.
#   2. Invokes build/build-desktop.sh, which runs the native compilation gate
#      (build/native-build.sh) and then live-build.
#
# The native gate compiles the kernel(s), the GNOME stack, and Chromium from the
# source trees in this repository. Trees that are not yet populated can be
# skipped for a partial build (see build/native-build.sh BUILD_SKIP_* flags);
# pass them through this script, e.g.:
#
#   sudo BUILD_SKIP_GNOME=1 BUILD_SKIP_CHROMIUM=1 bash build/bootstrap-native.sh
#
# Requirements: run as root; host tooling from build/prerequisites.sh native.

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(CDPATH= cd -- "${SCRIPT_DIR}/.." && pwd)"

die(){ printf 'bootstrap-native: ERROR: %s\n' "$*" >&2; exit 1; }
log(){ printf 'bootstrap-native: %s\n' "$*"; }

# Kernel versions to fetch when none are already present. Override with
# MIRVKBUNTU_KERNEL_VERSIONS="6.12.110" for a single, faster kernel build.
KERNEL_VERSIONS="${MIRVKBUNTU_KERNEL_VERSIONS:-6.12.110}"

[ "$(id -u)" -eq 0 ] || die "must run as root (live-build + chroot); try: sudo bash build/bootstrap-native.sh"
command -v lb >/dev/null 2>&1 || die "live-build (lb) not found; run: sudo bash build/prerequisites.sh native"

# --- 1. ensure real kernel source -------------------------------------------
have_kernel_source(){
  # A usable tree has a top-level Makefile + Kconfig + arch/ (what native-build
  # discovers). HTML placeholders lack the Makefile, so this is a real check.
  find -L "$REPO_ROOT/kernels" -type f -name Makefile 2>/dev/null | while read -r mk; do
    d="$(dirname "$mk")"
    if [ -f "$d/Kconfig" ] && [ -d "$d/arch" ]; then echo "$d"; return 0; fi
  done | grep -q .
}

if [ "${BUILD_SKIP_KERNELS:-0}" = "1" ]; then
  log "BUILD_SKIP_KERNELS=1; not fetching kernel source"
elif have_kernel_source; then
  log "compilable kernel source already present under kernels/; skipping fetch"
else
  log "no compilable kernel source found; fetching: $KERNEL_VERSIONS"
  [ -x "$REPO_ROOT/kernels/git.sh" ] || die "kernels/git.sh missing or not executable"
  # shellcheck disable=SC2086
  bash "$REPO_ROOT/kernels/git.sh" $KERNEL_VERSIONS \
    || die "kernel source fetch failed (need network access to kernel.org)"
  have_kernel_source \
    || die "kernel source still not usable after fetch; inspect kernels/"
  log "kernel source ready"
fi

# --- 2. run the native + live-build pipeline --------------------------------
log "starting native compilation gate and live-build (build-desktop.sh)"
exec bash "$SCRIPT_DIR/build-desktop.sh"
