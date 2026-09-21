#!/usr/bin/env bash
set -euo pipefail

# MirvkBuntu Chromium build (native path).
#
# Authoritative, self-contained release build of Chromium from the source tree
# vendored at userland/chromium/chromium-src/. It:
#   1. Bootstraps depot_tools (gn / autoninja / gclient) if not already present.
#   2. Runs gclient sync + runhooks so the checkout is buildable.
#   3. Generates a release GN build and compiles the `chrome` target.
#   4. Installs the result into DESTDIR (default: none = in place) with a
#      /usr/bin launcher, matching what the native gate stages into the rootfs.
#
# This is the single place the Chromium build is defined. The native gate
# (build/native-build.sh) and the thin userland/chromium/build.sh wrapper both
# funnel here so there is exactly one release build, never a debug/second pass.
#
# Environment:
#   DESTDIR            install root (e.g. the live rootfs staging tree). Empty =
#                      install under the source out/ dir only (no system install).
#   PREFIX             install prefix (default /usr).
#   JOBS / BUILD_JOBS  parallelism for autoninja.
#   CHROMIUM_SRC       override source root (default: repo userland/chromium/chromium-src).
#   CHROMIUM_OUT       override GN out dir (default: <src>/out/MirvkBuntuRelease).
#   DEPOT_TOOLS        override depot_tools location.
#   CHROMIUM_TAG       if set and source is missing, fetch this tag first.
#   SKIP_SYNC=1        skip gclient sync/runhooks (source already synced).
#
# Requirements: git, python3, and ~16 GB RAM / ~100 GB disk / hours of build time.

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(CDPATH= cd -- "${SCRIPT_DIR}/../.." && pwd)"

CHROMIUM_ROOT="${CHROMIUM_ROOT:-${REPO_ROOT}/userland/chromium}"
CHROMIUM_SRC="${CHROMIUM_SRC:-${CHROMIUM_ROOT}/chromium-src}"
CHROMIUM_OUT="${CHROMIUM_OUT:-${CHROMIUM_SRC}/out/MirvkBuntuRelease}"
DEPOT_TOOLS="${DEPOT_TOOLS:-${CHROMIUM_ROOT}/depot_tools}"
PREFIX="${PREFIX:-/usr}"
DESTDIR="${DESTDIR:-}"
JOBS="${JOBS:-${BUILD_JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)}}"

die(){ printf 'chromium-build: ERROR: %s\n' "$*" >&2; exit 1; }
log(){ printf 'chromium-build: %s\n' "$*"; }

command -v git >/dev/null 2>&1 || die "git is required"
command -v python3 >/dev/null 2>&1 || die "python3 is required"

# --- 1. source tree ----------------------------------------------------------
if [ ! -f "$CHROMIUM_SRC/DEPS" ] || [ ! -f "$CHROMIUM_SRC/BUILD.gn" ]; then
  if [ -n "${CHROMIUM_TAG:-}" ] && [ -x "$CHROMIUM_ROOT/fetch-chromium.sh" ]; then
    log "source missing; fetching Chromium tag $CHROMIUM_TAG"
    bash "$CHROMIUM_ROOT/fetch-chromium.sh" --tag "$CHROMIUM_TAG" "$CHROMIUM_SRC"
  else
    die "Chromium source not found at $CHROMIUM_SRC (expected DEPS + BUILD.gn). Fetch it with userland/chromium/fetch-chromium.sh, or set CHROMIUM_TAG."
  fi
fi
# Sanity: this must be the real source root, not a nested fixture.
for d in chrome content components third_party base; do
  [ -d "$CHROMIUM_SRC/$d" ] || die "not a Chromium source root (missing $d/): $CHROMIUM_SRC"
done
[ -f "$CHROMIUM_SRC/chrome/BUILD.gn" ] || die "missing chrome/BUILD.gn under $CHROMIUM_SRC"

# --- 2. depot_tools ----------------------------------------------------------
if [ ! -x "$DEPOT_TOOLS/gn" ] && ! (PATH="$DEPOT_TOOLS:$PATH" command -v gn >/dev/null 2>&1); then
  if [ -x "$CHROMIUM_ROOT/install-gn.sh" ]; then
    log "bootstrapping depot_tools via install-gn.sh"
    DEPOT_TOOLS="$DEPOT_TOOLS" "$CHROMIUM_ROOT/install-gn.sh"
  elif [ ! -d "$DEPOT_TOOLS/.git" ]; then
    log "cloning depot_tools"
    git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git "$DEPOT_TOOLS"
  fi
