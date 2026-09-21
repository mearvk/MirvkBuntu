#!/usr/bin/env bash
set -euo pipefail

# MirvkBuntu kernel source acquisition helper.
# Usage:
#   bash kernels/git.sh [VERSION ...]
#
# The script downloads complete Linux kernel source archives into the
# repository's declared kernels/<version>/linux-<version> layout and verifies
# the files required by build/native-build.sh.

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
KERNEL_ROOT="$SCRIPT_DIR"
KERNEL_BASE_URL="${KERNEL_BASE_URL:-https://www.kernel.org/pub/linux/kernel}"
DOWNLOAD_DIR="$KERNEL_ROOT/.downloads"

mkdir -p "$DOWNLOAD_DIR"

download_kernel() {
  local version="$1"
  local major archive url archive_path destination source_tree

  major="${version%%.*}"
  archive="linux-$version.tar.xz"
  url="$KERNEL_BASE_URL/v$major.x/$archive"
  archive_path="$DOWNLOAD_DIR/$archive"
  destination="$KERNEL_ROOT/linux-$version"
  source_tree="$destination/linux-$version"

  if [ -f "$source_tree/Makefile" ] &&
     [ -f "$source_tree/Kconfig" ] &&
     [ -d "$source_tree/arch" ]; then
    printf 'kernel-git: complete source already present: %s\n' "$source_tree"
    return 0
  fi

  printf 'kernel-git: downloading Linux %s\n' "$version"
  printf 'kernel-git: %s\n' "$url"

  rm -rf "$destination"
  mkdir -p "$destination"

  if [ ! -s "$archive_path" ]; then
    wget --https-only       --retry-connrefused       --tries=5       --timeout=30       -O "$archive_path"       "$url"
  fi

  tar -xJf "$archive_path" -C "$destination"

  if [ ! -f "$source_tree/Makefile" ] ||
     [ ! -f "$source_tree/Kconfig" ] ||
     [ ! -d "$source_tree/arch" ]; then
    printf 'kernel-git: ERROR: incomplete Linux %s source: %s\n'       "$version" "$source_tree" >&2
    return 1
  fi

  printf 'kernel-git: verified complete Linux %s source: %s\n'     "$version" "$source_tree"
}

# Default to the current kernel.org longterm (LTS) releases. These are real,
# downloadable versions; pass explicit versions on the command line to override.
# (Previous defaults 6.18.27 / 6.19.14 / 7.0.4 were not published on kernel.org
# and caused every download to 404, leaving the native build with no source.)
versions=( "$@" )
if [ "${#versions[@]}" -eq 0 ]; then
  versions=(6.12.110 6.6.157 5.15.204)
fi

status=0
for version in "${versions[@]}"; do
  download_kernel "$version" || status=1
done

rm -rf "$DOWNLOAD_DIR"
exit "$status"
