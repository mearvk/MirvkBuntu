#!/usr/bin/env bash
set -euo pipefail

# Install the White Edition dconf source policy into an ISO root.
# This does not build a dconf database in the repository; the target image
# must run dconf update after installation.

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
TARGET_ROOT="${1:-/}"

command -v install >/dev/null 2>&1 || { echo "ERROR: install is required." >&2; exit 1; }

if [ ! -d "$ROOT_DIR/profile" ] || [ ! -d "$ROOT_DIR/db/local.d" ]; then
  echo "ERROR: incomplete White Edition configuration tree." >&2
  exit 1
fi

install -D -m 0644 "$ROOT_DIR/profile/user" \
  "$TARGET_ROOT/etc/dconf/profile/user"
install -D -m 0644 "$ROOT_DIR/db/local.d/00-white-edition" \
  "$TARGET_ROOT/etc/dconf/db/local.d/00-white-edition"

if command -v dconf >/dev/null 2>&1 && [ "$TARGET_ROOT" = "/" ]; then
  dconf update
else
  echo "Installed source keyfiles into $TARGET_ROOT."
  echo "Run 'dconf update' in the target root before first GNOME login."
fi
