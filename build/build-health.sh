#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
MODE="${1:-preflight}"

log(){ printf "build-health: %s\n" "$*"; }
warn(){ printf "build-health: WARNING: %s\n" "$*" >&2; }
die(){ printf "build-health: ERROR: %s\n" "$*" >&2; exit 1; }
has(){ command -v "$1" >/dev/null 2>&1; }

cd "$REPO_ROOT"
case "$MODE" in preflight|strict) ;; *) die "usage: $0 [preflight|strict]" ;; esac

for d in kernels file-systems sources packages userland user-interface gnome-source docs build installer builder scout; do
  [ -d "$d" ] || die "required source directory missing: $d"
done
[ -f packages/basic-packages.txt ] || die "packages/basic-packages.txt is missing"
[ -f build/CUSTOM_COMPONENTS.txt ] || die "build/CUSTOM_COMPONENTS.txt is missing"
for p in slim minimum full; do [ -f "build/profiles/$p.profile" ] || die "profile definition missing: build/profiles/$p.profile"; done
for f in build/native-build.sh build/common-build.sh build/component-inventory.sh build/build-slim.sh build/build-minimal.sh build/build-desktop.sh build/release-manifest.sh build/validate-release.sh; do
  [ -f "$f" ] || die "required build file missing: $f"
done
for c in bash git make sha256sum find awk sort; do has "$c" || die "required host command missing: $c"; done
git rev-parse --is-inside-work-tree >/dev/null 2>&1 || die "not a Git working tree"

if [ -e output ] && [ ! -L output ]; then warn "output exists but is not a symlink; build scripts require output -> build/output"; fi
if [ -L output ]; then
  [ "$(readlink output)" = "$REPO_ROOT/build/output" ] || die "output points somewhere other than build/output"
fi

for f in build/build-slim.sh build/build-minimal.sh build/build-desktop.sh; do
  if grep -nE "quick-remaster|SOURCE_ISO|ubuntu[^[:space:]]*\.iso|\.iso[^[:space:]]*input" "$f" >/dev/null 2>&1; then
    die "primary profile script references an existing ISO/remaster input: $f"
  fi
done

log "source directories: OK"
log "package manifest: OK"
log "profile definitions: OK"
log "build orchestration files: OK"
log "host command baseline: OK"
log "Git checkout: OK"
log "custom source authority: OK"
log "existing Ubuntu ISO is not a primary build input: OK"

if has live-build && has lb && has xorriso && has file; then
  log "ISO assembly/validation tooling: READY"
else
  warn "ISO assembly tooling is not fully installed; run: make prereqs"
fi

if [ -d build/work/native/rootfs ] && [ -d build/work/native/artifacts/packages ]; then
  log "native staging tree: PRESENT"
else
  warn "native staging tree: not built yet; expected on a fresh checkout"
fi

if [ "$MODE" = strict ]; then
  [ -d build/work/native/rootfs ] || die "strict health requires build/work/native/rootfs"
  [ -d build/work/native/artifacts/packages ] || die "strict health requires native Debian packages"
  [ -f build/work/component-inventory.txt ] || die "strict health requires component inventory"
  [ -f build/work/dependency-cache.txt ] || die "strict health requires dependency cache"
  [ -f build/work/dependency-resolved.txt ] || die "strict health requires resolved dependency record"
  has live-build && has lb || die "strict health requires live-build/lb"
  has xorriso || die "strict health requires xorriso"
  has file || die "strict health requires file"
  log "native staging: READY"
  log "dependency records: READY"
  log "ISO tooling: READY"
fi
log "build health: $MODE READY"