#!/bin/bash
# SPDX-License-Identifier: GPL-2.0
#
# install-ubuntu-installer.sh — Install Ubuntu Desktop Provision (graphical installer)
#
# Installs the full Ubuntu graphical installer (Flutter-based, Subiquity backend)
# into the live session, branded for Galactic Cherry Marvell Edition 98.
#
# This is the same installer used by Ubuntu 24.04+ desktop ISOs — full GUI with:
#   - Language selection
#   - Keyboard layout
#   - Network connection
#   - Installation type (normal/minimal/alongside/erase)
#   - Disk partitioning (graphical)
#   - User account creation
#   - Timezone
#   - Progress with slideshow
#
# Branding is applied via whitelabel.yaml (accent colors, flavor, images).
#
# Usage:
#   In chroot: ./install-ubuntu-installer.sh
#   During build: chroot build/rootfs /usr/sbin/install-ubuntu-installer.sh
#
# Copyright (C) 2026 MEARVK LLC
# Author: Maximilian Eric Alexander Rupplin von Keffikon

set -e

echo "╔══════════════════════════════════════════════════╗"
echo "║  Ubuntu Desktop Installer — Galactic Cherry     ║"
echo "║  Full Graphical Installer (Flutter/Subiquity)   ║"
echo "╚══════════════════════════════════════════════════╝"
echo ""

if [ "$(id -u)" -ne 0 ]; then
    echo "ERROR: Must run as root"
    exit 1
fi

# ============================================================
# Install snapd and the installer snaps
# ============================================================

export DEBIAN_FRONTEND=noninteractive

echo "=== Installing snapd and subiquity ==="
apt-get update -qq
apt-get install -y --no-install-recommends \
    snapd \
    squashfuse \
    fuse3

# Start snapd (in chroot we simulate — on live system it runs)
if command -v systemctl >/dev/null 2>&1 && systemctl is-system-running >/dev/null 2>&1; then
    systemctl enable snapd.socket snapd.service 2>/dev/null || true
    systemctl start snapd.socket snapd.service 2>/dev/null || true
    sleep 2

    echo "=== Installing ubuntu-desktop-bootstrap snap ==="
    snap install ubuntu-desktop-bootstrap --classic 2>/dev/null || true
else
    echo "  NOTE: snapd not running (chroot environment)."
    echo "  The installer snap will be seeded for first boot."
    # Seed the snap for installation at first boot
    mkdir -p /var/lib/snapd/seed/snaps
    mkdir -p /var/lib/snapd/seed/assertions

    # Create seed.yaml so snapd installs it on first boot
    mkdir -p /var/lib/snapd/seed
    cat > /var/lib/snapd/seed/seed.yaml << 'SEED'
snaps:
  - name: snapd
    channel: stable
    file: snapd.snap
  - name: ubuntu-desktop-bootstrap
    channel: stable/ubuntu-24.04
    classic: true
    file: ubuntu-desktop-bootstrap.snap
SEED
    echo "  Snap seeding configured for first boot."
fi

# Also install subiquity as the backend (in case snap isn't ready)
echo "=== Installing subiquity backend ==="
apt-get install -y --no-install-recommends \
    subiquity \
    curtin \
    probert-common \
    probert-network \
    probert-storage 2>/dev/null || echo "  (subiquity from snap, deb fallback skipped)"

# ============================================================
# Branding: whitelabel.yaml
# ============================================================

echo ""
echo "=== Applying Galactic Cherry branding ==="

PROVISION_DIR="/usr/share/desktop-provision"
mkdir -p "$PROVISION_DIR/images"

cat > "$PROVISION_DIR/whitelabel.yaml" << 'EOF'
# Ubuntu Desktop Provision — Galactic Cherry Marvell Edition 98
# Branding configuration for the graphical installer

mode: standard

flavor: mate

app-name: "Install Galactic Cherry"

theme:
  light:
    accent-color: "#C0392B"
    elevated-button-color: "#C0392B"
    elevated-button-text-color: "#FFFFFF"
  dark:
    accent-color: "#E74C3C"
    elevated-button-color: "#C0392B"
    elevated-button-text-color: "#FFFFFF"

