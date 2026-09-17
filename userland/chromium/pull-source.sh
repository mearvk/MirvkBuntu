#!/usr/bin/env bash
# Acquire Chromium source for the Ubuntu White Edition build.
# No credentials are embedded or requested by this script.
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR="${CHROMIUM_SRC:-${SCRIPT_DIR}/chromium-src}"
DEPOT_TOOLS="${DEPOT_TOOLS:-${SCRIPT_DIR}/depot_tools}"

usage() {
  cat <<'EOF'
Usage: pull-source.sh [check|fetch|hooks]

Environment:
  CHROMIUM_SRC   Chromium checkout directory (default: ./chromium-src)
  DEPOT_TOOLS    depot_tools directory (default: ./depot_tools)
EOF
}

check_tools() {
  command -v git >/dev/null 2>&1 || { echo "ERROR: git is required." >&2; exit 1; }
  command -v python3 >/dev/null 2>&1 || { echo "ERROR: python3 is required." >&2; exit 1; }
  if [[ ! -x "${DEPOT_TOOLS}/fetch" ]]; then
    echo "ERROR: depot_tools/fetch not found: ${DEPOT_TOOLS}" >&2
    echo "Install depot_tools separately, or set DEPOT_TOOLS." >&2
    exit 1
  fi
}

check_layout() {
  if [[ -e "${SRC_DIR}" && ! -d "${SRC_DIR}" ]]; then
    echo "ERROR: Chromium source target exists but is not a directory: ${SRC_DIR}" >&2
    exit 1
  fi
  if [[ -d "${SRC_DIR}" && -e "${SRC_DIR}/.git" && ! -d "${SRC_DIR}/chrome" ]]; then
    echo "ERROR: unusual Chromium checkout structure: ${SRC_DIR}" >&2
    exit 1
  fi
}

case "${1:-check}" in
  check)
    check_tools
    check_layout
    echo "Chromium source acquisition prerequisites look valid."
    ;;
  fetch)
    check_tools
    check_layout
    mkdir -p "$(dirname -- "${SRC_DIR}")"
    export PATH="${DEPOT_TOOLS}:${PATH}"
    if [[ -d "${SRC_DIR}/.git" ]]; then
      echo "Chromium checkout already exists: ${SRC_DIR}"
      git -C "${SRC_DIR}" status --short
    else
      "${DEPOT_TOOLS}/fetch" --nohooks --no-history chromium
      # fetch uses its working directory; move only if a non-default target was requested.
      if [[ "$(pwd)/src" != "${SRC_DIR}" && -d "$(pwd)/src" ]]; then
        mv "$(pwd)/src" "${SRC_DIR}"
      fi
    fi
    ;;
  hooks)
    check_tools
    [[ -d "${SRC_DIR}/.git" ]] || { echo "ERROR: Chromium checkout not found: ${SRC_DIR}" >&2; exit 1; }
    export PATH="${DEPOT_TOOLS}:${PATH}"
    (cd "${SRC_DIR}" && gclient runhooks)
    ;;
  *)
    usage
    exit 2
    ;;
esac
