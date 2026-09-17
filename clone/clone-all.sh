#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${SCRIPT_DIR}"

SCRIPTS=(
  01-kernels.sh
  02-file-systems.sh
  03-sources.sh
  04-packages.sh
  05-userland.sh
  06-user-interface.sh
  07-gnome-source.sh
  08-docs.sh
)

echo "MirvkBuntu source loader"
echo "Source: mearvk/Ubuntu.Determinant.Beta.Restricted (main)"
echo

for script in "${SCRIPTS[@]}"; do
    echo "==> ${script}"
    bash "${SCRIPT_DIR}/${script}"
done

echo
echo "All source-directory download stages completed."
