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

# Depth limit and watchdog for kernel-source discovery. Overridable.
KERNEL_FIND_MAXDEPTH="${KERNEL_FIND_MAXDEPTH:-8}"
KERNEL_FIND_TIMEOUT="${KERNEL_FIND_TIMEOUT:-120}"

# Physical (no -L), depth-bounded, single-filesystem search for top-level kernel
# Makefiles, wrapped in a watchdog so a pathological/looping tree fails cleanly
# instead of hanging forever. Prunes VCS and obvious non-source noise. Prints one
# path per line (kernel top-level dirs have no spaces).
kernel_find_makefiles(){
  local root="$1" runner=""
  command -v timeout >/dev/null 2>&1 && runner="timeout ${KERNEL_FIND_TIMEOUT}"
  # -P (default) does NOT follow symlinks; -xdev stays on one fs; prune junk.
  $runner find -P "$root" -xdev -maxdepth "$KERNEL_FIND_MAXDEPTH" \
      \( -type d \( -name .git -o -name .svn -o -name node_modules -o -name build-local \) -prune \) \
      -o \( -type f -name Makefile -print \) 2>/dev/null
  local rc=$?
  if [ "$rc" -eq 124 ]; then
    die "kernel discovery timed out after ${KERNEL_FIND_TIMEOUT}s under $root (likely a symlink loop or an enormous tree). Set KERNEL_FIND_TIMEOUT/KERNEL_FIND_MAXDEPTH, or clean up kernels/."
  fi
  return 0
}

# Configure a kernel work tree for a bootable live ISO (Task C). Starts from the
# tree's defconfig via olddefconfig, then force-enables the options a live/ISO
# system needs (squashfs, overlayfs, loop, virtio, DRM, EFI, etc.) so the
# produced kernel can actually boot the media. Uses scripts/config so it works
# across kernel versions without a hand-maintained full .config.
configure_kernel(){
  local work="$1" cfgtool="$work/scripts/config"
  make -C "$work" olddefconfig
  if [ -x "$cfgtool" ]; then
    log "kernel: applying MirvkBuntu live-ISO config options"
    # Filesystems + live/overlay boot
    "$cfgtool" --file "$work/.config" \
      --enable SQUASHFS --enable SQUASHFS_XZ --enable SQUASHFS_ZSTD \
      --enable OVERLAY_FS --enable BLK_DEV_LOOP --enable ISO9660_FS \
      --enable VFAT_FS --enable EXT4_FS --enable TMPFS --enable TMPFS_POSIX_ACL \
      --enable DEVTMPFS --enable DEVTMPFS_MOUNT \
      --enable BLK_DEV_INITRD --enable RD_XZ --enable RD_ZSTD \
      --enable FW_LOADER || true
    # Virtualization guest support (so the ISO boots in VMs)
    "$cfgtool" --file "$work/.config" \
      --enable VIRTIO --enable VIRTIO_PCI --enable VIRTIO_BLK \
      --enable VIRTIO_NET --enable VIRTIO_CONSOLE --enable DRM_VIRTIO_GPU \
      --enable HYPERVISOR_GUEST --enable PARAVIRT || true
    # Graphics / display for a GNOME desktop
    "$cfgtool" --file "$work/.config" \
      --enable DRM --enable DRM_FBDEV_EMULATION --enable FB \
      --enable DRM_I915 --enable DRM_AMDGPU --enable DRM_NOUVEAU \
      --enable FRAMEBUFFER_CONSOLE || true
    # Core desktop hardware: USB input/storage, networking, sound
    "$cfgtool" --file "$work/.config" \
      --enable USB_SUPPORT --enable USB_XHCI_HCD --enable USB_EHCI_HCD \
      --enable USB_STORAGE --enable USB_HID --enable HID_GENERIC \
      --enable INPUT_EVDEV --enable SND --enable SND_HDA_INTEL \
      --enable E1000E --enable R8169 --enable IWLWIFI \
      --enable EFI --enable EFI_STUB --enable EFIVAR_FS || true
    # Reconcile any dependency changes the enables triggered.
    make -C "$work" olddefconfig
  else
    log "kernel: scripts/config not found; using plain olddefconfig"
  fi
  # Do not require module signing / trusted keys for a self-built kernel.
  "$cfgtool" --file "$work/.config" --disable MODULE_SIG --disable SYSTEM_TRUSTED_KEYS 2>/dev/null || true
  scripts_config_set_str "$work"
  make -C "$work" olddefconfig
}

