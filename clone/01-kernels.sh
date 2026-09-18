#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
KERNEL_ROOT="$SCRIPT_DIR/../kernels"

# Kernel source acquisition is deliberately independent of the recursive
# repository clone. The kernels directory can contain large source trees and
# must not be blocked by unrelated helper files (for example git.sh).
KERNEL_BASE_URL="${KERNEL_BASE_URL:-https://www.kernel.org/pub/linux/kernel}"
KERNEL_DOWNLOAD_DIR="$KERNEL_ROOT/.downloads"

ensure_kernel_directory() {
  local directory="$1"
  if [[ -d "$directory" ]]; then
    return 0
  fi
  if ! mkdir -p -- "$directory"; then
    printf 'kernel: ERROR: cannot create directory: %s\n' "$directory" >&2
    return 1
  fi
  [[ -d "$directory" ]] || {
    printf 'kernel: ERROR: directory was not created: %s\n' "$directory" >&2
    return 1
  }
}

ensure_kernel_directory "$KERNEL_ROOT"
ensure_kernel_directory "$KERNEL_DOWNLOAD_DIR"

download_kernel_source() {
  local version="$1"
  local major="${version%%.*}"
  local archive="linux-$version.tar.xz"
  local url="$KERNEL_BASE_URL/v$major.x/$archive"
  local archive_path="$KERNEL_DOWNLOAD_DIR/$archive"
  local destination="$KERNEL_ROOT/linux-$version"
  local source_tree="$destination/linux-$version"

  if [ -f "$source_tree/Makefile" ] && [ -f "$source_tree/Kconfig" ] && [ -d "$source_tree/arch" ]; then
    printf 'kernel: complete source already present: %s\n' "$source_tree"
    return 0
  fi

  printf 'kernel: complete source missing for Linux %s; downloading %s\n' "$version" "$url"
  rm -rf "$destination"
  ensure_kernel_directory "$destination"

  if [ ! -s "$archive_path" ]; then
    wget --https-only --retry-connrefused --tries=5 --timeout=30 \
      -O "$archive_path" "$url"
  fi

  if ! tar -xJf "$archive_path" -C "$destination"; then
    printf 'kernel: ERROR: failed to extract Linux %s: %s\n' "$version" "$archive_path" >&2
    exit 1
  fi

  if [ ! -f "$source_tree/Makefile" ] || [ ! -f "$source_tree/Kconfig" ] || [ ! -d "$source_tree/arch" ]; then
    printf 'kernel: ERROR: downloaded Linux %s source is incomplete: %s\n' "$version" "$source_tree" >&2
    exit 1
  fi

  printf 'kernel: complete Linux %s source installed: %s\n' "$version" "$source_tree"
}

download_kernel_source "5.15.204"
download_kernel_source "6.18.27"
download_kernel_source "6.19.14"
download_kernel_source "7.0.4"

rm -rf "$KERNEL_DOWNLOAD_DIR"
printf 'kernel: all declared MirvkBuntu kernel source trees are complete.\n'