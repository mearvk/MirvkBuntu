#!/usr/bin/env bash
set -euo pipefail

# MirvkBuntu GNOME source acquisition orchestrator.
#
# Runs each GNOME module's own pull-source.sh, then verifies every requested
# module ends with a usable source tree at gnome-source/<module>/source/ (a
# recognized build file: meson.build / configure / configure.ac / CMakeLists.txt
# / setup.py). Each module's pull-source.sh writes directly into source/ and is
# idempotent, so this orchestrator just drives them and checks the result.
#
# Version pinning:
#   Pins are read from gnome-source/GNOME_VERSIONS (KEY=VALUE lines, '#'
#   comments). Each line names a module's ref env var, e.g.
#       GLIB_REF=2.80.4
#       GTK_REF=4.14.5
#   Anything set in GNOME_VERSIONS is exported before the module's
#   pull-source.sh runs, so the module fetches that exact ref. Values already
#   present in the environment take precedence over the file. Without a pin, a
#   module fetches its upstream default branch.
#
# Usage:
#   bash gnome-source/pull-all-source.sh                 # the desktop-required set
#   bash gnome-source/pull-all-source.sh glib gtk        # only these modules
#   MODULES="glib gtk mutter" bash gnome-source/pull-all-source.sh
#   FORCE=1 bash gnome-source/pull-all-source.sh gtk     # re-fetch even if present
#
# Network access to the upstream source servers is required.

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
VERSIONS_FILE="${GNOME_VERSIONS_FILE:-$ROOT_DIR/GNOME_VERSIONS}"

die(){ printf 'pull-all: ERROR: %s\n' "$*" >&2; exit 1; }
log(){ printf 'pull-all: %s\n' "$*"; }

command -v git >/dev/null 2>&1 || die "git is required"

# The modules build/native-build.sh:build_gnome() requires for a full desktop.
DEFAULT_MODULES=(cairo glib gdk-pixbuf gtk glib-networking gvfs mutter gnome-shell gnome-control-center gnome-software gnome-terminal orca)

# Load pinned refs from GNOME_VERSIONS into the environment (without clobbering
# values already set by the caller).
load_pins(){
  [ -f "$VERSIONS_FILE" ] || { log "no pin file ($VERSIONS_FILE); using upstream defaults"; return 0; }
  log "loading version pins from $VERSIONS_FILE"
  local line key val
  while IFS= read -r line || [ -n "$line" ]; do
    line="${line%%#*}"                      # strip comments
    line="${line#"${line%%[![:space:]]*}"}" # ltrim
    [ -z "$line" ] && continue
    case "$line" in
      *=*) key="${line%%=*}"; val="${line#*=}"
           key="${key//[[:space:]]/}"; val="${val//[[:space:]]/}"
           [ -z "$key" ] && continue
           # Do not override an explicit environment value.
           if [ -z "${!key:-}" ]; then export "$key=$val"; fi
           ;;
    esac
  done < "$VERSIONS_FILE"
}

has_build_file(){
  local d="$1" f
  for f in meson.build configure configure.ac CMakeLists.txt setup.py pyproject.toml; do
    [ -e "$d/$f" ] && return 0
  done
  return 1
}

load_pins

# Resolve the module list.
if [ "$#" -gt 0 ]; then
  MODULES_LIST=( "$@" )
elif [ -n "${MODULES:-}" ]; then
  # shellcheck disable=SC2206
  MODULES_LIST=( ${MODULES} )
else
  MODULES_LIST=( "${DEFAULT_MODULES[@]}" )
fi

log "modules: ${MODULES_LIST[*]}"

failed=()
skipped=()
pulled=()

for module in "${MODULES_LIST[@]}"; do
  base="$ROOT_DIR/$module"
  src="$base/source"
  [ -d "$base" ] || { log "WARNING: unknown module directory: $module (skipping)"; skipped+=( "$module" ); continue; }

  # Already usable? Skip the network fetch unless FORCE=1.
  if [ "${FORCE:-0}" != "1" ] && [ -d "$src" ] && has_build_file "$src"; then
    log "$module: source/ already usable; skipping fetch (FORCE=1 to re-fetch)"
    continue
  fi

  [ -x "$base/pull-source.sh" ] || { log "WARNING: $module has no executable pull-source.sh (skipping)"; skipped+=( "$module" ); continue; }

  log "==> $module: running pull-source.sh"
  if ! ( cd "$base" && ./pull-source.sh ); then
    log "WARNING: $module: pull-source.sh failed"
    failed+=( "$module" )
    continue
  fi

  # Fold any transitional upstream/ layout into source/ (defensive; current
  # module scripts already write source/ directly).
  if [ ! -d "$src" ] && [ -x "$ROOT_DIR/normalize-source-layout.sh" ]; then
    "$ROOT_DIR/normalize-source-layout.sh" >/dev/null 2>&1 || true
  fi

  if [ -d "$src" ] && has_build_file "$src"; then
    log "$module: source/ ready"
    pulled+=( "$module" )
  else
    log "WARNING: $module: source/ still not usable after pull"
    failed+=( "$module" )
  fi
done

echo
log "summary:"
[ "${#pulled[@]}"  -gt 0 ] && log "  pulled : ${pulled[*]}"
[ "${#skipped[@]}" -gt 0 ] && log "  skipped: ${skipped[*]}"
[ "${#failed[@]}"  -gt 0 ] && log "  FAILED : ${failed[*]}"

# Final verification against the requested set.
missing=()
for module in "${MODULES_LIST[@]}"; do
  [ -d "$ROOT_DIR/$module" ] || continue
  if [ ! -d "$ROOT_DIR/$module/source" ] || ! has_build_file "$ROOT_DIR/$module/source"; then
    missing+=( "$module" )
  fi
done

if [ "${#missing[@]}" -gt 0 ]; then
  log "the following modules still lack a usable source/: ${missing[*]}"
  die "GNOME source acquisition incomplete"
fi

log "all requested GNOME module source trees are present and usable."
