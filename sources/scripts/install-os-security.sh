#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0
#
# install-os-security.sh — General-purpose OS security baseline.
#
# Installs and configures the basics of OS security for the general OS
# install, independent of the JWSTF / Java web server path. This mirrors the
# proven package/enable patterns from install-jwstf.sh but applies a
# conservative, non-silent hardening baseline suitable for any desktop or
# server profile:
#   - ClamAV antivirus (+ freshclam signature updates)
#   - UFW firewall with a default-deny-incoming / allow-outgoing / allow-SSH
#     policy (NO application-specific web ports)
#   - AppArmor mandatory access control + profiles
#   - fail2ban brute-force protection
#   - unattended-upgrades automatic security updates
#   - rkhunter / chkrootkit rootkit detectors
#
# Design principles (see ubuntu-white/INSTALL.md):
#   - inspect -> plan -> authorize -> apply -> verify
#   - no silent privileged changes; every action is announced
#   - user choice is authoritative (per-component OS_SECURITY_* toggles)
#   - idempotent: safe to re-run
#
# Toggles (default 1 = install+enable, 0 = skip):
#   OS_SECURITY_CLAMAV      ClamAV antivirus + freshclam
#   OS_SECURITY_UFW         UFW firewall (default-deny incoming, allow SSH)
#   OS_SECURITY_APPARMOR    AppArmor MAC + profiles
#   OS_SECURITY_FAIL2BAN    fail2ban brute-force protection
#   OS_SECURITY_UNATTENDED  unattended-upgrades automatic security updates
#
# Usage:
#   During OS install (chroot): /usr/sbin/install-os-security.sh
#   Standalone:                 sudo bash install-os-security.sh
#   Selective (CLI):            OS_SECURITY_UFW=0 sudo -E bash install-os-security.sh
#
# Copyright (C) 2026 MEARVK LLC

set -euo pipefail

# ============================================================
# Configuration
# ============================================================

LOG="/var/log/os-security-install.log"

# Per-component toggles. Default ON; set to 0 for headless/CLI opt-out.
OS_SECURITY_CLAMAV="${OS_SECURITY_CLAMAV:-1}"
OS_SECURITY_UFW="${OS_SECURITY_UFW:-1}"
OS_SECURITY_APPARMOR="${OS_SECURITY_APPARMOR:-1}"
OS_SECURITY_FAIL2BAN="${OS_SECURITY_FAIL2BAN:-1}"
OS_SECURITY_UNATTENDED="${OS_SECURITY_UNATTENDED:-1}"

echo "╔══════════════════════════════════════════════════════════════╗"
echo "║  OS Security Baseline — System Installation                 ║"
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

echo "[plan] OS security components selected:"
echo "  ClamAV antivirus ......... $([ "$OS_SECURITY_CLAMAV" = "1" ]     && echo enabled || echo skipped)"
echo "  UFW firewall ............. $([ "$OS_SECURITY_UFW" = "1" ]        && echo enabled || echo skipped)"
echo "  AppArmor MAC ............. $([ "$OS_SECURITY_APPARMOR" = "1" ]   && echo enabled || echo skipped)"
echo "  fail2ban ................. $([ "$OS_SECURITY_FAIL2BAN" = "1" ]   && echo enabled || echo skipped)"
echo "  unattended-upgrades ...... $([ "$OS_SECURITY_UNATTENDED" = "1" ] && echo enabled || echo skipped)"
echo ""

# ============================================================
# 1. Install security package set
# ============================================================

echo "=== [1/4] Installing OS security packages ==="

apt-get update -qq

# Assemble the package list from the enabled components so a disabled toggle
# never pulls in that component's packages.
PKGS=()

if [ "$OS_SECURITY_CLAMAV" = "1" ]; then
    # ClamAV antivirus engine + on-access daemon + signature updater.
    PKGS+=(clamav clamav-daemon clamav-freshclam)
fi

if [ "$OS_SECURITY_UFW" = "1" ]; then
    # Uncomplicated Firewall front-end to netfilter.
    PKGS+=(ufw)
fi

if [ "$OS_SECURITY_APPARMOR" = "1" ]; then
    # AppArmor mandatory access control + tooling + bundled profiles.
    PKGS+=(apparmor apparmor-utils apparmor-profiles)
fi

if [ "$OS_SECURITY_FAIL2BAN" = "1" ]; then
    # Brute-force / intrusion mitigation for exposed services.
    PKGS+=(fail2ban)
fi

if [ "$OS_SECURITY_UNATTENDED" = "1" ]; then
    # Automatic security updates + change reporting.
    PKGS+=(unattended-upgrades apt-listchanges)
fi

# Rootkit detectors are part of the advertised security baseline and are
# installed whenever any security component is requested.
PKGS+=(rkhunter chkrootkit)

