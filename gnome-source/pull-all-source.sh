#!/usr/bin/env bash
set -euo pipefail

# MirvkBuntu GNOME source acquisition orchestrator.
#
# Runs each GNOME module's own pull-source.sh, normalizes the layout so the
# canonical build input is always gnome-source/<module>/source/, and verifies
# every requested module ends with a usable source tree (a recognized build
# file: meson.build / configure / configure.ac / CMakeLists.txt).
#
# Some module pull-source.sh scripts clone the upstream tree into the MODULE
# ROOT (glib, gdk-pixbuf, gtk, mutter, gnome-shell, pango) rather than into
# source/. This orchestrator relocates that content into source/ so it matches
# what build/native-build.sh (build_gnome) and gnome-source/build-module.sh
# require. Modules that already populate source/ (cairo, glib-networking, gvfs,
# orca, gnome-control-center, gnome-software, gnome-terminal) are left as-is.
#
# Usage:
#   bash gnome-source/pull-all-source.sh                 # the desktop-required set
#   bash gnome-source/pull-all-source.sh glib gtk        # only these modules
#   MODULES="glib gtk mutter" bash gnome-source/pull-all-source.sh
#
# Network access to the GNOME/upstream source servers is required.

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

die(){ printf 'pull-all: ERROR: %s\n' "$*" >&2; exit 1; }
log(){ printf 'pull-all: %s\n' "$*"; }

command -v git >/dev/null 2>&1 || die "git is required"
command -v curl >/dev/null 2>&1 || die "curl is required"
command -v tar  >/dev/null 2>&1 || die "tar is required"

# The modules build/native-build.sh:build_gnome() requires for a full desktop.
DEFAULT_MODULES=(cairo glib gdk-pixbuf gtk glib-networking gvfs mutter gnome-shell gnome-control-center gnome-software gnome-terminal orca)

# Modules whose pull-source.sh clones into the module ROOT and therefore need
# their content relocated into source/ afterward.
is_clone_to_root(){
  case "$1" in
    glib|gdk-pixbuf|gtk|mutter|gnome-shell|pango) return 0 ;;
    *) return 1 ;;
  esac
}

# Recognized build entrypoints that mark a tree as "usable".
has_build_file(){
  local d="$1" f
  for f in meson.build configure configure.ac CMakeLists.txt setup.py pyproject.toml; do
    [ -e "$d/$f" ] && return 0
  done
  return 1
}

# Relocate an upstream tree that a pull script dropped in the module root into
# source/, without disturbing MirvkBuntu's own module files (pull-source.sh,
# README.md, build*, .git*, source/ itself).
relocate_root_into_source(){
  local base="$1" src="$base/source" entry name
  # If source/ already usable, nothing to do.
  if [ -d "$src" ] && has_build_file "$src"; then
    return 0
  fi
  # Only relocate if the root actually received an upstream build tree.
  has_build_file "$base" || return 1
  mkdir -p "$src"
  shopt -s dotglob
  for entry in "$base"/*; do
    name="$(basename "$entry")"
    case "$name" in
      source|build|build-local|build-aux|pull-source.sh|README.md|SOURCE-INFO.txt|UPSTREAM.md|.git|.gitkeep|.gitignore|*.sha256|*.tar.*)
        continue ;;
    esac
    # Move everything else (the upstream tree) into source/.
    mv -f "$entry" "$src/"
  done
  shopt -u dotglob
  has_build_file "$src"
}

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

  # Already usable? Skip the network fetch.
  if [ -d "$src" ] && has_build_file "$src"; then
    log "$module: source/ already usable; skipping fetch"
    continue
  fi

  [ -x "$base/pull-source.sh" ] || { log "WARNING: $module has no executable pull-source.sh (skipping)"; skipped+=( "$module" ); continue; }

  log "==> $module: running pull-source.sh"
  if ! ( cd "$base" && ./pull-source.sh ); then
    log "WARNING: $module: pull-source.sh failed"
    failed+=( "$module" )
    continue
  fi

  # Reconcile layout: clone-to-root modules need relocation into source/.
  if is_clone_to_root "$module"; then
    if ! relocate_root_into_source "$base"; then
      log "WARNING: $module: could not establish a usable source/ after pull"
      failed+=( "$module" )
      continue
    fi
  fi

  # Also run the shared normalizer to fold any upstream/ layout into source/.
  if [ -x "$ROOT_DIR/normalize-source-layout.sh" ]; then
    "$ROOT_DIR/normalize-source-layout.sh" >/dev/null 2>&1 || true
  fi

  if [ -d "$src" ] && has_build_file "$src"; then
    log "$module: source/ ready"
    pulled+=( "$module" )
  else
    log "WARNING: $module: source/ still not usable after pull + normalize"
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
