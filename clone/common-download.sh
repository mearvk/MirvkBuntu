#!/usr/bin/env bash
set -u

SOURCE_REPO="${SOURCE_REPO:-mearvk/Ubuntu.Determinant.Beta.Restricted}"
SOURCE_REF="${SOURCE_REF:-main}"
API_ROOT="https://api.github.com/repos/${SOURCE_REPO}"
RAW_ROOT="https://raw.githubusercontent.com/${SOURCE_REPO}/${SOURCE_REF}"
SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(CDPATH= cd -- "${SCRIPT_DIR}/.." && pwd)"

die() {
  printf 'clone: ERROR: %s\n' "$*" >&2
  return 1
}

url_encode_path() {
  printf '%s' "$1" | jq -sRr '@uri' | sed 's#%2F#/#g'
}

clone_tree() {
  if [[ $# -ne 2 ]]; then
    die "clone_tree requires <source-path> <destination>"
    return 1
  fi

  local source_path="$1"
  local destination="$2"
  local marker="${destination}/.clone-complete"

  # Destinations are rooted at the existing MirvkBuntu checkout.
  # Existing files are never overwritten.
  mkdir -p "$destination" || return 1

  if [[ -f "$marker" ]]; then
    printf 'clone: already complete: %s -> %s\n' "$source_path" "$destination"
    return 0
  fi

  command -v wget >/dev/null 2>&1 || {
    die "wget is required"
    return 1
  }
  command -v jq >/dev/null 2>&1 || {
    die "jq is required"
    return 1
  }

  local tree_json
  tree_json="$(wget -qO- "${API_ROOT}/git/trees/${SOURCE_REF}?recursive=1")" || {
    die "cannot read source tree: ${SOURCE_REPO}@${SOURCE_REF}"
    return 1
  }

  if ! jq -e 'type == "object" and (.tree | type == "array")' <<<"$tree_json" >/dev/null 2>&1; then
    die "GitHub returned an invalid tree response for ${SOURCE_REPO}@${SOURCE_REF}"
    return 1
  fi

  # A recursive repository tree can exceed GitHub's size limit. Do not abort:
  # use the entries GitHub returned and download only missing files. Existing
  # files remain untouched. A completion marker is intentionally not written
  # for a truncated tree because the result may be incomplete.
  local truncated=0
  if jq -e '.truncated == true' <<<"$tree_json" >/dev/null 2>&1; then
    truncated=1
    printf 'clone: warning: GitHub returned a truncated tree; downloading available entries only: %s\n' "$source_path" >&2
  fi

  if ! jq -e --arg p "$source_path"     '.tree | any(.[]; .path == $p or (.path | startswith($p + "/")))'     <<<"$tree_json" >/dev/null; then
    printf 'clone: source path not present in returned tree, skipping: %s\n' "$source_path"
    return 0
  fi

  local count=0
  local skipped=0
  local failed=0
  local path type relative target encoded

  while IFS=$'\t' read -r path type; do
    [[ "$type" == "blob" ]] || continue
    [[ "$path" == "$source_path" || "$path" == "$source_path/"* ]] || continue

    relative="${path#"$source_path"/}"
    target="${destination}/${relative}"
    if [[ "$path" == "$source_path" ]]; then
      relative="$(basename "$path")"
      target="${destination}/${relative}"
    fi

    encoded="$(url_encode_path "$path")" || {
      failed=$((failed + 1))
      continue
    }

    mkdir -p "$(dirname "$target")" || {
      failed=$((failed + 1))
      continue
    }

    # Never overwrite an existing file. This is deliberately checked before
    # wget so an existing repository file cannot be truncated or replaced.
    if [[ -e "$target" ]]; then
      skipped=$((skipped + 1))
      continue
    fi

    printf 'clone: downloading %s\n' "$path"
    if wget -q --show-progress -O "$target" "${RAW_ROOT}/${encoded}"; then
      count=$((count + 1))
    else
      rm -f "$target"
      failed=$((failed + 1))
    fi
  done < <(jq -r '.tree[] | [.path,.type] | @tsv' <<<"$tree_json")

  if (( failed != 0 )); then
    printf 'clone: %s failed downloads in %s\n' "$failed" "$source_path" >&2
    return 1
  fi

  if (( truncated == 1 )); then
    printf 'clone: incomplete tree; no completion marker written: %s\n' "$source_path" >&2
    return 0
  fi

  touch "$marker" || return 1
  printf 'clone: %s downloaded, %s already present: %s\n'     "$count" "$skipped" "$source_path"
}
