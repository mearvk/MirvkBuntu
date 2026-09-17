#!/usr/bin/env bash
set -euo pipefail

# Ubuntu White Edition — install the boot-to-desktop presentation chain into an
# ISO target root.
#
# This wires the three pre-session stages (GRUB menu, Plymouth splash, GDM
# greeter + "Ubuntu White" session offering) into a target filesystem so the
# white-skinned presentation is continuous from power-on to the desktop. The
# desktop-session layer is installed separately by ../install-config.sh.
#
# It follows the same safety contract as ../install-config.sh:
#   * requires an explicit target root that contains /etc;
#   * never touches the build host's own / unless the target root IS /;
#   * installs source config/themes only (no compiled databases in the repo);
#   * records every installed path for reversibility/audit;
#   * refuses rather than guessing when a required tool is missing.
#
# Activation commands that must run INSIDE the target (update-grub,
# update-initramfs, dconf update) are executed when the target is the running
# system, and otherwise printed as the exact required post-install steps. This
# avoids running host-scoped generators against a directory tree that is not the
# active root.

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
TARGET_ROOT="${1:-}"

usage() {
  cat >&2 <<EOF
Usage: $0 <target-root>

  <target-root>   ISO/install target filesystem root (must contain /etc).
                  Use "/" only to configure the running system itself.

Installs the Ubuntu White Edition boot presentation chain:
  - GRUB white theme + default entry      -> /boot/grub/themes/white-edition, /etc/default/grub.d
  - Plymouth white splash                 -> /usr/share/plymouth/themes/white-edition
  - GDM white greeter + "Ubuntu White"    -> /etc/dconf/db/gdm.d, session + desktop files
EOF
  exit 2
}

[ -n "$TARGET_ROOT" ] || usage
[ -d "$TARGET_ROOT/etc" ] || { echo "ERROR: target root has no /etc: $TARGET_ROOT" >&2; exit 2; }

# Absolute, normalized target root.
TARGET_ROOT="$(CDPATH= cd -- "$TARGET_ROOT" && pwd)"
IS_LIVE_SYSTEM=0
[ "$TARGET_ROOT" = "/" ] && IS_LIVE_SYSTEM=1

MANIFEST="$TARGET_ROOT/etc/ubuntu-white/boot-presentation.installed"
mkdir -p "$TARGET_ROOT/etc/ubuntu-white"
: > "$MANIFEST"
record() { echo "$1" >> "$MANIFEST"; }

log()  { echo "[white-edition/boot] $*"; }
need() { command -v "$1" >/dev/null 2>&1; }

install_file() { # src dest mode
  local src="$1" dest="$2" mode="${3:-0644}"
  mkdir -p "$(dirname -- "$dest")"
  install -m "$mode" "$src" "$dest"
  record "$dest"
  log "installed $dest"
}

# ---------------------------------------------------------------------------
# 1. GRUB
# ---------------------------------------------------------------------------
install_grub() {
  log "GRUB white menu theme"
  local theme_dir="$TARGET_ROOT/boot/grub/themes/white-edition"
  mkdir -p "$theme_dir"
  install_file "$ROOT_DIR/grub/theme/theme.txt" "$theme_dir/theme.txt"
  install_file "$ROOT_DIR/grub/05_white_edition" "$TARGET_ROOT/etc/default/grub.d/05_white_edition"

  # Generate the 9-slice pixmaps referenced by theme.txt from the token colors,
  # if a converter is available and they are not already present. This keeps the
  # theme rendering on a fresh build host. Absence is non-fatal: GRUB falls back
  # to the solid colors declared in theme.txt.
  if need convert; then
    # White row, Ubuntu-red selected row, red progress fill, gray track.
    convert -size 8x8 xc:'#fafafa'      "$theme_dir/item_c.png"           2>/dev/null && record "$theme_dir/item_c.png" || true
    convert -size 8x8 xc:'#e95420'      "$theme_dir/selected_item_c.png"  2>/dev/null && record "$theme_dir/selected_item_c.png" || true
    convert -size 8x8 xc:'#dddddd'      "$theme_dir/progress_bg_c.png"    2>/dev/null && record "$theme_dir/progress_bg_c.png" || true
    convert -size 8x8 xc:'#e95420'      "$theme_dir/progress_fg_c.png"    2>/dev/null && record "$theme_dir/progress_fg_c.png" || true
    log "generated GRUB theme pixmaps"
  else
    log "note: 'convert' (ImageMagick) not found; GRUB uses solid theme colors (still white-skinned)."
  fi

  # Convert a system font to the .pf2 GRUB needs, if possible.
  if need grub-mkfont; then
    for f in /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf \
             /usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf; do
      [ -f "$f" ] || continue
      base="$(basename "${f%.ttf}")"
      grub-mkfont -s 16 -o "$theme_dir/${base}.pf2" "$f" 2>/dev/null \
        && record "$theme_dir/${base}.pf2" || true
    done
  fi
}

