#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"

# Master native installer: compile/install all recent C/C++ utilities, then
# install platform service integration where the host supports it.
"$ROOT/installer/install-native.sh"

# Build the JavaFX installer application when the Java/Maven toolchain exists.
if command -v java >/dev/null 2>&1 && command -v mvn >/dev/null 2>&1; then
  echo "building JavaFX master installer"
  (cd "$ROOT/installer" && mvn -B clean package)
else
  echo "Java/Maven not available; native installation completed."
fi

echo "Master installation completed."