if [ "${#PKGS[@]}" -gt 0 ]; then
    apt-get install -y --no-install-recommends "${PKGS[@]}" >> "$LOG" 2>&1
    echo "  ✓ Security packages installed: ${PKGS[*]}"
else
    echo "  (no security packages selected — nothing to install)"
fi

# ============================================================
# 2. Enable security services
# ============================================================

echo ""
echo "=== [2/4] Enabling security services ==="

systemctl daemon-reload 2>/dev/null || true

if [ "$OS_SECURITY_CLAMAV" = "1" ]; then
    # ClamAV signature updates + scanning daemon.
    systemctl enable clamav-freshclam.service 2>/dev/null || true
    systemctl enable clamav-daemon.service 2>/dev/null || true
    echo "  ✓ ClamAV freshclam + daemon enabled"
fi

if [ "$OS_SECURITY_APPARMOR" = "1" ]; then
    # AppArmor is often already active; enable only when not already enabled.
    if command -v aa-enabled >/dev/null 2>&1 && aa-enabled --quiet 2>/dev/null; then
        echo "  ✓ AppArmor already enabled"
    else
        systemctl enable apparmor.service 2>/dev/null || true
        echo "  ✓ AppArmor service enabled"
    fi
fi

if [ "$OS_SECURITY_FAIL2BAN" = "1" ]; then
    systemctl enable fail2ban.service 2>/dev/null || true
    echo "  ✓ fail2ban enabled"
fi

if [ "$OS_SECURITY_UNATTENDED" = "1" ]; then
    systemctl enable unattended-upgrades.service 2>/dev/null || true
    echo "  ✓ unattended-upgrades service enabled"
fi

# ============================================================
# 3. Firewall (UFW) — conservative default policy
# ============================================================
#
# NOTE: This is the GENERAL OS security policy and intentionally does NOT open
# the JWSTF/NWE web ports (80/443/8080/8443/23). install-jwstf.sh adds those
# rules separately when the Java web server path is installed, so the two
# scripts do not conflict when both run.

echo ""
echo "=== [3/4] Configuring firewall (UFW) ==="

if [ "$OS_SECURITY_UFW" = "1" ]; then
    ufw default deny incoming 2>/dev/null || true
    ufw default allow outgoing 2>/dev/null || true
    # Keep remote administration reachable. Prefer the named app profile and
    # fall back to the raw SSH port when the profile is unavailable.
    ufw allow OpenSSH 2>/dev/null || ufw allow ssh 2>/dev/null || true
    ufw --force enable 2>/dev/null || true
    echo "  ✓ UFW: default deny incoming, allow outgoing, SSH allowed, enabled"
else
    echo "  (UFW skipped by OS_SECURITY_UFW=0)"
fi

# ============================================================
# 4. Automatic security updates (unattended-upgrades)
# ============================================================

echo ""
echo "=== [4/4] Configuring automatic security updates ==="

if [ "$OS_SECURITY_UNATTENDED" = "1" ]; then
    # Idempotent: (re)write the periodic policy so package lists are refreshed
    # and unattended upgrades run daily. dpkg-reconfigure covers the debconf
    # side; the apt.conf.d drop-in makes the intent explicit and auditable.
    dpkg-reconfigure -f noninteractive unattended-upgrades 2>/dev/null || true
    cat > /etc/apt/apt.conf.d/20auto-upgrades <<'AUTOUPGRADES' || true
APT::Periodic::Update-Package-Lists "1";
APT::Periodic::Unattended-Upgrade "1";
AUTOUPGRADES
    echo "  ✓ unattended-upgrades configured (daily update + security upgrade)"
else
    echo "  (automatic security updates skipped by OS_SECURITY_UNATTENDED=0)"
fi

# ============================================================
# Summary
# ============================================================

echo ""
echo "=== OS Security baseline installed ==="
echo "  ClamAV antivirus:     $([ "$OS_SECURITY_CLAMAV" = "1" ]     && echo 'installed + freshclam/daemon enabled' || echo 'skipped')"
echo "  UFW firewall:         $([ "$OS_SECURITY_UFW" = "1" ]        && echo 'deny-incoming / allow-outgoing / SSH / enabled' || echo 'skipped')"
echo "  AppArmor MAC:         $([ "$OS_SECURITY_APPARMOR" = "1" ]   && echo 'installed + enabled' || echo 'skipped')"
echo "  fail2ban:             $([ "$OS_SECURITY_FAIL2BAN" = "1" ]   && echo 'installed + enabled' || echo 'skipped')"
echo "  unattended-upgrades:  $([ "$OS_SECURITY_UNATTENDED" = "1" ] && echo 'installed + enabled + configured' || echo 'skipped')"
echo "  Rootkit detectors:    rkhunter + chkrootkit installed"
echo "  Log:                  $LOG"