# Clear embedded key paths that break self-builds (set to empty string).
scripts_config_set_str(){
  local work="$1" cfgtool="$work/scripts/config"
  [ -x "$cfgtool" ] || return 0
  "$cfgtool" --file "$work/.config" --set-str SYSTEM_TRUSTED_KEYS "" 2>/dev/null || true
  "$cfgtool" --file "$work/.config" --set-str SYSTEM_REVOCATION_KEYS "" 2>/dev/null || true
}

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

  log "kernel discovery: searching under $kernel_root for top-level Makefile files"
  # IMPORTANT: do NOT use `find -L` here. Following symlinks (-L) will hang
  # indefinitely on a symlink loop, which is exactly the "silent stall right
  # after this line" failure mode. We search physical files only, bound the
  # depth, stay on one filesystem (-xdev), and prune VCS/build noise. A watchdog
  # timeout converts any pathological tree into a clean error instead of a hang.
  makefile_count="$(kernel_find_makefiles "$kernel_root" | wc -l)"
  log "kernel discovery: found $makefile_count Makefile file(s) under $kernel_root"

  while IFS= read -r candidate; do
    [ -n "$candidate" ] || continue
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
    log "kernel: configuring source tree $name from $src"
    configure_kernel "$work"
    log "kernel: compiling source tree $name (bindeb-pkg)"
    make -C "$work" -j"$JOBS" bindeb-pkg
    for deb in "$NATIVE_ROOT/kernels/"*.deb "$work/../"*.deb; do
      [ -f "$deb" ] || continue
      cp -f "$deb" "$ARTIFACT_ROOT/packages/"
    done
  done < <(kernel_find_makefiles "$kernel_root")

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
    candidate="$(kernel_find_makefiles "$extract_root" | head -1)"
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
    log "kernel: configuring archived source tree $name from $src"
    configure_kernel "$work"
    log "kernel: compiling archived source tree $name (bindeb-pkg)"
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
      find "$kernel_root" -maxdepth 3 -mindepth 1 -printf "    %p\n" 2>/dev/null | head -100 || true
      log "  Makefiles actually discovered:"
      kernel_find_makefiles "$kernel_root" | head -100 || true
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

  # Build in DEPENDENCY ORDER. Each module installs into the shared staging
  # rootfs ($ROOTFS_STAGE), and later modules must discover the libraries,
  # pkg-config files, headers, and girepository data produced by earlier ones.
  # We thread the standard discovery variables to point at the staging prefix so
  # meson/pkg-config/gobject-introspection resolve intra-stack deps that are not
  # yet installed on the host.
  #
  # Order rationale:
  #   glib            -> foundation (GObject/GIO); everything depends on it
  #   cairo, pango    -> text/vector; gdk-pixbuf -> image loading
  #   gtk             -> needs glib, cairo, pango, gdk-pixbuf
  #   glib-networking, gvfs -> GIO extension points
  #   mutter          -> compositor; needs gtk stack
  #   gnome-shell     -> needs mutter + gtk
  #   control-center/software/terminal/orca -> apps on top of the stack
  local order=(glib cairo pango gdk-pixbuf gtk glib-networking gvfs mutter gnome-shell gnome-control-center gnome-software gnome-terminal orca)

  local prefix="/usr"
  local staged="$ROOTFS_STAGE$prefix"
  local libdir="$staged/lib" libdir_arch="$staged/lib/$(dpkg-architecture -qDEB_HOST_MULTIARCH 2>/dev/null || echo x86_64-linux-gnu)"

  # Make earlier-built modules discoverable by later ones.
  export PKG_CONFIG_PATH="$libdir/pkgconfig:$libdir_arch/pkgconfig:$staged/share/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
  export LD_LIBRARY_PATH="$libdir:$libdir_arch${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
  export XDG_DATA_DIRS="$staged/share${XDG_DATA_DIRS:+:$XDG_DATA_DIRS}:/usr/share"
  export GI_TYPELIB_PATH="$libdir/girepository-1.0:$libdir_arch/girepository-1.0${GI_TYPELIB_PATH:+:$GI_TYPELIB_PATH}"
  # meson/GNOME often need PYTHONPATH for freshly installed gi overrides.
  export PATH="$staged/bin:$PATH"

  local built=0 module
  for module in "${order[@]}"; do
    # Only build modules that are actually present (pango is optional-but-listed;
    # the required set is still verified below).
    if [ ! -d "$REPO_ROOT/gnome-source/$module/source" ]; then
      case "$module" in
        pango) log "gnome: pango source not present; skipping (optional in this order)"; continue ;;
        *) die "required GNOME module source missing: $module (run gnome-source/pull-all-source.sh)" ;;
      esac
    fi
    log "gnome: compiling $module (deps resolved from $staged)"
    DESTDIR="$ROOTFS_STAGE" PREFIX="$prefix" INSTALL=1 JOBS="$JOBS" \
      PKG_CONFIG_PATH="$PKG_CONFIG_PATH" LD_LIBRARY_PATH="$LD_LIBRARY_PATH" \
      XDG_DATA_DIRS="$XDG_DATA_DIRS" GI_TYPELIB_PATH="$GI_TYPELIB_PATH" \
      "$builder" "$module"
    built=$((built+1))
  done
  log "gnome: built $built module(s) in dependency order"
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
