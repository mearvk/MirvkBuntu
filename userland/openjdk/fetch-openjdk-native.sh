#!/bin/bash
# SPDX-License-Identifier: GPL-2.0
#
# fetch-openjdk-native.sh - Fetch OpenJDK 28 native (C/C++) sources
#
# Pulls only .c, .cpp, .h, .hpp, and .S files from the OpenJDK mainline (JDK 28)
# to keep total size under ~50MB.
#
# Source: https://github.com/openjdk/jdk (master branch = JDK 28 development)
# License: GPL-2.0 with Classpath Exception
#
# Copyright (C) 2026 MEARVK LLC

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
TARGET_DIR="$SCRIPT_DIR/jdk-src"
REPO_URL="https://github.com/openjdk/jdk.git"
BRANCH="master"

echo "╔══════════════════════════════════════════════════════════════╗"
echo "║  OpenJDK 28 Native Source Fetch                              ║"
echo "║  Repository: openjdk/jdk (master)                            ║"
echo "║  Scope: .c .cpp .h .hpp .S files only                       ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo ""

# Method: sparse checkout with cone mode to get only native source paths
if [ -d "$TARGET_DIR" ]; then
    echo "Source directory already exists: $TARGET_DIR"
    echo "To re-fetch, remove it first: rm -rf $TARGET_DIR"
    exit 0
fi

echo "=== Initializing sparse clone ==="
mkdir -p "$TARGET_DIR"
cd "$TARGET_DIR"

git init
git remote add origin "$REPO_URL"

# Configure sparse checkout - only native source directories
git sparse-checkout init --cone
git sparse-checkout set \
    src/hotspot \
    src/java.base/share/native \
    src/java.base/unix/native \
    src/java.base/linux/native \
    src/java.desktop/share/native \
    src/java.desktop/unix/native \
    src/java.desktop/linux/native \
    src/java.prefs/unix/native \
    src/java.security.jgss/share/native \
    src/java.smartcardio/unix/native \
    src/jdk.crypto.ec/share/native \
    src/jdk.hotspot.agent/share/native \
    src/jdk.hotspot.agent/linux/native \
    src/jdk.jdwp.agent/share/native \
    src/jdk.jdwp.agent/unix/native \
    src/jdk.management/share/native \
    src/jdk.management/unix/native \
    src/jdk.net/share/native \
    src/jdk.net/linux/native \
    src/jdk.sctp/share/native \
    src/jdk.sctp/unix/native

echo ""
echo "=== Fetching (depth=1, sparse) ==="
git fetch --depth=1 origin "$BRANCH"
git checkout FETCH_HEAD

echo ""
echo "=== Removing non-native files ==="
# Remove everything except C/C++ source and headers
find . -type f \
    ! -name "*.c" \
    ! -name "*.cpp" \
    ! -name "*.h" \
    ! -name "*.hpp" \
    ! -name "*.S" \
    ! -name "*.s" \
    ! -name "LICENSE" \
    ! -name "ASSEMBLY_EXCEPTION" \
    ! -path "./.git/*" \
    -delete

# Remove non-x86 CPU architectures (keep only x86)
echo "  Trimming non-x86 architectures..."
cd src/hotspot/cpu && ls | grep -v x86 | xargs rm -rf && cd ../../..
# Remove non-Linux OS dirs
echo "  Trimming non-Linux OS targets..."
cd src/hotspot/os && ls | grep -v linux | xargs rm -rf && cd ../../..
cd src/hotspot/os_cpu && ls | grep -v linux_x86 | xargs rm -rf && cd ../../..
# Remove java.desktop (AWT/Swing native - 18MB, not needed for JVM mods)
echo "  Removing java.desktop (AWT/Swing)..."
rm -rf src/java.desktop
# Remove JFR and CDS (optional subsystems)
echo "  Removing JFR and CDS..."
rm -rf src/hotspot/share/jfr src/hotspot/share/cds

# Remove empty directories
find . -type d -empty -delete 2>/dev/null || true

# Remove .git to save space (we have what we need)
rm -rf .git

echo ""
echo "=== Writing provenance marker ==="
cat > OPENJDK_SOURCE_INFO << 'EOF'
OpenJDK 28 Native Source (C/C++)
================================
Repository: https://github.com/openjdk/jdk
Branch: master (JDK 28 development trunk)
License: GPL-2.0-only WITH Classpath-exception-2.0
Fetched: $(date -u '+%Y-%m-%d %H:%M:%S UTC')
Scope: Native source only (.c, .cpp, .h, .hpp, .S)

This is a partial checkout containing only the native (C/C++) source files
from the OpenJDK project. Java source files are excluded.

Key directories:
  src/hotspot/          - JVM implementation (HotSpot)
  src/java.base/       - Core native libraries (libjava, libnet, libnio, etc.)
  src/java.desktop/    - AWT/Swing native code
  src/jdk.*/           - Module-specific native code
EOF

# Replace the date placeholder
sed -i "s/\$(date -u '+%Y-%m-%d %H:%M:%S UTC')/$(date -u '+%Y-%m-%d %H:%M:%S UTC')/" OPENJDK_SOURCE_INFO

echo ""
echo "=== Summary ==="
FILE_COUNT=$(find . -type f \( -name "*.c" -o -name "*.cpp" -o -name "*.h" -o -name "*.hpp" -o -name "*.S" \) | wc -l)
TOTAL_SIZE=$(du -sh . | cut -f1)
echo "  Files: $FILE_COUNT"
echo "  Size:  $TOTAL_SIZE"
echo "  Location: $TARGET_DIR"
echo ""
echo "Done."
