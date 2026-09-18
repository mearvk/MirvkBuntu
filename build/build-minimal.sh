#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/common-build.sh"
require_commands live-build lb
prepare_dirs
WORK_DIR="${BUILD_ROOT}/minimal"
rm -rf "$WORK_DIR"; mkdir -p "$WORK_DIR"; cd "$WORK_DIR"
lb clean --purge >/dev/null 2>&1 || true
lb config --distribution "$UBUNTU_SUITE" --architectures "$ARCH" --binary-images iso-hybrid --debian-installer live --apt-recommends false --archive-areas "main restricted universe multiverse" --iso-application "MirvkBuntu" --iso-publisher "MEARVK LLC" --iso-volume "$ISO_LABEL-MINIMAL"
mkdir -p config/package-lists
cat > config/package-lists/mirvkbuntu-minimal.list.chroot <<'EOF'
linux-image-generic
systemd-sysv
network-manager
openssh-server
sudo
ca-certificates
curl
wget
nano
git
build-essential
EOF
lb build
mkdir -p "$OUTPUT_ROOT"
mv -f ./*.iso "$OUTPUT_ROOT/MirvkBuntu-minimal-${ARCH}.iso"
printf 'build: created %s\n' "$OUTPUT_ROOT/MirvkBuntu-minimal-${ARCH}.iso"
