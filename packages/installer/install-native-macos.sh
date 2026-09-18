#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PREFIX="${PREFIX:-/usr/local/bin}"
LIBEXEC="${LIBEXEC:-/usr/local/lib/ubuntu-determinant}"
CC="${CC:-cc}"
mkdir -p "$PREFIX" "$LIBEXEC"

if [[ -f "$ROOT/tools/xmc/Makefile" ]]; then
  echo "building and installing xmc"
  make -C "$ROOT/tools/xmc" clean all
  make -C "$ROOT/tools/xmc" PREFIX="$PREFIX" install
fi

for name in limit size ctrmsctl; do
  dir="$ROOT/tools/$name"
  src="$dir/$name.c"
  if [[ ! -f "$src" ]]; then echo "skip: $name source not present"; continue; fi
  echo "building $name"
  "$CC" -O2 -Wall -Wextra -Werror -std=c11 -o "$LIBEXEC/$name.new" "$src"
  install -m 0755 "$LIBEXEC/$name.new" "$PREFIX/$name"
  rm -f "$LIBEXEC/$name.new"
done

install -m 0755 "$ROOT/tools/gcc/download-gcc.sh" "$LIBEXEC/download-gcc.sh"
install -m 0755 "$ROOT/tools/gcc/extract-gcc.sh" "$LIBEXEC/extract-gcc.sh"
echo "Native tools installed to $PREFIX"
