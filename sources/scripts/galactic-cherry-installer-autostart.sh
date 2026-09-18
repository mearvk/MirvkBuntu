#!/bin/bash
# SPDX-License-Identifier: GPL-2.0
#
# galactic-cherry-installer-autostart.sh
# Placed in /etc/profile.d/ — launches installer when 'installer' is on kernel cmdline
#
# Copyright (C) 2026 MEARVK LLC

if grep -q "installer" /proc/cmdline 2>/dev/null; then
    if [ "$(id -u)" -eq 0 ] && [ -x /usr/sbin/galactic-cherry-installer ]; then
        /usr/sbin/galactic-cherry-installer
    elif [ "$(id -u)" -ne 0 ] && [ -x /usr/sbin/galactic-cherry-installer ]; then
        echo ""
        echo "  ╔══════════════════════════════════════════════════╗"
        echo "  ║  Galactic Cherry Marvell Edition 98 — Installer ║"
        echo "  ╚══════════════════════════════════════════════════╝"
        echo ""
        echo "  To start the installer, run:"
        echo "    sudo galactic-cherry-installer"
        echo ""
    fi
fi
