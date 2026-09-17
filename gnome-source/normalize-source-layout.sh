#!/usr/bin/env bash
set -euo pipefail

# Ubuntu Determinant GNOME source-layout normalizer.
# Canonical build input is always gnome-source/<module>/source/.
# `upstream/` is transitional and is never a final source name.
ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
MODULES=(cairo gdk-pixbuf glib glib-networking gnome-control-center gnome-shell gnome-software gnome-terminal gtk gvfs mutter orca vala gala)

for module in "${MODULES[@]}"; do
  base="$ROOT_DIR/$module"; old="$base/upstream"; source="$base/source"
  [ -d "$base" ] || continue
  if [ -d "$old" ]; then
    if [ -e "$source" ] && [ ! -d "$source" ]; then
      echo "ERROR: $module: source exists but is not a directory" >&2; exit 1
    fi
    if [ -d "$source" ] && [ "$(find "$source" -mindepth 1 -maxdepth 1 -print -quit)" ]; then
      echo "ERROR: $module: refusing to merge non-empty upstream into non-empty source" >&2; exit 1
    fi
    mkdir -p "$source"
    if command -v rsync >/dev/null 2>&1; then rsync -a --links --safe-links "$old/" "$source/"; else cp -a "$old/." "$source/"; fi
    rm -rf --one-file-system "$old"
    echo "Normalized $module: upstream -> source"
  else
    mkdir -p "$source"
  fi
done

echo "GNOME source layout normalization complete: source/ is the canonical build boundary."
