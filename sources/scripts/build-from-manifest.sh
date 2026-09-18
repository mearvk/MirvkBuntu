#!/bin/bash
# =============================================================================
# build-from-manifest.sh — Read build-manifest.xml and compile selected components
#
# Ubuntu Determinant Alpha RS — Galactic Cherry Marvell Edition 98
# Copyright (C) 2026 MEARVK LLC
# Author: Maximilian Eric Alexander Rupplin von Keffikon
#
# Usage:
#   ./scripts/build-from-manifest.sh                         # Use default manifest
#   ./scripts/build-from-manifest.sh build-manifest.xml      # Explicit manifest
#   ./scripts/build-from-manifest.sh --profile server        # Override profile
#   ./scripts/build-from-manifest.sh --list                  # List components
#   ./scripts/build-from-manifest.sh --dry-run               # Show what would build
# =============================================================================

set -euo pipefail

MANIFEST="${1:-build-manifest.xml}"
PROFILE=""
DRY_RUN=0
LIST_ONLY=0

# Parse arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        --profile)
            PROFILE="$2"
            shift 2
            ;;
        --dry-run)
            DRY_RUN=1
            shift
            ;;
        --list)
            LIST_ONLY=1
            shift
            ;;
        --help|-h)
            echo "Usage: $0 [manifest.xml] [--profile NAME] [--dry-run] [--list]"
            echo ""
            echo "Profiles: full, server, minimal, desktop"
            exit 0
            ;;
        *)
            if [[ -f "$1" ]]; then
                MANIFEST="$1"
            fi
            shift
            ;;
    esac
done

# Verify manifest exists
if [[ ! -f "$MANIFEST" ]]; then
    echo "ERROR: Manifest not found: $MANIFEST"
    exit 1
fi

# Check for xmllint (libxml2-utils)
if ! command -v xmllint &>/dev/null; then
    echo "ERROR: xmllint not found. Install: sudo apt install libxml2-utils"
    exit 1
fi

# =============================================================================
# Parse manifest
# =============================================================================

echo "╔══════════════════════════════════════════════════════════════╗"
echo "║  Build from Manifest                                         ║"
echo "║  Galactic Cherry Marvell Edition 98                          ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo ""
echo "  Manifest: $MANIFEST"

# Get edition info
EDITION=$(xmllint --xpath 'string(/manifest/@edition)' "$MANIFEST" 2>/dev/null || echo "unknown")
VERSION=$(xmllint --xpath 'string(/manifest/@version)' "$MANIFEST" 2>/dev/null || echo "?")
KERNEL_VER=$(xmllint --xpath 'string(/manifest/@kernel)' "$MANIFEST" 2>/dev/null || echo "?")

echo "  Edition:  $EDITION v$VERSION (kernel $KERNEL_VER)"

# Determine active profile
if [[ -z "$PROFILE" ]]; then
    PROFILE=$(xmllint --xpath 'string(/manifest/profile/@name)' "$MANIFEST" 2>/dev/null || echo "full")
fi
echo "  Profile:  $PROFILE"
echo ""

# =============================================================================
# Collect enabled components
# =============================================================================

# Get all component names and their enabled status
declare -a TARGETS=()
declare -a NAMES=()
declare -a DESCRIPTIONS=()

