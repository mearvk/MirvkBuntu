#!/usr/bin/env bash
set -euo pipefail

# Ubuntu White Edition — initial Desktop LAF icon installer.
# Source of truth: ubuntu-white/icons/set-002/*.png
#
# Set-002 is the active development artwork set. This installer deliberately
# consumes PNG files from that directory and does not substitute SVG artwork.
# It validates the source tree before copying and never follows symlinks.
#
# Usage: install-icons.sh <target-root>

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/../../../../" && pwd)"
ICON_ROOT="$ROOT_DIR/ubuntu-white/icons/set-002"
TARGET_ROOT="${1:-}"
THEME_ROOT="$TARGET_ROOT/usr/share/icons/Ubuntu-White"

[ -n "$TARGET_ROOT" ] || { echo "ERROR: target root is required." >&2; exit 2; }
[ -d "$TARGET_ROOT/etc" ] || { echo "ERROR: target root lacks /etc: $TARGET_ROOT" >&2; exit 2; }
[ -d "$ICON_ROOT" ] || { echo "ERROR: PNG icon source not found: $ICON_ROOT" >&2; exit 1; }

case "$TARGET_ROOT" in
  /*) ;;
  *) echo "ERROR: target root must be an absolute path." >&2; exit 2 ;;
esac

# Never permit source links or special filesystem objects to influence the ISO.
if find -L "$ICON_ROOT" -type l -print -quit | grep -q .; then
  echo "ERROR: symbolic links found under set-002; refusing installation." >&2
  exit 1
fi
if find "$ICON_ROOT" \( -type b -o -type c -o -type p -o -type s \) -print -quit | grep -q .; then
  echo "ERROR: special filesystem object found in set-002." >&2
  exit 1
fi

# PNG-only production boundary. Ignore README/source metadata and reject
# unexpected image formats rather than silently mixing development assets.
if find "$ICON_ROOT" -type f ! -name '*.png' -print -quit | grep -q .; then
  echo "ERROR: non-PNG file found in set-002; refusing production installation." >&2
  exit 1
fi

mkdir -p "$THEME_ROOT/48x48/places" \
         "$THEME_ROOT/48x48/apps" \
         "$THEME_ROOT/48x48/actions"

copy_icon() {
  local name="$1" category="$2"
  local src="$ICON_ROOT/$name.png"
  [ -f "$src" ] || return 0
  install -m 0644 -- "$src" "$THEME_ROOT/48x48/$category/$name.png"
}

# Initial Desktop LAF allow-list. Expand only through an explicit reviewed
# change as the set-002 artwork develops.
copy_icon folder places
copy_icon home places
copy_icon trash places
copy_icon terminal apps
copy_icon settings apps
copy_icon downloads actions

install -m 0644 -- "$(dirname -- "$0")/icon-theme/index.theme" "$THEME_ROOT/index.theme"

printf 'Ubuntu White Edition PNG icon theme installed from set-002: %s\n' "$THEME_ROOT"
