#!/bin/bash
# SPDX-License-Identifier: BSD-3-Clause
#
# fetch-chromium.sh — Shallow clone Chromium open-source browser
#
# Chromium source is too large for Git hosting (30+ GB full).
# This script performs a shallow clone (10 commits) of the official
# GitHub mirror at build time.
#
# The source can then be built with GN/Ninja on the client machine.
#
# Source: https://github.com/chromium/chromium (BSD-3-Clause)
#
# Usage:
#   ./fetch-chromium.sh                    Clone to current directory
#   ./fetch-chromium.sh /path/to/dest      Clone to specific directory
#   ./fetch-chromium.sh --tag 127.0.6533.0 Clone a specific version tag
#
# Copyright (C) 2026 MEARVK LLC

set -e

REPO_URL="https://github.com/chromium/chromium.git"
DEPTH=10
DEST_DIR="${1:-$(pwd)/chromium-src}"
TAG=""
BRANCH="main"

# Parse arguments
while [ $# -gt 0 ]; do
    case "$1" in
        --tag)    shift; TAG="$1" ;;
        --branch) shift; BRANCH="$1" ;;
        --depth)  shift; DEPTH="$1" ;;
        --help|-h)
            echo "fetch-chromium.sh — Shallow clone Chromium source"
            echo ""
            echo "Usage:"
            echo "  ./fetch-chromium.sh [dest_dir]"
            echo "  ./fetch-chromium.sh --tag 127.0.6533.0"
            echo "  ./fetch-chromium.sh --depth 10"
            echo ""
            echo "Options:"
            echo "  --tag TAG       Clone a specific release tag"
            echo "  --branch BRANCH Clone a specific branch (default: main)"
            echo "  --depth N       Shallow clone depth (default: 10)"
            echo ""
            echo "Source: github.com/chromium/chromium (BSD-3-Clause)"
            echo "NOTE: Even shallow, this is ~5-8 GB. Ensure sufficient disk space."
            exit 0
            ;;
        *)
            if [ -z "$TAG" ] && [ "$1" != "" ] && [[ ! "$1" == --* ]]; then
                DEST_DIR="$1"
            fi
            ;;
    esac
    shift
done

echo "╔══════════════════════════════════════════════════╗"
echo "║  Chromium Browser — Open Source (BSD-3-Clause)  ║"
echo "╚══════════════════════════════════════════════════╝"
echo ""
echo "  Repository:  ${REPO_URL}"
echo "  Destination: ${DEST_DIR}"
echo "  Depth:       ${DEPTH} commits"
[ -n "$TAG" ] && echo "  Tag:         ${TAG}"
[ -z "$TAG" ] && echo "  Branch:      ${BRANCH}"
echo ""

# Check prerequisites
if ! command -v git >/dev/null 2>&1; then
    echo "ERROR: git not found. Install with: apt install git"
    exit 1
fi

# Check disk space (need ~8 GB free minimum)
AVAIL_GB=$(df -BG "${DEST_DIR%/*}" 2>/dev/null | awk 'NR==2 {print $4}' | tr -d 'G')
if [ -n "$AVAIL_GB" ] && [ "$AVAIL_GB" -lt 8 ] 2>/dev/null; then
    echo "WARNING: Only ${AVAIL_GB}GB free. Chromium shallow clone needs ~5-8 GB."
    read -p "Continue anyway? [y/N] " answer
    [ "$answer" != "y" ] && [ "$answer" != "Y" ] && exit 1
fi

# Check if already cloned
if [ -d "${DEST_DIR}/.git" ]; then
    echo "Already cloned at ${DEST_DIR}"
    echo "Updating (fetch latest ${DEPTH} commits)..."
    cd "${DEST_DIR}"
    git fetch --depth="${DEPTH}" origin "${BRANCH}" 2>&1 | tail -5
    git reset --hard "origin/${BRANCH}" 2>/dev/null || true
    echo ""
    echo "Updated. HEAD: $(git log --oneline -1)"
    exit 0
fi

# Perform shallow clone
echo "Cloning Chromium (shallow, ${DEPTH} commits)..."
echo "This will download ~5-8 GB. Please be patient."
echo ""

mkdir -p "$(dirname "${DEST_DIR}")"

if [ -n "$TAG" ]; then
    git clone --depth="${DEPTH}" --branch "${TAG}" --single-branch \
        "${REPO_URL}" "${DEST_DIR}"
else
    git clone --depth="${DEPTH}" --branch "${BRANCH}" --single-branch \
        "${REPO_URL}" "${DEST_DIR}"
fi

echo ""
echo "╔══════════════════════════════════════════════════╗"
echo "║  CHROMIUM SOURCE CLONED                         ║"
echo "╚══════════════════════════════════════════════════╝"
echo ""
echo "  Location:  ${DEST_DIR}"
echo "  Size:      $(du -sh "${DEST_DIR}" 2>/dev/null | awk '{print $1}')"
echo "  HEAD:      $(cd "${DEST_DIR}" && git log --oneline -1)"
echo "  Commits:   ${DEPTH}"
echo ""
echo "  To build Chromium:"
echo "    1. Install depot_tools:"
echo "       git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git"
echo "       export PATH=\$PATH:/path/to/depot_tools"
echo ""
echo "    2. Generate build files:"
echo "       cd ${DEST_DIR}"
echo "       gn gen out/Release --args='is_debug=false is_component_build=false'"
echo ""
echo "    3. Build:"
echo "       autoninja -C out/Release chrome"
echo ""
echo "    4. Run:"
echo "       ./out/Release/chrome"
echo ""
echo "  Build requirements: ~16 GB RAM, ~100 GB disk, ~2-4 hours"
echo "  See: https://chromium.googlesource.com/chromium/src/+/main/docs/linux/build_instructions.md"
echo ""
