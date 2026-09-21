#!/usr/bin/env bash
set -euo pipefail

# MirvkBuntu native compilation gate. This runs before live-build.
# Required project components must compile successfully; distro fallbacks are rejected.

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$BASH_SOURCE")" && pwd)"
REPO_ROOT="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)"
BUILD_ROOT="${BUILD_ROOT:-$REPO_ROOT/build/work}"
NATIVE_ROOT="${NATIVE_ROOT:-$BUILD_ROOT/native}"
ARTIFACT_ROOT="${ARTIFACT_ROOT:-$NATIVE_ROOT/artifacts}"
ROOTFS_STAGE="${ROOTFS_STAGE:-$NATIVE_ROOT/rootfs}"
JOBS="${JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)}"
ARCH="${ARCH:-amd64}"

die(){ printf "native-build: ERROR: %s\n" "$*" >&2; exit 1; }
require(){ command -v "$1" >/dev/null 2>&1 || die "required command not found: $1"; }
for c in find sha256sum tar split make dpkg-deb; do require "$c"; done
rm -rf "$NATIVE_ROOT"
mkdir -p "$ARTIFACT_ROOT/packages" "$ARTIFACT_ROOT/chunks" "$ROOTFS_STAGE"
log(){ printf "native-build: %s\n" "$*"; }

build_kernels(){
  local found=0 candidate src work deb name archive extract_root kernel_root makefile_count
  kernel_root="$REPO_ROOT/kernels"
  log "Compiling every MirvkBuntu kernel source tree that is actually present."
  log "native-build script directory: $SCRIPT_DIR"
  log "native-build repository root: $REPO_ROOT"
  log "native-build kernel source root: $kernel_root"

  [ -d "$kernel_root" ] || die "kernel source directory is missing: expected $kernel_root (script is in $SCRIPT_DIR; repository root is one directory above build/)"

  if [ ! -r "$kernel_root" ]; then
    die "kernel source directory is not readable: $kernel_root"
  fi

  log "kernel discovery: searching recursively under $kernel_root for top-level Makefile files"
  makefile_count="$(find -L "$kernel_root" -type f -name Makefile -print 2>/dev/null | wc -l)"
  log "kernel discovery: found $makefile_count Makefile file(s) under $kernel_root"

  while IFS= read -r -d "" candidate; do
    src="$(dirname "$candidate")"
    log "kernel discovery: candidate Makefile: $candidate"
    if [ ! -f "$src/Kconfig" ]; then
      log "kernel discovery: rejected candidate (missing Kconfig): $src"
      continue
    fi
    if [ ! -d "$src/arch" ]; then
      log "kernel discovery: rejected candidate (missing arch/): $src"
      continue
    fi
    found=1
    name="$(basename "$src")"
    work="$NATIVE_ROOT/kernels/$name"
    rm -rf "$work"
    mkdir -p "$work"
    cp -a "$src/." "$work/"
    log "kernel: compiling source tree $name from $src"
    make -C "$work" olddefconfig
    make -C "$work" -j"$JOBS" bindeb-pkg
    for deb in "$NATIVE_ROOT/kernels/"*.deb "$work/../"*.deb; do
      [ -f "$deb" ] || continue
      cp -f "$deb" "$ARTIFACT_ROOT/packages/"
    done
  done < <(find -L "$kernel_root" -type f -name Makefile -print0)

  while IFS= read -r -d "" archive; do
    extract_root="$NATIVE_ROOT/kernel-archives/$(basename "$archive")"
    rm -rf "$extract_root"
    mkdir -p "$extract_root"
    case "$archive" in
      *.tar.gz|*.tgz) tar -xzf "$archive" -C "$extract_root" ;;
      *.tar.xz) tar -xJf "$archive" -C "$extract_root" ;;
      *.tar.zst) tar --zstd -xf "$archive" -C "$extract_root" ;;
      *.tar.bz2) tar -xjf "$archive" -C "$extract_root" ;;
      *) continue ;;
    esac
    candidate="$(find -L "$extract_root" -type f -name Makefile -print -quit)"
    [ -n "$candidate" ] || continue
    src="$(dirname "$candidate")"
    [ -f "$src/Kconfig" ] || continue
    [ -d "$src/arch" ] || continue
    found=1
    name="$(basename "$src")"
    work="$NATIVE_ROOT/kernels/$name"
    rm -rf "$work"
    mkdir -p "$work"
    cp -a "$src/." "$work/"
    log "kernel: compiling archived source tree $name from $src"
    make -C "$work" olddefconfig
    make -C "$work" -j"$JOBS" bindeb-pkg
    for deb in "$NATIVE_ROOT/kernels/"*.deb "$work/../"*.deb; do
      [ -f "$deb" ] || continue
      cp -f "$deb" "$ARTIFACT_ROOT/packages/"
    done
  done < <(find -L "$kernel_root" -type f \( -name "*.tar.gz" -o -name "*.tgz" -o -name "*.tar.xz" -o -name "*.tar.zst" -o -name "*.tar.bz2" \) -print0)

  if [ "$found" -eq 0 ]; then
    log "kernel discovery diagnostics:"
    log "  script directory = $SCRIPT_DIR"
    log "  repository root  = $REPO_ROOT"
    log "  expected kernels  = $kernel_root"
    if [ -d "$REPO_ROOT" ]; then
      log "  repository root entries:"
      find "$REPO_ROOT" -maxdepth 1 -mindepth 1 -printf "    %p\n" 2>/dev/null || true
    fi
    if [ -d "$kernel_root" ]; then
      log "  kernels directory entries:"
      find -L "$kernel_root" -maxdepth 3 -mindepth 1 -printf "    %p\n" 2>/dev/null | head -100 || true
      log "  Makefiles actually discovered:"
      find -L "$kernel_root" -type f -name Makefile -print 2>/dev/null | head -100 || true
    fi
    die "no complete MirvkBuntu-supplied kernel source found under $kernel_root; build/native-build.sh resolves the repository root as the parent of build/ and refuses a distribution-kernel fallback"
  fi
}
# Canonical Chromium source root and its authoritative build script. Pinning to
# userland/chromium/chromium-src avoids matching nested chrome/BUILD.gn fixtures
# (e.g. base/tracing/stdlib/chrome/BUILD.gn) that are not the real source tree.
CHROMIUM_SRC_ROOT="$REPO_ROOT/userland/chromium/chromium-src"
CHROMIUM_BUILD_SCRIPT="$REPO_ROOT/build/chromium/build-chromium.sh"

