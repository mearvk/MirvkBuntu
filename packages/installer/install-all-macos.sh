#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
"$ROOT/installer/install-native-macos.sh"
if command -v java >/dev/null 2>&1 && command -v mvn >/dev/null 2>&1; then
  echo "building JavaFX master installer"
  (cd "$ROOT/installer" && mvn -B clean package)
else
  echo "Java/Maven not available; native installation completed."
fi
echo "Master installation completed."