# ---------------------------------------------------------------------------
# 2. Plymouth
# ---------------------------------------------------------------------------
install_plymouth() {
  log "Plymouth white splash"
  local dest="$TARGET_ROOT/usr/share/plymouth/themes/white-edition"
  install_file "$ROOT_DIR/plymouth/white-edition/white-edition.plymouth" "$dest/white-edition.plymouth"
  install_file "$ROOT_DIR/plymouth/white-edition/white-edition.script"   "$dest/white-edition.script"

  # Optional White Edition mark. The icon source of truth is ubuntu-white/icons/;
  # if a boot logo exists there, copy it in as logo.png. The script draws a text
  # mark when logo.png is absent, so this is optional.
  local repo_root; repo_root="$(cd "$ROOT_DIR/../../../.." && pwd)"
  for cand in "$repo_root/ubuntu-white/icons/white-edition-logo.png" \
              "$repo_root/ubuntu-white/icons/logo.png"; do
    if [ -f "$cand" ]; then
      install_file "$cand" "$dest/logo.png"
      break
    fi
  done
}

# ---------------------------------------------------------------------------
# 3. GDM greeter + "Ubuntu White" session offering
# ---------------------------------------------------------------------------
install_gdm() {
  log "GDM white greeter + Ubuntu White session"
  # Greeter dconf defaults (GDM's own profile).
  install_file "$ROOT_DIR/gdm/10-white-edition" "$TARGET_ROOT/etc/dconf/db/gdm.d/10-white-edition"

  # Session offering shown in the greeter chooser (Wayland and X11 lists).
  install_file "$ROOT_DIR/gdm/ubuntu-white.desktop" "$TARGET_ROOT/usr/share/wayland-sessions/ubuntu-white.desktop"
  install_file "$ROOT_DIR/gdm/ubuntu-white.desktop" "$TARGET_ROOT/usr/share/xsessions/ubuntu-white.desktop"

  # The gnome-session definition the offering launches.
  install_file "$ROOT_DIR/gdm/ubuntu-white.session" "$TARGET_ROOT/usr/share/gnome-session/sessions/ubuntu-white.session"

  # Make "Ubuntu White" the default session for existing AccountsService users
  # (installer-created users). This is a default, not a lock.
  local as_dir="$TARGET_ROOT/var/lib/AccountsService/users"
  if [ -d "$as_dir" ]; then
    for uf in "$as_dir"/*; do
      [ -f "$uf" ] || continue
      if grep -q '^\[User\]' "$uf" 2>/dev/null; then
        # Set Session/XSession without duplicating keys.
        sed -i '/^Session=/d; /^XSession=/d' "$uf"
        printf 'Session=ubuntu-white\nXSession=ubuntu-white\n' >> "$uf"
        log "set default session -> ubuntu-white for $(basename "$uf")"
      fi
    done
  else
    log "note: no AccountsService users yet; installer should set Session=ubuntu-white when it creates the first user."
  fi
}

# ---------------------------------------------------------------------------
# Activation (must run inside the target root)
# ---------------------------------------------------------------------------
activate_or_instruct() {
  echo
  log "activation steps (inside the target root):"
  if [ "$IS_LIVE_SYSTEM" -eq 1 ]; then
    need dconf          && { dconf update || true; }                  || log "  (dconf missing) run: dconf update"
    need update-grub    && { update-grub || true; }                   || log "  (update-grub missing) run: update-grub"
    if need plymouth-set-default-theme; then
      plymouth-set-default-theme white-edition || true
      need update-initramfs && { update-initramfs -u || true; } || log "  run: update-initramfs -u"
    else
      log "  run: plymouth-set-default-theme white-edition && update-initramfs -u"
    fi
  else
    cat <<EOF
  chroot "$TARGET_ROOT" dconf update
  chroot "$TARGET_ROOT" update-grub
  chroot "$TARGET_ROOT" plymouth-set-default-theme white-edition
  chroot "$TARGET_ROOT" update-initramfs -u
EOF
    log "target is a staged root, not the running system: the four commands above"
    log "must run inside it (chroot or the ISO build's target-exec step)."
  fi
}

# ---------------------------------------------------------------------------
main() {
  log "installing White Edition boot presentation into: $TARGET_ROOT"
  install_grub
  install_plymouth
  install_gdm
  activate_or_instruct
  echo
  log "done. installed paths recorded in: $MANIFEST"
  log "Ubuntu White Edition boot presentation staged. Verify with ../ASSUMPTION.md section 6."
}

main "$@"
