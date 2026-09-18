#!/usr/bin/env bash
set -euo pipefail

# Aptitude prototype: context-aware installer and integrity monitor.
# Inspection is read-only. State-changing operations require explicit `apply`.
# Continuous monitoring is opt-in and bounded.

SCAN_INTERVAL_SECONDS="${APTITUDE_SCAN_INTERVAL_SECONDS:-3}"
SCAN_CYCLES="${APTITUDE_SCAN_CYCLES:-1000}"
STATE_DIR="${APTITUDE_STATE_DIR:-/var/lib/aptitude}"
MANIFEST="${APTITUDE_MANIFEST:-$STATE_DIR/sha256.manifest}"
QUARANTINE="${APTITUDE_QUARANTINE:-$STATE_DIR/quarantine}"

usage() {
  cat <<'EOF'
Aptitude — context-aware installer and integrity monitor

Usage:
  aptitude inspect ARTIFACT
  aptitude plan ARTIFACT
  aptitude apply ARTIFACT
  aptitude verify NAME
  aptitude manifest ROOT
  aptitude check
  aptitude monitor ROOT
  aptitude repair FILE

Integrity policy:
  SHA-256 is the default content-integrity mechanism for tracked software.
  `manifest` creates a baseline; `check` verifies that baseline without
  rewriting it. The monitor is opt-in and bounded.
EOF
}

need_cmd() { command -v "$1" >/dev/null 2>&1; }

sample_resources() {
  local mem_available="unknown" load="unknown"
  [[ -r /proc/meminfo ]] && mem_available=$(awk '/MemAvailable:/ {print $2}' /proc/meminfo)
  [[ -r /proc/loadavg ]] && load=$(awk '{print $1}' /proc/loadavg)
  echo "resource.mem_available_kb=$mem_available"
  echo "resource.load_1m=$load"
}

artifact_info() {
  local file="$1"
  [[ -f "$file" ]] || { echo "artifact not found: $file" >&2; exit 2; }
  echo "artifact=$file"
  echo "sha256=$(sha256sum "$file" | awk '{print $1}')"
  echo "size=$(stat -c '%s' "$file")"
  echo "mode=$(stat -c '%A' "$file")"
  if file "$file" | grep -q 'ELF'; then
    echo "format=ELF"
    if need_cmd readelf; then
      readelf -h "$file" | awk -F: '/Class|Machine/ {gsub(/^ +/,"",$2); print tolower($1) "=" $2}'
      echo "dynamic_dependencies="
      readelf -d "$file" 2>/dev/null | awk '/NEEDED/ {gsub(/[\[\]]/,"",$5); print "  - " $5}' || true
    fi
  else
    echo "format=$(file -b "$file")"
  fi
}

surfaces() {
  echo "host=$(uname -s)"
  echo "kernel=$(uname -r)"
  echo "arch=$(uname -m)"
  for surface in systemctl crontab at getent ldconfig java javac; do
    if need_cmd "$surface"; then echo "surface.$surface=available"; else echo "surface.$surface=unavailable"; fi
  done
  [[ -d /run/systemd/system ]] && echo "surface.systemd_runtime=available" || echo "surface.systemd_runtime=unavailable"
  [[ -d /sys/fs/cgroup ]] && echo "surface.cgroup=available" || echo "surface.cgroup=unavailable"
}

inspect() {
  echo '--- ARTIFACT ---'
  artifact_info "$1"
  echo '--- HOST SURFACES ---'
  surfaces
  echo '--- RESOURCES ---'
  sample_resources
}

plan() {
  local file="$1"
  echo 'APTITUDE INSTALL PLAN'
  echo '====================='
  inspect "$file"
  echo
  echo 'proposed_operations='
  echo '  1. verify artifact identity and SHA-256'
  echo '  2. verify architecture/ABI compatibility'
  echo '  3. resolve required shared libraries'
  echo '  4. select an installation target'
  echo '  5. propose PATH/JAVA_HOME changes when applicable'
  echo '  6. detect systemd/cron/timer integration opportunities'
  echo '  7. request explicit authorization for privileged changes'
  echo '  8. stage files atomically'
  echo '  9. verify executable and installed metadata'
  echo ' 10. retain an installation evidence record'
  echo
  echo 'decision=dry-run; no system state changed'
}

apply() {
  local file="$1"
  echo 'Aptitude apply: conservative prototype'
  plan "$file"
  echo
  echo 'apply_status=not_applied'
  echo 'reason=prototype refuses implicit privileged or persistent system changes'
}

