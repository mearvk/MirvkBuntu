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
run_dependency_scout(){
  local scout_binary="$REPO_ROOT/build/scout/mirvkbuntu-scout"
  local scout_build="$REPO_ROOT/build/scout.sh"
  if [ -f "$BUILD_ROOT/dependency-cache.txt" ]; then
    printf 'build: dependency scout cache already present: %s\\n' "$BUILD_ROOT/dependency-cache.txt"
    return 0
  fi
  [ -f "$scout_build" ] || die "MirvkBuntu dependency scout build script missing: $scout_build"
  if [ ! -x "$scout_binary" ]; then
    printf 'build: building dependency scout before compilation\\n'
    bash "$scout_build"
  fi
  printf 'build: scouting package dependencies before compilation\\n'
  "$scout_binary"
  [ -f "$BUILD_ROOT/dependency-cache.txt" ] || die "dependency scout did not produce dependency-cache.txt"
  cp -f "$BUILD_ROOT/dependency-cache.txt" "$BUILD_ROOT/../dependency-cache.txt" 2>/dev/null || true
}

run_native_build(){
  local native_script="$SCRIPT_DIR/native-build.sh"
  local inventory_script="$SCRIPT_DIR/component-inventory.sh"
  [ -x "$native_script" ] || die "MirvkBuntu native compilation script missing or not executable: $native_script"
  [ -f "$inventory_script" ] || die "MirvkBuntu component inventory script missing: $inventory_script"
  run_dependency_scout
  printf "build: inventorying MirvkBuntu custom components before compilation\n"
  MIRVKBUNTU_COMPONENT_INVENTORY="$BUILD_ROOT/component-inventory.txt" bash "$inventory_script"
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
