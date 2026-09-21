#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(CDPATH= cd -- "${SCRIPT_DIR}/.." && pwd)"
BUILD_ROOT="${BUILD_ROOT:-${REPO_ROOT}/build/work}"
OUTPUT_ROOT="${OUTPUT_ROOT:-${REPO_ROOT}/build/output}"
DESKTOP_OUTPUT="${DESKTOP_OUTPUT:-${HOME}/Desktop}"
ISO_LABEL="${ISO_LABEL:-MIRVKBUNTU}"
UBUNTU_SUITE="${UBUNTU_SUITE:-noble}"
ARCH="${ARCH:-amd64}"

die(){ printf 'build: ERROR: %s\n' "$*" >&2; exit 1; }
require_commands(){ local c; for c in "$@"; do command -v "$c" >/dev/null 2>&1 || die "required command not found: $c"; done; }
require_mirvkbuntu_source(){
  local d
  for d in kernels file-systems sources packages userland user-interface gnome-source docs; do
    [ -d "${REPO_ROOT}/${d}" ] || die "MirvkBuntu author source directory missing: ${d}"
  done
  [ -f "${REPO_ROOT}/packages/basic-packages.txt" ] || die "MirvkBuntu package manifest missing: packages/basic-packages.txt"
}
prepare_dirs(){
  mkdir -p "$BUILD_ROOT" "$OUTPUT_ROOT"
  if [ -e "${REPO_ROOT}/output" ] && [ ! -L "${REPO_ROOT}/output" ]; then
    die "${REPO_ROOT}/output exists and is not the required symlink"
  fi
  if [ -L "${REPO_ROOT}/output" ]; then
    [ "$(readlink "${REPO_ROOT}/output")" = "${OUTPUT_ROOT}" ] || die "${REPO_ROOT}/output points somewhere other than ${OUTPUT_ROOT}"
  else
    ln -s "${OUTPUT_ROOT}" "${REPO_ROOT}/output"
  fi
  mkdir -p "${DESKTOP_OUTPUT}"
}
stage_mirvkbuntu_source(){
  local work_dir="$1"
  local include_root="${work_dir}/config/includes.chroot/opt/mirvkbuntu/source"
  require_mirvkbuntu_source
  mkdir -p "$include_root" "${work_dir}/config/includes.chroot/etc/mirvkbuntu"
  for d in kernels file-systems sources packages userland user-interface gnome-source docs; do
    cp -a "${REPO_ROOT}/${d}" "${include_root}/"
  done
  printf '%s\n' \
    "MIRVKBUNTU_SOURCE_REPOSITORY=mearvk/MirvkBuntu" \
    "MIRVKBUNTU_SOURCE_ROOT=/opt/mirvkbuntu/source" \
    "MIRVKBUNTU_AUTHORITATIVE=true" \
    "MIRVKBUNTU_BASE_DISTRIBUTION=${UBUNTU_SUITE}" \
    "MIRVKBUNTU_BUILD_ARCH=${ARCH}" \
    > "${work_dir}/config/includes.chroot/etc/mirvkbuntu/source.conf"
}
write_mirvkbuntu_manifest(){
  local work_dir="$1"
  mkdir -p "${work_dir}/config/includes.chroot/etc/mirvkbuntu"
  (
    cd "${REPO_ROOT}"
    find kernels file-systems sources packages userland user-interface gnome-source docs -type f -print0 |
      sort -z |
      while IFS= read -r -d '' f; do
        sha256sum "$f"
      done
  ) > "${work_dir}/config/includes.chroot/etc/mirvkbuntu/source.sha256"
}
write_package_list(){
  local work_dir="$1"
  local list_name="$2"
  mkdir -p "${work_dir}/config/package-lists"
  awk '!/^[[:space:]]*#/ && NF {print $1}' "${REPO_ROOT}/packages/basic-packages.txt" |
    sort -u > "${work_dir}/config/package-lists/${list_name}.list.chroot"
}
run_native_build(){
  local native_script="$SCRIPT_DIR/native-build.sh"
  [ -x "$native_script" ] || die "MirvkBuntu native compilation script missing or not executable: $native_script"
  printf "build: compiling MirvkBuntu native components before live-build\n"
  BUILD_ROOT="$BUILD_ROOT" ARCH="$ARCH" "$native_script"
  [ -d "$BUILD_ROOT/native/rootfs" ] || die "native build completed without a rootfs staging tree"
  [ -d "$BUILD_ROOT/native/artifacts/packages" ] || die "native build completed without native packages"
}

