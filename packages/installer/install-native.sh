#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PREFIX="${PREFIX:-/usr/local/bin}"
LIBEXEC="${LIBEXEC:-/usr/local/lib/ubuntu-determinant}"
SYSTEMD="${SYSTEMD:-/etc/systemd/system}"
CC="${CC:-cc}"
CFLAGS="${CFLAGS:--O2 -Wall -Wextra -Werror -std=c11}"

mkdir -p "$LIBEXEC"

if [[ -f "$ROOT/tools/xmc/Makefile" ]]; then
  echo "building and installing xmc"
  make -C "$ROOT/tools/xmc" clean all
  make -C "$ROOT/tools/xmc" PREFIX="$PREFIX" install
fi

build_tool() {
  local name="$1"
  local dir="$ROOT/tools/$name"
  [[ -f "$dir/$name.c" ]] || { echo "skip: $name source not present"; return; }
  echo "building $name"
  # shellcheck disable=SC2086
  "$CC" $CFLAGS -o "$LIBEXEC/$name.new" "$dir/$name.c"
  install -m 0755 "$LIBEXEC/$name.new" "$PREFIX/$name"
  rm -f "$LIBEXEC/$name.new"
}

build_tool limit
build_tool size
build_tool ctrmsctl

if [[ -f "$ROOT/tools/ctrmsctl/ctrmsctl.service" && -d /run/systemd/system ]]; then
  install -m 0644 "$ROOT/tools/ctrmsctl/ctrmsctl.service" "$SYSTEMD/ctrmsctl.service"
  systemctl daemon-reload
  systemctl enable --now ctrmsctl.service
fi

install -m 0755 "$ROOT/tools/gcc/download-gcc.sh" "$LIBEXEC/download-gcc.sh"
install -m 0755 "$ROOT/tools/gcc/extract-gcc.sh" "$LIBEXEC/extract-gcc.sh"

echo "Native tools installed to $PREFIX"
