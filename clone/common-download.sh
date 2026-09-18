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

  local failed=0
  local count=0
  local skipped=0

  clone_directory() {
    local remote_path="$1"
    local local_directory="$2"
    local listing entry_type entry_name entry_path target encoded

    encoded="$(url_encode_path "$remote_path")" || return 1
    listing="$(wget -qO- "${API_ROOT}/contents/${encoded}?ref=${SOURCE_REF}")" || {
      printf 'clone: ERROR: cannot read source directory: %s\n' "$remote_path" >&2
      return 1
    }

    if ! jq -e 'type == "array"' <<<"$listing" >/dev/null 2>&1; then
      printf 'clone: ERROR: invalid directory response: %s\n' "$remote_path" >&2
      return 1
    fi

    while IFS=$'\t' read -r entry_type entry_name; do
      [[ -n "$entry_name" ]] || continue
      entry_path="${remote_path}/${entry_name}"
      target="${local_directory}/${entry_name}"

      case "$entry_type" in
        dir)
          mkdir -p "$target" || return 1
          clone_directory "$entry_path" "$target" || return 1
          ;;
        file)
          mkdir -p "$(dirname "$target")" || return 1

          # Never overwrite an existing file.
          if [[ -e "$target" ]]; then
            skipped=$((skipped + 1))
            continue
          fi

          encoded="$(url_encode_path "$entry_path")" || return 1
          printf 'clone: downloading %s\n' "$entry_path"
          if wget -q --show-progress -O "$target" "${RAW_ROOT}/${encoded}"; then
            count=$((count + 1))

            # GitHub's Contents API does not preserve executable mode when
            # downloading raw file content. Restore the conventional mode for
            # shell scripts so cloned scripts can be executed directly.
            case "$entry_name" in
              *.sh)
                chmod 0755 "$target" || {
                  rm -f "$target"
                  printf 'clone: ERROR: cannot make executable: %s\n' "$entry_path" >&2
                  return 1
                }
                ;;
            esac
          else
            rm -f "$target"
            printf 'clone: ERROR: failed download: %s\n' "$entry_path" >&2
            return 1
          fi
          ;;
        *)
          # Ignore symlinks/submodules and other GitHub content types.
          ;;
      esac
    done < <(jq -r '.[] | [.type,.name] | @tsv' <<<"$listing")
  }

  if ! clone_directory "$source_path" "$destination"; then
    failed=1
  fi

  if (( failed != 0 )); then
    printf 'clone: incomplete clone: %s -> %s\n' "$source_path" "$destination" >&2
    return 1
  fi

  touch "$marker" || return 1
  printf 'clone: %s downloaded, %s already present: %s\n' \
    "$count" "$skipped" "$source_path"
}
