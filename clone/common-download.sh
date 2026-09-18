#!/usr/bin/env bash
set -u
SOURCE_REPO="${SOURCE_REPO:-mearvk/Ubuntu.Determinant.Beta.Restricted}"
SOURCE_REF="${SOURCE_REF:-main}"
API_ROOT="https://api.github.com/repos/${SOURCE_REPO}"
RAW_ROOT="https://raw.githubusercontent.com/${SOURCE_REPO}/${SOURCE_REF}"
SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
die(){ printf 'clone: ERROR: %s\n' "$*" >&2; return 1; }
url_encode_path(){ printf '%s' "$1" | jq -sRr '@uri' | sed 's#%2F#/#g'; }
clone_tree(){
  local source_path="$1" destination="$2" marker="${destination}/.clone-complete"
  mkdir -p "$destination"
  [[ -f "$marker" ]] && { printf 'clone: already complete: %s -> %s\n' "$source_path" "$destination"; return 0; }
  command -v wget >/dev/null 2>&1 || die "wget is required"
  command -v jq >/dev/null 2>&1 || die "jq is required"
  local tree_json
  tree_json="$(wget -qO- "${API_ROOT}/git/trees/${SOURCE_REF}?recursive=1")" || die "cannot read source tree"
  if ! jq -e --arg p "$source_path" '.tree | any(.[]; .path == $p or (.path | startswith($p + "/")))' <<<"$tree_json" >/dev/null; then
    printf 'clone: source path not present, skipping: %s\n' "$source_path"; return 0
  fi
  local count=0 skipped=0 failed=0
  while IFS=$'\t' read -r path type; do
    [[ "$type" == "blob" ]] || continue
    [[ "$path" == "$source_path" || "$path" == "$source_path/"* ]] || continue
    local relative="${path#"$source_path"/}" target="${destination}/${relative}" encoded
    [[ "$path" == "$source_path" ]] && relative="$(basename "$path")" && target="${destination}/${relative}"
    encoded="$(url_encode_path "$path")"; mkdir -p "$(dirname "$target")"
    if [[ -s "$target" ]]; then skipped=$((skipped+1)); continue; fi
    printf 'clone: downloading %s\n' "$path"
    if wget -q --show-progress -O "$target" "${RAW_ROOT}/${encoded}"; then count=$((count+1)); else rm -f "$target"; failed=$((failed+1)); fi
  done < <(jq -r '.tree[] | [.path,.type] | @tsv' <<<"$tree_json")
  (( failed == 0 )) || { printf 'clone: %s failed downloads in %s\n' "$failed" "$source_path" >&2; return 1; }
  touch "$marker"; printf 'clone: %s downloaded, %s already present: %s\n' "$count" "$skipped" "$source_path"
}
