#!/usr/bin/env bash
set -u

# Aptitude local health collector. Read-only by design.
# Exit: 0 healthy, 1 attention, 2 unable to complete required checks.

state="GREEN"
notes=()

set_attention() {
  [[ "$state" == "GREEN" ]] && state="YELLOW"
}

emit() {
  printf '%s=%s\n' "$1" "$2"
}

emit system "$(uname -s)"
emit kernel "$(uname -r)"
emit architecture "$(uname -m)"

if [[ -r /proc/meminfo ]]; then
  available_kb=$(awk '/MemAvailable:/ {print $2}' /proc/meminfo)
  emit memory_available_kb "$available_kb"
  if [[ "$available_kb" =~ ^[0-9]+$ ]] && (( available_kb < 262144 )); then
    set_attention
    notes+=("available RAM is below the conservative 256 MiB inspection threshold")
  fi
else
  notes+=("RAM information unavailable")
fi

if [[ -r /proc/loadavg ]]; then
  emit load_1m "$(awk '{print $1}' /proc/loadavg)"
else
  notes+=("load information unavailable")
fi

if command -v df >/dev/null 2>&1; then
  root_used=$(df -P / | awk 'NR==2 {gsub(/%/,"",$5); print $5}')
  emit root_disk_used_percent "$root_used"
  if [[ "$root_used" =~ ^[0-9]+$ ]] && (( root_used >= 90 )); then
    set_attention
    notes+=("root filesystem is at or above 90% usage")
  fi
else
  notes+=("filesystem capacity check unavailable")
fi

if command -v systemctl >/dev/null 2>&1; then
  emit systemd "available"
  failed=$(systemctl --failed --no-legend --plain 2>/dev/null | awk 'NF {n++} END {print n+0}')
  emit failed_units "$failed"
  if (( failed > 0 )); then
    set_attention
    notes+=("one or more systemd units are failed")
  fi
else
  emit systemd "unavailable"
fi

if command -v journalctl >/dev/null 2>&1; then
  emit journal "available"
  # Read-only, recent high-priority evidence; do not treat ordinary warnings as failure.
  critical=$(journalctl -p 0..3 -b --no-pager -q 2>/dev/null | tail -n 200 | wc -l | tr -d ' ')
  emit recent_high_priority_journal_lines "$critical"
  if (( critical > 0 )); then
    set_attention
    notes+=("recent high-priority journal entries deserve review")
  fi
else
  emit journal "unavailable"
fi

if command -v sha256sum >/dev/null 2>&1; then
  emit sha256 "available"
else
  emit sha256 "unavailable"
  notes+=("SHA-256 utility unavailable")
fi

if [[ -f "${APTITUDE_MANIFEST:-/var/lib/aptitude/sha256.manifest}" ]]; then
  emit integrity_manifest "present"
else
  emit integrity_manifest "not_present"
  notes+=("no Aptitude integrity manifest is currently installed")
fi

if command -v apt-get >/dev/null 2>&1; then
  emit package_manager "apt-get"
else
  emit package_manager "not_detected"
fi

if ((${#notes[@]} == 0)); then
  emit result "HEALTHY"
  echo 'message=System is operating normally.'
  exit 0
fi

emit result "$state"
for note in "${notes[@]}"; do
  echo "note=$note"
done

[[ "$state" == "YELLOW" ]] && exit 1
exit 2