verify() {
  local name="$1"
  if [[ -n "$(command -v "$name" 2>/dev/null || true)" ]]; then
    echo "verified=$name"
    "$name" --version 2>&1 | head -n 2 || true
  else
    echo "not_found=$name"
    return 1
  fi
}

manifest_root() {
  local root="$1"
  [[ -d "$root" ]] || { echo "root not found: $root" >&2; exit 2; }
  mkdir -p "$(dirname "$MANIFEST")"
  local tmp_manifest
  tmp_manifest=$(mktemp "${MANIFEST}.XXXXXX")
  if ! find "$root" -xdev -type f \
    -not -path "$STATE_DIR/*" \
    -not -path '/proc/*' -not -path '/sys/*' -not -path '/dev/*' \
    -print0 | sort -z | xargs -0 -r sha256sum > "$tmp_manifest"; then
    rm -f -- "$tmp_manifest"
    echo 'manifest_status=failed' >&2
    return 1
  fi
  chmod 0644 "$tmp_manifest"
  mv -f -- "$tmp_manifest" "$MANIFEST"
  echo "manifest=$MANIFEST"
  echo "files=$(wc -l < "$MANIFEST")"
}

check_manifest() {
  [[ -f "$MANIFEST" ]] || { echo "manifest missing: $MANIFEST" >&2; return 2; }
  echo 'APTITUDE SHA-256 INTEGRITY CHECK'
  echo '================================'
  if sha256sum -c "$MANIFEST" --quiet; then
    echo 'integrity=clean'
    return 0
  fi
  echo 'integrity=changes_or_damage_detected'
  return 1
}

repair_file() {
  local file="$1"
  [[ -f "$file" ]] || { echo "file not found: $file" >&2; exit 2; }
  mkdir -p "$QUARANTINE"
  local stamp target
  stamp=$(date +%Y%m%d-%H%M%S)
  target="$QUARANTINE/$(basename "$file").$stamp"
  cp -a -- "$file" "$target"
  echo "quarantine_copy=$target"
  echo "current_sha256=$(sha256sum "$file" | awk '{print $1}')"
  echo 'repair_status=quarantined_only'
  echo 'note=trusted restoration source must be selected and verified before replacement'
}

monitor() {
  local root="$1" cycle=1
  [[ -d "$root" ]] || { echo "root not found: $root" >&2; exit 2; }
  [[ "$SCAN_INTERVAL_SECONDS" =~ ^[0-9]+$ ]] || { echo 'APTITUDE_SCAN_INTERVAL_SECONDS must be an integer' >&2; exit 2; }
  [[ "$SCAN_CYCLES" =~ ^[0-9]+$ ]] || { echo 'APTITUDE_SCAN_CYCLES must be an integer' >&2; exit 2; }
  echo "monitor_interval_seconds=$SCAN_INTERVAL_SECONDS"
  echo "monitor_cycles=$SCAN_CYCLES"
  [[ -f "$MANIFEST" ]] || { echo "manifest missing: $MANIFEST; run 'manifest ROOT' first" >&2; exit 2; }
  while (( cycle <= SCAN_CYCLES )); do
    echo "--- check $cycle/$SCAN_CYCLES $(date -Is) ---"
    sample_resources
    if [[ -r /proc/meminfo ]] && awk '/MemAvailable:/ {found=1; exit !($2 > 262144)} END {if (!found) exit 1}' /proc/meminfo; then
      check_manifest || true
    else
      echo 'scan=deferred_due_to_low_available_memory'
    fi
    (( cycle++ ))
    (( cycle <= SCAN_CYCLES )) && sleep "$SCAN_INTERVAL_SECONDS"
  done
}

case "${1:-}" in
  inspect|plan|apply)
    [[ $# -eq 2 ]] || { usage; exit 2; }
    "$1" "$2"
    ;;
  verify)
    [[ $# -eq 2 ]] || { usage; exit 2; }
    verify "$2"
    ;;
  manifest)
    [[ $# -eq 2 ]] || { usage; exit 2; }
    manifest_root "$2"
    ;;
  check)
    [[ $# -eq 1 ]] || { usage; exit 2; }
    check_manifest
    ;;
  monitor)
    [[ $# -eq 2 ]] || { usage; exit 2; }
    monitor "$2"
    ;;
  repair)
    [[ $# -eq 2 ]] || { usage; exit 2; }
    repair_file "$2"
    ;;
  *)
    usage
    exit 2
    ;;
esac
