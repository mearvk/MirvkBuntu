#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0
#
# install-git-improved.sh — Improved Git experience as an install component.
#
# Installs a modern Git plus the companions most users expect, and applies a
# conservative, NON-DESTRUCTIVE set of system-wide defaults. Today git is only
# an optional entry in installer/software-relative-meaning.json with no
# dedicated install path; this script provides that path following the same
# conventions as install-os-security.sh (FEAT-002):
#   - Modern Git via the official PPA (ppa:git-core/ppa) when reachable, with a
#     graceful fall back to the archive git when the PPA cannot be added
#   - git-lfs (large file storage), git-doc, gitk and git-gui companions
#   - `git lfs install --system` so LFS is available system-wide
#   - sensible system-wide defaults written to /etc/gitconfig, but ONLY for
#     keys that are not already set — never overwriting existing configuration
#
# Design principles (see ubuntu-white/INSTALL.md):
#   - inspect -> plan -> authorize -> apply -> verify
#   - no silent privileged changes; every action is announced
#   - user choice is authoritative (per-component GIT_IMPROVED_* toggles)
#   - idempotent: safe to re-run
#   - user identity (user.name / user.email) is NEVER set system-wide; that is
#     left entirely to the end user
#
# Toggles (default 1 = enable, 0 = skip):
#   GIT_IMPROVED_PPA     add ppa:git-core/ppa for a newer Git (non-fatal on
#                        failure; falls back to the archive git)
#   GIT_IMPROVED_CONFIG  write system-wide /etc/gitconfig defaults for keys
#                        that are not already set
#
# Usage:
#   During OS install (chroot): /usr/sbin/install-git-improved.sh
#   Standalone:                 sudo bash install-git-improved.sh
#   Selective (CLI):            GIT_IMPROVED_PPA=0 sudo -E bash install-git-improved.sh
#
# Copyright (C) 2026 MEARVK LLC

set -euo pipefail

# ============================================================
# Configuration
# ============================================================

LOG="/var/log/git-improved-install.log"

# Per-component toggles. Default ON; set to 0 for headless/CLI opt-out.
GIT_IMPROVED_PPA="${GIT_IMPROVED_PPA:-1}"
GIT_IMPROVED_CONFIG="${GIT_IMPROVED_CONFIG:-1}"

echo "╔══════════════════════════════════════════════════════════════╗"
echo "║  Improved Git — System Installation                         ║"
echo "║  Galactic Cherry Marvell Edition 98                         ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo ""

exec > >(tee -a "$LOG") 2>&1

if [ "$(id -u)" -ne 0 ]; then
    echo "ERROR: Must run as root (or in chroot during OS install)" >&2
    exit 1
fi

command -v apt-get >/dev/null 2>&1 || { echo "ERROR: apt-get not found." >&2; exit 1; }

export DEBIAN_FRONTEND=noninteractive

echo "[plan] Improved Git components selected:"
echo "  Git PPA (git-core/ppa) ... $([ "$GIT_IMPROVED_PPA" = "1" ]    && echo enabled || echo skipped)"
echo "  System-wide defaults ..... $([ "$GIT_IMPROVED_CONFIG" = "1" ] && echo enabled || echo skipped)"
echo "  Packages ................. git git-lfs git-doc gitk git-gui (always)"
echo "  User identity ............ NEVER set system-wide (left to the user)"
echo ""

# Track outcomes for the trailing summary block.
PPA_USED="no"
LFS_STATUS="skipped"
CONFIG_APPLIED=()
CONFIG_PRESENT=()

# ============================================================
# 1. Optionally add the official Git PPA (newer Git)
# ============================================================

echo "=== [1/4] Selecting Git package source ==="

if [ "$GIT_IMPROVED_PPA" = "1" ]; then
    # add-apt-repository lives in software-properties-common; install it first
    # if it is missing so the PPA can be added. All of this is best-effort:
    # a restricted/offline environment must fall back to the archive git
    # without failing the whole script.
    if ! command -v add-apt-repository >/dev/null 2>&1; then
        echo "  add-apt-repository missing; installing software-properties-common ..."
        apt-get update -qq || true
        apt-get install -y --no-install-recommends software-properties-common >> "$LOG" 2>&1 || true
    fi

    if command -v add-apt-repository >/dev/null 2>&1 \
        && add-apt-repository -y ppa:git-core/ppa >> "$LOG" 2>&1; then
        PPA_USED="yes"
        echo "  ✓ Added ppa:git-core/ppa (newer Git than the archive default)"
    else
        echo "  ! Could not add ppa:git-core/ppa (offline/restricted); using archive git"
    fi
