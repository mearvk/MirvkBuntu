#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"

bash "${ROOT}/sources/kernel/fetch-linux-kernel.sh"
bash "${ROOT}/sources/gnome/fetch-gnome-core.sh"

echo "MirvkBuntu basic source set is available under sources/work/."
