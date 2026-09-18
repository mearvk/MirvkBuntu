#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$BASH_SOURCE")" && pwd)"
REPO_ROOT="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)"
ARCH="${ARCH:-amd64}"
ARTIFACT_ROOT="${ARTIFACT_ROOT:-$REPO_ROOT/build/work/native/artifacts}"
CHUNK_DIR="${CHUNK_DIR:-$ARTIFACT_ROOT/chunks}"
OUTPUT="${OUTPUT:-$ARTIFACT_ROOT/reassembled/mirvkbuntu-native-$ARCH.tar.zst}"

die(){ printf "reassemble-native: ERROR: %s\n" "$*" >&2; exit 1; }
command -v sha256sum >/dev/null || die "sha256sum is required"
command -v cat >/dev/null || die "cat is required"
command -v tar >/dev/null || die "tar is required"
[ -f "$CHUNK_DIR/SHA256SUMS" ] || die "missing $CHUNK_DIR/SHA256SUMS"
mkdir -p "$(dirname "$OUTPUT")"
(cd "$CHUNK_DIR" && sha256sum -c SHA256SUMS)
rm -f "$OUTPUT"
cat "$CHUNK_DIR"/mirvkbuntu-native-$ARCH.part-* > "$OUTPUT"
printf "reassemble-native: reassembled %s\n" "$OUTPUT"
tar --zstd -tf "$OUTPUT" >/dev/null
printf "reassemble-native: archive integrity verified\n"
tar --zstd -xf "$OUTPUT" -C "$(dirname "$OUTPUT")"
printf "reassemble-native: contents extracted under %s\n" "$(dirname "$OUTPUT")"