# Function to check if component is enabled (considering profile overrides)
is_enabled() {
    local comp_name="$1"

    # Check for profile override
    local override
    override=$(xmllint --xpath "string(/manifest/profiles/profile[@name='$PROFILE']/override[@component='$comp_name']/@enabled)" "$MANIFEST" 2>/dev/null || echo "")

    if [[ -n "$override" ]]; then
        [[ "$override" == "true" ]] && return 0 || return 1
    fi

    # Check if the component's group is in the profile's include-groups
    local include_groups
    include_groups=$(xmllint --xpath "string(/manifest/profiles/profile[@name='$PROFILE']/include-groups)" "$MANIFEST" 2>/dev/null || echo "")

    if [[ -n "$include_groups" ]]; then
        # Find which group this component belongs to
        local comp_group
        comp_group=$(xmllint --xpath "name(/manifest/group[component[@name='$comp_name']]/..)" "$MANIFEST" 2>/dev/null || echo "")
        # Simpler: check the group name attribute
        comp_group=$(xmllint --xpath "string(/manifest/group[component[@name='$comp_name']]/@name)" "$MANIFEST" 2>/dev/null || echo "")

        if [[ -n "$comp_group" ]]; then
            if echo "$include_groups" | grep -q "$comp_group"; then
                # Group is included, check component's own enabled attribute
                local self_enabled
                self_enabled=$(xmllint --xpath "string(/manifest/group/component[@name='$comp_name']/@enabled)" "$MANIFEST" 2>/dev/null || echo "true")
                [[ "$self_enabled" == "true" ]] && return 0 || return 1
            else
                return 1
            fi
        fi
    fi

    # Default: use the component's enabled attribute
    local enabled
    enabled=$(xmllint --xpath "string(/manifest/group/component[@name='$comp_name']/@enabled)" "$MANIFEST" 2>/dev/null || echo "true")
    [[ "$enabled" == "true" ]] && return 0 || return 1
}

# Extract all components
COMPONENT_COUNT=$(xmllint --xpath 'count(/manifest/group/component)' "$MANIFEST" 2>/dev/null || echo "0")

echo "  Components ($COMPONENT_COUNT total):"
echo "  ─────────────────────────────────────────────"

for i in $(seq 1 "$COMPONENT_COUNT"); do
    name=$(xmllint --xpath "string(/manifest/group/component[$i]/@name)" "$MANIFEST" 2>/dev/null)
    target=$(xmllint --xpath "string(/manifest/group/component[$i]/@target)" "$MANIFEST" 2>/dev/null)
    desc=$(xmllint --xpath "string(/manifest/group/component[$i]/description)" "$MANIFEST" 2>/dev/null)

    if is_enabled "$name"; then
        status="✓"
        TARGETS+=("$target")
        NAMES+=("$name")
    else
        status="✗"
    fi

    printf "  %s %-16s %-12s %s\n" "$status" "$name" "($target)" "$desc"
done

echo ""
echo "  Enabled: ${#TARGETS[@]} components"
echo ""

# List mode — just show and exit
if [[ $LIST_ONLY -eq 1 ]]; then
    exit 0
fi

# =============================================================================
# Build
# =============================================================================

# Deduplicate targets (some components share a target like "tools")
declare -A UNIQUE_TARGETS
for t in "${TARGETS[@]}"; do
    UNIQUE_TARGETS["$t"]=1
done

# Define build order
declare -a BUILD_ORDER=(
    "asm"
    "kernel"
    "rootfs"
    "x11"
    "wallpapers"
    "tools"
    "tools-muntutils"
    "tools-cronie"
    "tools-clamav"
    "tools-chkrootkit"
    "tools-rkhunter"
    "tools-mysql"
    "tools-ai"
    "java"
    "chromium"
    "desktop"
    "jwstf-install"
    "initramfs"
    "grub"
)

echo "  Build order:"

# Dry-run or actual build
for target in "${BUILD_ORDER[@]}"; do
    if [[ -n "${UNIQUE_TARGETS[$target]:-}" ]]; then
        if [[ $DRY_RUN -eq 1 ]]; then
            echo "    [dry-run] make $target"
        else
            echo "    → make $target"
            make "$target" || {
                echo "  ERROR: 'make $target' failed"
                exit 1
            }
        fi
    fi
done

echo ""
echo "  ═══════════════════════════════════════════"

if [[ $DRY_RUN -eq 1 ]]; then
    echo "  DRY RUN COMPLETE (no changes made)"
else
    echo "  BUILD COMPLETE"
    echo ""
    echo "  Next: make iso"
fi