else
    echo "  (PPA skipped by GIT_IMPROVED_PPA=0; using archive git)"
fi

# ============================================================
# 2. Install Git + companions
# ============================================================

echo ""
echo "=== [2/4] Installing Git and companions ==="

apt-get update -qq

# git         — the version control system itself (newer if the PPA was added)
# git-lfs     — Git Large File Storage extension
# git-doc     — offline HTML/man documentation
# gitk        — Tk-based history browser
# git-gui     — Tk-based commit/staging GUI
PKGS=(git git-lfs git-doc gitk git-gui)

apt-get install -y --no-install-recommends "${PKGS[@]}" >> "$LOG" 2>&1
echo "  ✓ Git packages installed: ${PKGS[*]}"

# ============================================================
# 3. System-wide Git LFS
# ============================================================

echo ""
echo "=== [3/4] Enabling Git LFS system-wide ==="

# `git lfs install --system` wires the LFS smudge/clean filters into the
# system-wide gitconfig. Guarded so a missing/older git-lfs never aborts the
# run.
if git lfs install --system >> "$LOG" 2>&1; then
    LFS_STATUS="installed (system-wide)"
    echo "  ✓ git lfs install --system"
else
    LFS_STATUS="unavailable"
    echo "  ! git lfs install --system failed (LFS unavailable); continuing"
fi

# ============================================================
# 4. Non-destructive system-wide defaults (/etc/gitconfig)
# ============================================================
#
# We only write a key when `git config --system --get <key>` reports nothing,
# so any value already present (from the base image, an admin, or a prior run)
# is preserved. This keeps the change idempotent and honours the repo's
# 'no silent changes / user choice authoritative' philosophy.
#
# IMPORTANT: user identity is intentionally NOT configured here. user.name and
# user.email are personal to each user and must be set by the user themselves,
# never system-wide.

echo ""
echo "=== [4/4] Applying system-wide Git defaults (non-destructive) ==="

# set_default_if_unset <key> <value>
# Sets a system-wide git config key ONLY when it is not already present.
set_default_if_unset() {
    local key="$1" value="$2" existing
    if existing="$(git config --system --get "$key" 2>/dev/null)" && [ -n "$existing" ]; then
        echo "  = ${key} already set (${existing}); leaving unchanged"
        CONFIG_PRESENT+=("$key")
    else
        if git config --system "$key" "$value" >> "$LOG" 2>&1; then
            echo "  ✓ ${key} = ${value}"
            CONFIG_APPLIED+=("$key")
        else
            echo "  ! failed to set ${key} (continuing)"
        fi
    fi
}

if [ "$GIT_IMPROVED_CONFIG" = "1" ]; then
    # Prefer the libsecret credential helper when it is available on the host,
    # otherwise use the in-memory cache helper.
    CRED_HELPER="cache"
    if [ -x /usr/lib/git-core/git-credential-libsecret ] \
        || [ -x /usr/libexec/git-core/git-credential-libsecret ]; then
        CRED_HELPER="libsecret"
    fi

    set_default_if_unset init.defaultBranch main
    set_default_if_unset pull.rebase false
    set_default_if_unset fetch.prune true
    set_default_if_unset core.pager less
    set_default_if_unset color.ui auto
    set_default_if_unset credential.helper "$CRED_HELPER"
else
    echo "  (system-wide defaults skipped by GIT_IMPROVED_CONFIG=0)"
fi

# ============================================================
# Summary
# ============================================================

GIT_VERSION="$(git --version 2>/dev/null || echo 'git (version unknown)')"

echo ""
echo "=== Improved Git installed ==="
echo "  Installed packages:   git git-lfs git-doc gitk git-gui"
echo "  Git version:          ${GIT_VERSION}"
echo "  git-core PPA used:    ${PPA_USED}"
echo "  Git LFS:              ${LFS_STATUS}"
if [ "$GIT_IMPROVED_CONFIG" = "1" ]; then
    echo "  Config applied:       $([ "${#CONFIG_APPLIED[@]}" -gt 0 ] && echo "${CONFIG_APPLIED[*]}" || echo '(none — all already set)')"
    echo "  Config already set:   $([ "${#CONFIG_PRESENT[@]}" -gt 0 ] && echo "${CONFIG_PRESENT[*]}" || echo '(none)')"
else
    echo "  Config applied:       skipped (GIT_IMPROVED_CONFIG=0)"
fi
echo "  User identity:        NOT set system-wide (user.name/user.email left to the user)"
echo "  Log:                  $LOG"
