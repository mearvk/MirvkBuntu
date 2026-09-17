#!/usr/bin/env bash
set -euo pipefail

# MirvkBuntu basic GNOME source acquisition.
# The module set follows the GNOME source organization already used by
# Ubuntu.Determinant.Beta.Restricted.
#
# Interactive HTTPS authentication is intentionally enabled. GitHub accepts
# the GitHub username at the username prompt and a personal access token at
# the password prompt; ordinary GitHub account passwords are not accepted
# for Git HTTPS authentication.
unset GIT_ASKPASS SSH_ASKPASS
export GIT_TERMINAL_PROMPT=1

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
DEST="${ROOT}/sources/work/gnome"

mkdir -p "$DEST"

# Keep the initial set deliberately small: these are the platform and desktop
# pieces needed before adding applications and optional GNOME modules.
declare -A REPOS=(
  [cairo]="https://gitlab.gnome.org/GNOME/cairo.git"
  [glib]="https://gitlab.gnome.org/GNOME/glib.git"
  [gdk-pixbuf]="https://gitlab.gnome.org/GNOME/gdk-pixbuf.git"
  [gtk]="https://gitlab.gnome.org/GNOME/gtk.git"
  [mutter]="https://gitlab.gnome.org/GNOME/mutter.git"
  [gnome-shell]="https://gitlab.gnome.org/GNOME/gnome-shell.git"
)

command -v git >/dev/null 2>&1 || { echo "git is required" >&2; exit 1; }

for name in "${!REPOS[@]}"; do
    url="${REPOS[$name]}"
    target="${DEST}/${name}"
    if [[ -d "${target}/.git" ]]; then
        echo "Updating ${name}"
        git -C "$target" fetch --tags --prune
    else
        echo "Cloning ${name} from ${url}"
        git clone --depth 1 "$url" "$target"
    fi
    git -C "$target" rev-parse HEAD > "${target}.revision"
done

echo "GNOME core sources acquired under ${DEST}"