build_chromium(){
  # Validate the pinned source root is the real Chromium tree (not a fixture).
  [ -f "$CHROMIUM_SRC_ROOT/DEPS" ] && [ -f "$CHROMIUM_SRC_ROOT/BUILD.gn" ] && [ -f "$CHROMIUM_SRC_ROOT/chrome/BUILD.gn" ] \
    || die "Chromium source not found at $CHROMIUM_SRC_ROOT (expected DEPS + BUILD.gn + chrome/BUILD.gn); refusing a distro-browser fallback"
  [ -x "$CHROMIUM_BUILD_SCRIPT" ] || die "authoritative Chromium build script missing or not executable: $CHROMIUM_BUILD_SCRIPT"

  # Delegate to the single authoritative build. It bootstraps depot_tools,
  # syncs, does a release build, and installs into DESTDIR (the rootfs staging
  # tree) as /opt/mirvkbuntu/chromium + a /usr/bin launcher.
  log "chromium: delegating to $CHROMIUM_BUILD_SCRIPT"
  DESTDIR="$ROOTFS_STAGE" PREFIX=/usr JOBS="$JOBS" \
    CHROMIUM_SRC="$CHROMIUM_SRC_ROOT" \
    CHROMIUM_OUT="$NATIVE_ROOT/chromium/out/MirvkBuntuRelease" \
    "$CHROMIUM_BUILD_SCRIPT"

  [ -x "$ROOTFS_STAGE/opt/mirvkbuntu/chromium/chrome" ] \
    || die "Chromium build did not stage a chrome executable into the rootfs"
}

build_gnome(){
  local builder="$REPO_ROOT/gnome-source/build-module.sh" module
  [ -x "$builder" ] || die "GNOME common builder is missing or not executable"
  for module in cairo glib gdk-pixbuf gtk glib-networking gvfs mutter gnome-shell gnome-control-center gnome-software gnome-terminal orca; do
    [ -d "$REPO_ROOT/gnome-source/$module/source" ] || die "required GNOME module source missing: $module"
    log "gnome: compiling $module"
    DESTDIR="$ROOTFS_STAGE" PREFIX=/usr INSTALL=1 JOBS="$JOBS" "$builder" "$module"
  done
}

