#!/usr/bin/env bash
set -euo pipefail

# Ubuntu White Edition installer platform adapter.
# This adapter accepts only fixed operation names; GUI input must never become
# arbitrary shell syntax.

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"

usage() {
  echo "Usage: $0 inspect | build-iso | install-rootfs <directory> | run-iso <iso> | vm <iso>"
}

require() { command -v "$1" >/dev/null 2>&1 || { echo "Missing required command: $1" >&2; exit 2; }; }

case "${1:-}" in
  inspect)
    echo "OS: $(uname -s)"
    echo "Kernel: $(uname -r)"
    echo "Architecture: $(uname -m)"
    command -v qemu-system-x86_64 >/dev/null 2>&1 && echo "QEMU: available" || echo "QEMU: unavailable"
    ;;
  build-iso)
    require make
    cd "$ROOT"
    make iso
    ;;
  install-rootfs)
    target="${2:-}"
    [[ -n "$target" && "$target" = /* ]] || { echo "Target must be an absolute directory" >&2; exit 2; }
    [[ -d "$target" ]] || { echo "Target does not exist: $target" >&2; exit 2; }
    echo "REVIEW REQUIRED: rootfs installation target: $target"
    echo "Use the verified rootfs installation operation from the professional installer."
    exit 3
    ;;
  run-iso)
    iso="${2:-}"
    [[ -f "$iso" ]] || { echo "ISO not found: $iso" >&2; exit 2; }
    require qemu-system-x86_64
    exec qemu-system-x86_64 -enable-kvm -m 4096 -cdrom "$iso" -boot d
    ;;
  vm)
    iso="${2:-}"
    [[ -f "$iso" ]] || { echo "ISO not found: $iso" >&2; exit 2; }
    require qemu-system-x86_64
    exec qemu-system-x86_64 -enable-kvm -m 4096 -cdrom "$iso" -boot d
    ;;
  *) usage; exit 2 ;;
esac