fi
export PATH="$DEPOT_TOOLS:$PATH"
command -v gn >/dev/null 2>&1 || die "gn unavailable after depot_tools bootstrap"
command -v autoninja >/dev/null 2>&1 || die "autoninja unavailable after depot_tools bootstrap"

# --- 3. sync + hooks ---------------------------------------------------------
if [ "${SKIP_SYNC:-0}" != "1" ]; then
  if command -v gclient >/dev/null 2>&1 && [ -f "$CHROMIUM_SRC/../.gclient" -o -f "$CHROMIUM_SRC/.gclient" ]; then
    log "running gclient sync + runhooks (this can take a long time)"
    ( cd "$CHROMIUM_SRC" && gclient sync --no-history --shallow ) || die "gclient sync failed"
    ( cd "$CHROMIUM_SRC" && gclient runhooks ) || die "gclient runhooks failed"
  else
    log "no .gclient found; skipping sync (assuming a complete checkout)"
  fi
fi

# --- 4. generate + build -----------------------------------------------------
log "source : $CHROMIUM_SRC"
log "out    : $CHROMIUM_OUT"
log "jobs   : $JOBS"

mkdir -p "$CHROMIUM_OUT"
# Release build args. use_siso keeps the build fast where available.
cat > "$CHROMIUM_OUT/args.gn" <<'ARGS'
# MirvkBuntu Chromium release build.
is_debug = false
is_component_build = false
symbol_level = 0
blink_symbol_level = 0
v8_symbol_level = 0
is_official_build = false
is_chrome_branded = false
enable_nacl = false
use_cups = true
use_dbus = true
use_gio = true
use_pulseaudio = true
ARGS
if [ -n "${TARGET_CPU:-}" ]; then
  printf 'target_cpu = "%s"\n' "$TARGET_CPU" >> "$CHROMIUM_OUT/args.gn"
fi

( cd "$CHROMIUM_SRC" && gn gen "$CHROMIUM_OUT" )
( cd "$CHROMIUM_SRC" && autoninja -C "$CHROMIUM_OUT" -j "$JOBS" chrome )

[ -x "$CHROMIUM_OUT/chrome" ] || die "build produced no chrome executable at $CHROMIUM_OUT/chrome"
log "built: $CHROMIUM_OUT/chrome"
file "$CHROMIUM_OUT/chrome" 2>/dev/null || true
# Fail on unresolved shared libraries (a broken/partial build).
if command -v ldd >/dev/null 2>&1; then
  if ldd "$CHROMIUM_OUT/chrome" 2>&1 | grep -q "not found"; then
    die "chrome has unresolved shared libraries"
  fi
fi

# --- 5. install --------------------------------------------------------------
# Layout matches build/native-build.sh: /opt/mirvkbuntu/chromium + a launcher in
# <PREFIX>/bin. When DESTDIR is empty the install lands on the live system root.
INSTALL_LIBDIR="${DESTDIR}/opt/mirvkbuntu/chromium"
INSTALL_BINDIR="${DESTDIR}${PREFIX}/bin"

log "installing to ${INSTALL_LIBDIR}"
mkdir -p "$INSTALL_LIBDIR" "$INSTALL_BINDIR"
# Copy the runtime payload (chrome + its resources/libraries).
cp -a "$CHROMIUM_OUT/." "$INSTALL_LIBDIR/"

# Launcher.
cat > "$INSTALL_BINDIR/mirvkbuntu-chrome" <<'LAUNCH'
#!/usr/bin/env bash
exec /opt/mirvkbuntu/chromium/chrome "$@"
LAUNCH
chmod 0755 "$INSTALL_BINDIR/mirvkbuntu-chrome"

# Friendly aliases so the desktop and users find it under common names.
ln -sf mirvkbuntu-chrome "$INSTALL_BINDIR/chromium" 2>/dev/null || true
ln -sf mirvkbuntu-chrome "$INSTALL_BINDIR/chromium-browser" 2>/dev/null || true

log "done. installed chrome + launcher (mirvkbuntu-chrome, chromium, chromium-browser)."
