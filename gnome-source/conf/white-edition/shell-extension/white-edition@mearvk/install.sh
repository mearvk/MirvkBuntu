#!/usr/bin/env bash
set -euo pipefail

# Ubuntu White Edition Panel - GNOME Shell extension installer.
#
# Installs the additive White Edition Start button extension into an ISO
# target root and compiles its GSettings schema in place. This installer
# takes an explicit target root and refuses to operate without one. It
# rejects symbolic links and special filesystem objects in the extension
# source and never deletes unrelated files from the target.
#
# Usage: install.sh <target-root>

UUID="white-edition@mearvk"
SRC_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
TARGET_ROOT="${1:-}"
EXT_ROOT="$TARGET_ROOT/usr/share/gnome-shell/extensions/$UUID"

[ -n "$TARGET_ROOT" ] || { echo "ERROR: target root is required." >&2; exit 2; }
[ -d "$TARGET_ROOT/etc" ] || { echo "ERROR: target root lacks /etc: $TARGET_ROOT" >&2; exit 2; }

case "$TARGET_ROOT" in
  /*) ;;
  *) echo "ERROR: target root must be an absolute path." >&2; exit 2 ;;
esac

[ -f "$SRC_DIR/metadata.json" ] || { echo "ERROR: extension source not found: $SRC_DIR" >&2; exit 1; }

# Never permit source links or special filesystem objects to influence the ISO.
if find -L "$SRC_DIR" -type l -print -quit | grep -q .; then
  echo "ERROR: symbolic links found under extension source; refusing installation." >&2
  exit 1
fi
if find "$SRC_DIR" \( -type b -o -type c -o -type p -o -type s \) -print -quit | grep -q .; then
  echo "ERROR: special filesystem object found in extension source." >&2
  exit 1
fi

mkdir -p "$EXT_ROOT/schemas" "$EXT_ROOT/logos"

# Top-level extension files. install.sh itself is not copied into the target.
install -m 0644 -- "$SRC_DIR/metadata.json"   "$EXT_ROOT/metadata.json"
install -m 0644 -- "$SRC_DIR/extension.js"    "$EXT_ROOT/extension.js"
install -m 0644 -- "$SRC_DIR/prefs.js"        "$EXT_ROOT/prefs.js"
install -m 0644 -- "$SRC_DIR/stylesheet.css"  "$EXT_ROOT/stylesheet.css"

# GSettings schema source.
install -m 0644 -- "$SRC_DIR/schemas/org.gnome.shell.extensions.white-edition.gschema.xml" \
                   "$EXT_ROOT/schemas/org.gnome.shell.extensions.white-edition.gschema.xml"

# Bundled Start button logos.
for svg in "$SRC_DIR"/logos/*.svg; do
  install -m 0644 -- "$svg" "$EXT_ROOT/logos/$(basename -- "$svg")"
done

# Compile the schema on the installed schemas directory. The compiled cache is
# produced here at install time and is never checked into the repository.
glib-compile-schemas "$EXT_ROOT/schemas"

printf 'Ubuntu White Edition Panel extension installed: %s\n' "$EXT_ROOT"
