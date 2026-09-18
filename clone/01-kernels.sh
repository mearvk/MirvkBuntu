#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common-download.sh"

KERNEL_ROOT="$SCRIPT_DIR/../kernels"
KERNEL_BASE_URL="${KERNEL_BASE_URL:-https://www.kernel.org/pub/linux/kernel}"
KERNEL_DOWNLOAD_DIR="$KERNEL_ROOT/.downloads"

clone_tree "kernels" "$KERNEL_ROOT"

mkdir -p "$KERNEL_DOWNLOAD_DIR"

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
  mkdir -p "$destination"

  if [ ! -s "$archive_path" ]; then
    wget --https-only --retry-connrefused --tries=5 --timeout=30 \
      -O "$archive_path" "$url"
  fi

  tar -xJf "$archive_path" -C "$destination"

  if [ ! -f "$source_tree/Makefile" ] || [ ! -f "$source_tree/Kconfig" ] || [ ! -d "$source_tree/arch" ]; then
    printf 'kernel: ERROR: downloaded Linux %s source is incomplete: %s\n' "$version" "$source_tree" >&2
    exit 1
  fi

  printf 'kernel: complete Linux %s source installed: %s\n' "$version" "$source_tree"
}

# These are the MirvkBuntu kernel versions represented by the repository's
# kernel source layout. The download is only used to complete an incomplete
# source tree; it does not replace a complete MirvkBuntu-provided source tree.
download_kernel_source "5.15.204"
download_kernel_source "6.18.27"
download_kernel_source "6.19.14"
download_kernel_source "7.0.4"

# Archives are bootstrap material, not native source-tree inputs.
# Keep them out of ordinary source discovery and out of Git.
rm -rf "$KERNEL_DOWNLOAD_DIR"
