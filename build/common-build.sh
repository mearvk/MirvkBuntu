#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(CDPATH= cd -- "${SCRIPT_DIR}/.." && pwd)"
BUILD_ROOT="${BUILD_ROOT:-${REPO_ROOT}/build/work}"
OUTPUT_ROOT="${OUTPUT_ROOT:-${REPO_ROOT}/build/output}"
ISO_LABEL="${ISO_LABEL:-MIRVKBUNTU}"
UBUNTU_SUITE="${UBUNTU_SUITE:-noble}"
ARCH="${ARCH:-amd64}"
die(){ printf 'build: ERROR: %s\n' "$*" >&2; exit 1; }
require_commands(){ local c; for c in "$@"; do command -v "$c" >/dev/null 2>&1 || die "required command not found: $c"; done; }
prepare_dirs(){ mkdir -p "$BUILD_ROOT" "$OUTPUT_ROOT"; }