stage_native_outputs(){
  local work_dir="$1"
  local native_root="$BUILD_ROOT/native"
  local include_root="$work_dir/config/includes.chroot"
  local package_root="$work_dir/config/packages.chroot"
  local deb
  mkdir -p "$include_root" "$package_root"
  [ -d "$native_root/rootfs" ] || die "native rootfs staging tree missing"
  cp -a "$native_root/rootfs/." "$include_root/"
  shopt -s nullglob
  for deb in "$native_root/artifacts/packages/"*.deb; do cp -f "$deb" "$package_root/"; done
  shopt -u nullglob
  find "$package_root" -type f -name "*.deb" -print -quit | grep -q . || die "no native Debian packages staged for live-build"
  mkdir -p "$work_dir/config/includes.chroot/etc/mirvkbuntu"
  cp -f "$native_root/artifacts/release.env" "$work_dir/config/includes.chroot/etc/mirvkbuntu/native-release.env"
  cp -f "$native_root/artifacts/rootfs.sha256" "$work_dir/config/includes.chroot/etc/mirvkbuntu/native-rootfs.sha256"
}
# Build and stage the edition-aware installer (and the Bash install engine it
# can delegate to) into the live rootfs, and expose a "mirvkbuntu-install"
# convenience wrapper on PATH. This is what makes the ISO self-installing: the
# live session ships a real installer executable (mirvkbuntu-installer) that
# lays the chosen edition down on disk. $1 is the live-build work dir, $2 is the
# edition name (slim|minimal|full) baked into the wrapper's default.
stage_installer(){
  local work_dir="$1"
  local edition="${2:-full}"
  local inc="$work_dir/config/includes.chroot"
  local installer_dir="$REPO_ROOT/packages/installer/linux"
  local engine="$REPO_ROOT/sources/scripts/galactic-cherry-installer"

  mkdir -p "$inc/usr/sbin" "$inc/usr/bin"

  # Compile the native installer binaries if a toolchain is present. The ISO
  # ships the prebuilt ELF; a missing compiler on the build host is non-fatal
  # (a committed binary in the repo, if any, is used instead).
  if [ -f "$installer_dir/Makefile" ] && command -v cc >/dev/null 2>&1; then
    make -C "$installer_dir" mirvkbuntu-installer white-installer >/dev/null 2>&1 || \
      printf 'build: WARNING: native installer build failed; staging any prebuilt binary\n' >&2
  fi

  if [ -x "$installer_dir/mirvkbuntu-installer" ]; then
    cp -f "$installer_dir/mirvkbuntu-installer" "$inc/usr/sbin/mirvkbuntu-installer"
    chmod 0755 "$inc/usr/sbin/mirvkbuntu-installer"
  else
    printf 'build: WARNING: mirvkbuntu-installer binary not found to stage\n' >&2
  fi
  [ -x "$installer_dir/white-installer" ] && \
    cp -f "$installer_dir/white-installer" "$inc/usr/sbin/white-installer"

  # Stage the Bash install engine under /usr/sbin so white-installer's
  # delegation target resolves on the live system as well.
  if [ -f "$engine" ]; then
    cp -f "$engine" "$inc/usr/sbin/galactic-cherry-installer"
    chmod 0755 "$inc/usr/sbin/galactic-cherry-installer"
  fi

  # A tiny PATH wrapper that launches the installer with the edition this ISO
  # was built for as the default (still overridable with --edition).
  cat > "$inc/usr/bin/mirvkbuntu-install" <<WRAP
#!/bin/sh
# MirvkBuntu ${edition} edition — install-to-disk launcher (live session).
# Runs the edition-aware installer as root. Default run is a safe dry-run;
# pass --install (and a --target) to actually write to disk.
exec sudo /usr/sbin/mirvkbuntu-installer --edition ${edition} "\$@"
WRAP
  chmod 0755 "$inc/usr/bin/mirvkbuntu-install"

  # Record the edition marker so the installer auto-detects it at runtime.
  mkdir -p "$inc/etc/mirvkbuntu"
  printf 'MIRVKBUNTU_EDITION=%s\n' "$edition" > "$inc/etc/mirvkbuntu/edition.conf"

  printf 'build: staged mirvkbuntu-installer (edition=%s) into the live rootfs\n' "$edition"
}

run_live_build(){
  local work_dir="$1"
  local log_file="${work_dir}/live-build.log"
  printf 'build: live-build started in %s\n' "$work_dir"
  if ! lb build 2>&1 | tee "$log_file"; then
    printf 'build: live-build failed; work tree retained at %s\n' "$work_dir" >&2
    printf 'build: live-build log: %s\n' "$log_file" >&2
    return 1
  fi
}
find_iso(){
  local work_dir="$1"
  local iso
  iso="$(find "$work_dir" -maxdepth 1 -type f \( -name '*.iso' -o -name '*.iso.hybrid' \) -print -quit)"
  [ -n "$iso" ] || die "live-build completed without producing an ISO; inspect ${work_dir}/live-build.log"
  printf '%s\n' "$iso"
}
publish_iso(){
  local iso="$1"
  local name
  name="$(basename "$iso")"
  mkdir -p "$OUTPUT_ROOT"
  cp -f "$iso" "$OUTPUT_ROOT/$name"
  cp -f "$OUTPUT_ROOT/$name" "$DESKTOP_OUTPUT/$name"
  printf 'build: ISO stored at %s\n' "$OUTPUT_ROOT/$name"
  printf 'build: ISO copied to desktop: %s\n' "$DESKTOP_OUTPUT/$name"
}
