#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$BASH_SOURCE")" && pwd)"
source "$SCRIPT_DIR/common-download.sh"
for path in aptitude installer securejdk-installer; do
  clone_tree "$path" "$SCRIPT_DIR/../packages/$path"
done
