#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
log(){ printf 'build-health: %s\n' "$*"; }
warn(){ printf 'build-health: WARNING: %s\n' "$*" >&2; }
die(){ printf 'build-health: ERROR: %s\n' "$*" >&2; exit 1; }
cd "$REPO_ROOT"
for d in kernels file-systems sources packages userland user-interface gnome-source docs build installer builder scout; do
  [ -d "$d" ] || die "required source directory missing: $d"
done
[ -f packages/basic-packages.txt ] || die "packages/basic-packages.txt is missing"
[ -f build/CUSTOM_COMPONENTS.txt ] || die "build/CUSTOM_COMPONENTS.txt is missing"
for p in slim minimum full; do
  [ -f "build/profiles/$p.profile" ] || die "profile definition missing: build/profiles/$p.profile"
done
for c in bash git make sha256sum find; do
  command -v "$c" >/dev/null 2>&1 || die "required host command missing: $c"
done
git rev-parse --is-inside-work-tree >/dev/null 2>&1 || die "not a Git working tree"
if [ -e output ] && [ ! -L output ]; then warn "output exists but is not a symlink; canonical artifacts remain in build/output/"; fi
if grep -RInE 'quick-remaster|SOURCE_ISO|ubuntu.*\.iso' build/build-slim.sh build/build-minimal.sh build/build-desktop.sh >/dev/null 2>&1; then
  die "a primary Slim/Minimum/Full build script references an existing-ISO/remaster input"
fi
log "source directories: OK"
log "profile definitions: OK"
log "host command baseline: OK"
log "custom source authority: OK"
log "Slim/Minimum/Full do not consume an existing ISO: OK"
log "build health: READY FOR NATIVE/ISO PIPELINE"
