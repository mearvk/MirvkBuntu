#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
REPO_ROOT="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)"
CACHE="$REPO_ROOT/build/cache/packages"
WORK="$REPO_ROOT/build/work"
mkdir -p "$CACHE" "$WORK"
MANIFEST_SHA="$(sha256sum "$REPO_ROOT/packages/basic-packages.txt" | awk '{print $1}')"
command -v apt-cache >/dev/null 2>&1 || { echo "scout: apt-cache is required" >&2; exit 30; }
command -v apt-get >/dev/null 2>&1 || { echo "scout: apt-get is required" >&2; exit 30; }\n\n# Refresh package metadata before resolving/downloading. Set SCOUT_APT_UPDATE=0\n# when the caller has already refreshed the exact repositories it intends to use.\nif [ "${SCOUT_APT_UPDATE:-1}" = "1" ]; then\n  echo "scout: refreshing APT package metadata"\n  apt-get update\nfi
[ -f "$REPO_ROOT/packages/basic-packages.txt" ] || { echo "scout: package manifest missing" >&2; exit 31; }
python3 - "$REPO_ROOT/packages/basic-packages.txt" "$WORK/dependency-resolved.txt" <<'PY'
import re, subprocess, sys
manifest, out = sys.argv[1], sys.argv[2]
roots=[]
for line in open(manifest, encoding="utf-8"):
    line=line.split("#",1)[0]
    roots += [x for x in line.split() if not x.startswith("-")]
seen=set(); q=list(roots)
while q:
    p=q.pop()
    if p in seen: continue
    seen.add(p)
    r=subprocess.run(["apt-cache","depends",p],capture_output=True,text=True)
    for line in r.stdout.splitlines():
        m=re.match(r"\s*(?:Pre)?Depends:\s*([^ <|]+)",line)
        if m and m.group(1): q.append(m.group(1))
with open(out,"w",encoding="utf-8") as f:
    f.write("\n".join(sorted(seen))+"\n")
print(f"scout: requested packages: {len(roots)}")
print(f"scout: resolved package names: {len(seen)}")
PY
if [[ "${1:-}" == "--check" ]]; then
  echo "scout: dependency manifest: $WORK/dependency-resolved.txt"
  exit 0
fi
while IFS= read -r package; do
  [ -n "$package" ] || continue
  (cd "$CACHE" && apt-get download "$package")
done < "$WORK/dependency-resolved.txt"
{
  echo "MIRVKBUNTU_DEPENDENCY_CACHE=$CACHE"
  echo "PACKAGE_COUNT=$(wc -l < "$WORK/dependency-resolved.txt")"
  echo "PACKAGE_MANIFEST_SHA256=$MANIFEST_SHA"
} > "$WORK/dependency-cache.txt"
echo "scout: package cache ready: $CACHE"
