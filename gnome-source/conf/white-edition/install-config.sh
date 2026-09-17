#!/usr/bin/env bash
set -euo pipefail

# Install the human-readable White Edition dconf configuration into an ISO
# target root. The target must contain an /etc directory. The script does not
# copy compiled dconf databases; it installs source keyfiles and then invokes
# dconf update against the target root when the tool supports --root.

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
TARGET_ROOT="${1:-/}"

if [ ! -d "$TARGET_ROOT/etc" ]; then
  echo "ERROR: target root does not contain /etc: $TARGET_ROOT" >&2
  exit 2
fi

command -v dconf >/dev/null 2>&1 || {
  echo "ERROR: dconf is required to compile the White Edition configuration." >&2
  exit 1
}

mkdir -p "$TARGET_ROOT/etc/dconf/profile" "$TARGET_ROOT/etc/dconf/db/local.d"
install -m 0644 "$ROOT_DIR/profile/user" "$TARGET_ROOT/etc/dconf/profile/user"
install -m 0644 "$ROOT_DIR/db/local.d/00-white-edition" "$TARGET_ROOT/etc/dconf/db/local.d/00-white-edition"

# dconf update supports a root prefix on modern dconf builds. If unavailable,
# refuse rather than accidentally modifying the build host's /etc/dconf.
if dconf update --help 2>&1 | grep -q -- '--root'; then
  dconf update --root="$TARGET_ROOT"
else
  echo "ERROR: installed dconf does not advertise --root; refusing host update." >&2
  exit 1
fi

echo "Ubuntu White Edition GNOME configuration installed in: $TARGET_ROOT"