pages:
  locale:
    visible: true
  keyboard:
    visible: true
  network:
    visible: true
  source-selection:
    visible: true
  codecs-and-drivers:
    visible: true
  storage:
    visible: true
  identity:
    visible: true
  confirm:
    visible: true
  done:
    visible: true
EOF

# ============================================================
# Installer slides (shown during installation progress)
# ============================================================

SLIDES_DIR="$PROVISION_DIR/slides"
mkdir -p "$SLIDES_DIR"

cat > "$SLIDES_DIR/01_welcome.html" << 'HTML'
<!DOCTYPE html>
<html>
<head><style>
body { font-family: 'Ubuntu', sans-serif; background: #1A0A0A; color: #F5E6E6; padding: 40px; }
h1 { color: #E74C3C; font-size: 2em; }
p { font-size: 1.2em; line-height: 1.6; }
.accent { color: #C0392B; font-weight: bold; }
</style></head>
<body>
<h1>Welcome to Galactic Cherry</h1>
<p>Ubuntu Determinant Alpha RS — Marvell Edition 98</p>
<p>A custom Linux system with <span class="accent">extended port addressing</span>,
heuristic security monitoring, graded privilege systems, and Dave —
the system's kernel-adjacent AI intelligence.</p>
</body>
</html>
HTML

cat > "$SLIDES_DIR/02_security.html" << 'HTML'
<!DOCTYPE html>
<html>
<head><style>
body { font-family: 'Ubuntu', sans-serif; background: #1A0A0A; color: #F5E6E6; padding: 40px; }
h1 { color: #E74C3C; font-size: 2em; }
p { font-size: 1.2em; line-height: 1.6; }
ul { font-size: 1.1em; }
li { margin: 8px 0; }
.accent { color: #C0392B; }
</style></head>
<body>
<h1>Security Built In</h1>
<p>Your system is protected by multiple layers:</p>
<ul>
<li><span class="accent">chkrootkit 2.51</span> — Rootkit detection (100+ signatures)</li>
<li><span class="accent">rkhunter 8.46.9</span> — System integrity verification</li>
<li><span class="accent">ClamAV</span> — Protected antivirus (Memory Grain 3)</li>
<li><span class="accent">UFW + AppArmor</span> — Firewall + mandatory access control</li>
<li><span class="accent">fail2ban + unattended-upgrades</span> — Brute-force defense + auto security updates</li>
<li><span class="accent">File Integrity DB</span> — MySQL-backed SHA-256 baselines</li>
<li><span class="accent">Heuristic Port Monitor</span> — Three-stage packet analysis</li>
<li><span class="accent">sudo_gate</span> — 8-level graded privilege system</li>
</ul>
</body>
</html>
HTML

cat > "$SLIDES_DIR/03_desktop.html" << 'HTML'
<!DOCTYPE html>
<html>
<head><style>
body { font-family: 'Ubuntu', sans-serif; background: #1A0A0A; color: #F5E6E6; padding: 40px; }
h1 { color: #E74C3C; font-size: 2em; }
p { font-size: 1.2em; line-height: 1.6; }
.accent { color: #C0392B; }
</style></head>
<body>
<h1>Your Desktop</h1>
<p>Galactic Cherry comes with the <span class="accent">MATE Desktop</span>
in a distinctive dark theme with cherry red accents.</p>
<p>19 original wallpapers — 9 SVG (resolution-independent) and 10 Marvell JPEG (4K).</p>
<p>Humanity icons give you the familiar Ubuntu look with warm orange and dark icon sets.</p>
<p>OpenJDK 28 is pre-installed for Java development.</p>
</body>
</html>
HTML

cat > "$SLIDES_DIR/04_kernel.html" << 'HTML'
<!DOCTYPE html>
<html>
<head><style>
body { font-family: 'Ubuntu', sans-serif; background: #1A0A0A; color: #F5E6E6; padding: 40px; }
h1 { color: #E74C3C; font-size: 2em; }
p { font-size: 1.2em; line-height: 1.6; }
ul { font-size: 1.1em; columns: 2; }
li { margin: 6px 0; }
.accent { color: #C0392B; }
</style></head>
<body>
<h1>Custom Kernel Extensions</h1>
<p>Linux 5.15.204 with 9 purpose-built extensions:</p>
<ul>
<li>Extended Port Range (30 quintillion)</li>
<li>EPMP Port Multiplexer</li>
<li>Heuristic Port Monitor</li>
<li>Extended Permission Classes</li>
<li>USB Dynamic RAM Expansion</li>
<li>USB Hardware-Direct DMA</li>
<li>NEGAMANE Immutable Filesystem</li>
<li>Per-User Kernel Objects</li>
<li>CPU Boost Designation</li>
</ul>
</body>
</html>
HTML

cat > "$SLIDES_DIR/05_dave.html" << 'HTML'
<!DOCTYPE html>
<html>
<head><style>
body { font-family: 'Ubuntu', sans-serif; background: #1A0A0A; color: #F5E6E6; padding: 40px; }
h1 { color: #E74C3C; font-size: 2em; }
p { font-size: 1.2em; line-height: 1.6; }
.accent { color: #C0392B; }
</style></head>
<body>
<h1>Meet Dave</h1>
<p><span class="accent">Dave</span> is the system's kernel-adjacent AI intelligence.
He loads at boot, reasons at 200+ IQ, and casts approximately 150 million votes per year
across all system decisions.</p>
<p>Five internal voters — safety, correctness, ethics, performance, elegance —
with veto power ensure every decision is careful and sound.</p>
<p>A 75-book library informs his reasoning: philosophy, science, ethics, mathematics.</p>
</body>
</html>
HTML

# ============================================================
# Desktop file for installer launch icon
# ============================================================

mkdir -p /usr/share/applications
cat > /usr/share/applications/galactic-cherry-installer.desktop << 'EOF'
[Desktop Entry]
Type=Application
Name=Install Galactic Cherry
Comment=Install Ubuntu Determinant Alpha RS to your computer
Exec=sh -c 'pkexec ubuntu-desktop-bootstrap 2>/dev/null || pkexec /usr/sbin/galactic-cherry-installer'
Icon=system-installer
Terminal=false
Categories=System;
Keywords=install;installer;system;
NoDisplay=false
EOF

# Also put it on the desktop for live session
for user_home in /home/* /root; do
    if [ -d "$user_home" ]; then
        mkdir -p "$user_home/Desktop"
        cp /usr/share/applications/galactic-cherry-installer.desktop "$user_home/Desktop/" 2>/dev/null || true
        chmod 755 "$user_home/Desktop/galactic-cherry-installer.desktop" 2>/dev/null || true
    fi
done

# ============================================================
# Casper/live-boot configuration for installer mode
# ============================================================

# If casper is available, configure it
if [ -d /etc/casper.conf ] || command -v casper >/dev/null 2>&1; then
    cat >> /etc/casper.conf << 'EOF'
export FLAVOUR="Galactic Cherry"
export HOST="galactic-cherry"
export BUILD_SYSTEM="Ubuntu"
EOF
fi

# ============================================================
# Cleanup
# ============================================================

apt-get clean
rm -rf /var/lib/apt/lists/*

echo ""
echo "╔══════════════════════════════════════════════════╗"
echo "║  GRAPHICAL INSTALLER CONFIGURED                 ║"
echo "╚══════════════════════════════════════════════════╝"
echo ""
echo "  Installer:    Ubuntu Desktop Provision (Flutter)"
echo "  Backend:      Subiquity"
echo "  Branding:     Galactic Cherry (dark + #C0392B red)"
echo "  Slides:       5 custom installation slides"
echo "  Flavor:       MATE"
echo ""
echo "  Live session: Double-click 'Install Galactic Cherry' on desktop"
echo "  Or from GRUB: Select 'Install to Disk'"
echo ""
echo "  Fallback:     /usr/sbin/galactic-cherry-installer (TUI)"
echo ""