build_other_native_projects(){
  local base dir buildfile count out
  for base in "$REPO_ROOT/sources" "$REPO_ROOT/userland" "$REPO_ROOT/user-interface"; do
    [ -d "$base" ] || continue
    while IFS= read -r -d "" dir; do
      # Chromium is owned by the dedicated build_chromium stage; skip it here so
      # it is never built twice (its build.sh delegates to the same script).
      case "$dir" in
        "$REPO_ROOT/userland/chromium"|"$REPO_ROOT/userland/chromium"/*)
          continue ;;
      esac
      if [ -x "$dir/build.sh" ]; then
        log "native project wrapper: $dir/build.sh"
        DESTDIR="$ROOTFS_STAGE" JOBS="$JOBS" MIRVKBUNTU_RELEASE=true "$dir/build.sh"
        continue
      fi
      count=0
      for buildfile in Makefile meson.build CMakeLists.txt configure; do [ -e "$dir/$buildfile" ] && count=$((count+1)); done
      [ "$count" -eq 0 ] && continue
      [ "$count" -eq 1 ] || die "ambiguous native project build definition in $dir"
      out="$NATIVE_ROOT/other/$(basename "$dir")"
      if [ -x "$dir/configure" ]; then
        mkdir -p "$out"
        (cd "$out" && "$dir/configure" --prefix=/usr && make -j"$JOBS" && make DESTDIR="$ROOTFS_STAGE" install)
      elif [ -f "$dir/meson.build" ]; then
        require meson
        meson setup "$out" "$dir" --buildtype=release --prefix=/usr
        meson compile -C "$out"
        DESTDIR="$ROOTFS_STAGE" meson install -C "$out"
      elif [ -f "$dir/CMakeLists.txt" ]; then
        require cmake
        cmake -S "$dir" -B "$out" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
        cmake --build "$out" --parallel "$JOBS"
        DESTDIR="$ROOTFS_STAGE" cmake --install "$out"
      else
        (cd "$dir" && make -j"$JOBS")
      fi
    done < <(find "$base" -mindepth 1 -maxdepth 2 -type d -print0)
  done
}

verify_native_output(){
  local executable_count package_count
  executable_count="$(find "$ROOTFS_STAGE" -type f -perm /111 | wc -l)"
  package_count="$(find "$ARTIFACT_ROOT/packages" -type f -name "*.deb" | wc -l)"
  [ "$executable_count" -gt 0 ] || die "native build produced no executable files"
  [ "$package_count" -gt 0 ] || die "native build produced no Debian packages"
  for deb in "$ARTIFACT_ROOT/packages/"*.deb; do dpkg-deb --info "$deb" >/dev/null || die "invalid native Debian package: $deb"; done
  (cd "$ROOTFS_STAGE" && find . -type f -print0 | sort -z | xargs -0 sha256sum) > "$ARTIFACT_ROOT/rootfs.sha256"
  sha256sum "$ARTIFACT_ROOT/rootfs.sha256" > "$ARTIFACT_ROOT/rootfs.sha256.digest"
}

write_release_manifest(){
  # Report the true state of each stage so a partial (stage-skipped) build is
  # never mislabeled as a complete native release.
  local kernels_compiled chromium_compiled gnome_compiled complete
  kernels_compiled=$([ "${BUILD_SKIP_KERNELS:-0}" = "1" ] && echo false || echo true)
  chromium_compiled=$([ "${BUILD_SKIP_CHROMIUM:-0}" = "1" ] && echo false || echo true)
  gnome_compiled=$([ "${BUILD_SKIP_GNOME:-0}" = "1" ] && echo false || echo true)
  if [ "$kernels_compiled" = true ] && [ "$chromium_compiled" = true ] && [ "$gnome_compiled" = true ]; then
    complete=true
  else
    complete=false
  fi
  printf "%s\n" \
    "MIRVKBUNTU_NATIVE_RELEASE=$complete" \
    "ARCH=$ARCH" "JOBS=$JOBS" \
    "KERNELS_COMPILED=$kernels_compiled" \
    "CHROMIUM_COMPILED=$chromium_compiled" \
    "GNOME_COMPILED=$gnome_compiled" \
    "SOURCE_TREE=$REPO_ROOT" > "$ARTIFACT_ROOT/release.env"
  sha256sum "$ARTIFACT_ROOT"/packages/*.deb "$ARTIFACT_ROOT/rootfs.sha256" > "$ARTIFACT_ROOT/artifacts.sha256"
}

create_split_bundle(){
  local bundle="$ARTIFACT_ROOT/mirvkbuntu-native-$ARCH.tar.zst"
  tar --zstd -C "$ARTIFACT_ROOT" -cf "$bundle" packages rootfs.sha256 rootfs.sha256.digest release.env artifacts.sha256
  rm -f "$ARTIFACT_ROOT/chunks/"*
  split -b "${MIRVKBUNTU_CHUNK_SIZE:-90M}" -d -a 4 "$bundle" "$ARTIFACT_ROOT/chunks/mirvkbuntu-native-$ARCH.part-"
  sha256sum "$ARTIFACT_ROOT/chunks/"* > "$ARTIFACT_ROOT/chunks/SHA256SUMS"
}

# Stage selection.
#
# By default every native stage is mandatory: a complete distribution build must
# compile its own kernel, GNOME stack, and Chromium. However, while the source
# trees are still being populated it is useful to produce an ISO with the stages
# whose source IS present. Each stage can be individually skipped by exporting
# the matching variable to 1. Skips are logged loudly so a partial build is
# never mistaken for a complete one.
#
#   BUILD_SKIP_KERNELS=1   BUILD_SKIP_CHROMIUM=1
#   BUILD_SKIP_GNOME=1     BUILD_SKIP_OTHER=1
#
# verify_native_output still requires at least one executable and one .deb, so a
# fully-empty build is still rejected.
run_stage(){
  local name="$1" skip_var="$2" fn="$3"
  if [ "${!skip_var:-0}" = "1" ]; then
    log "WARNING: skipping stage '$name' because $skip_var=1 (build will NOT be a complete native distribution)"
    return 0
  fi
  "$fn"
}

run_stage kernels  BUILD_SKIP_KERNELS  build_kernels
run_stage chromium BUILD_SKIP_CHROMIUM build_chromium
run_stage gnome    BUILD_SKIP_GNOME    build_gnome
run_stage other    BUILD_SKIP_OTHER    build_other_native_projects
verify_native_output
write_release_manifest
create_split_bundle
log "ALL REQUIRED NATIVE COMPILATION PASSED."
log "Only MirvkBuntu-native build outputs may be fed to the ISO assembly stage."
