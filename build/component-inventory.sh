#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)"
REGISTRY="${MIRVKBUNTU_COMPONENT_REGISTRY:-$REPO_ROOT/build/CUSTOM_COMPONENTS.txt}"
OUT="${MIRVKBUNTU_COMPONENT_INVENTORY:-$REPO_ROOT/build/work/component-inventory.txt}"

[ -f "$REGISTRY" ] || { echo "component-inventory: registry missing: $REGISTRY" >&2; exit 2; }
mkdir -p "$(dirname "$OUT")"

required=(kernels file-systems sources packages userland user-interface gnome-source docs build)

{
echo "MirvkBuntu Component Inventory"
echo "Generated: $(date -u +%Y-%m-%dT%H:%M:%SZ)"
echo "Repository: $REPO_ROOT"
echo
echo "== Registered components =="
awk -F'|' '!/^[[:space:]]*#/ && NF >= 5 {print $0}' "$REGISTRY"
echo
echo "== Required component directories =="
for d in "${required[@]}"; do
  if [ -d "$REPO_ROOT/$d" ]; then
    count="$(find "$REPO_ROOT/$d" -type f -not -path '*/.git/*' 2>/dev/null | wc -l)"
    printf '%s | present | %s files\n' "$d" "$count"
  else
    printf '%s | MISSING\n' "$d"
  fi
done
echo
echo "== Additional top-level source/software directories =="
find "$REPO_ROOT" -mindepth 1 -maxdepth 1 -type d ! -name .git ! -name build ! -name clone ! -name kernels ! -name file-systems ! -name sources ! -name packages ! -name userland ! -name user-interface ! -name gnome-source ! -name docs -printf '%f\n' 2>/dev/null | sort
echo
echo "== Package manifest =="
if [ -f "$REPO_ROOT/packages/basic-packages.txt" ]; then awk '!/^[[:space:]]*#/ && NF {print $1}' "$REPO_ROOT/packages/basic-packages.txt" | sort -u; else echo "MISSING: packages/basic-packages.txt"; fi
echo
echo "== Build scripts =="
for f in build/native-build.sh build/build-desktop.sh build/build-slim.sh build/build-minimal.sh build/installer.sh build/builder.sh; do [ -f "$REPO_ROOT/$f" ] && echo "present | $f" || echo "missing | $f"; done
} > "$OUT"

for d in "${required[@]}"; do [ -d "$REPO_ROOT/$d" ] || { echo "component-inventory: required component missing: $d" >&2; exit 3; }; done
[ -f "$REPO_ROOT/packages/basic-packages.txt" ] || { echo "component-inventory: required package manifest missing" >&2; exit 4; }
echo "component-inventory: wrote $OUT"
