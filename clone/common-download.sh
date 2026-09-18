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

should_skip_path() {
  local path="$1"
  local component
  IFS='/' read -r -a components <<< "$path"
  for component in "${components[@]}"; do
    case "$component" in
      .gradle|.gradle-dist|build|out|target|dist|node_modules|__pycache__|.idea)
        return 0
        ;;
    esac
  done
  return 1
}

# GitHub's API is rate-limited for anonymous clients. Support either
# GITHUB_TOKEN or GH_TOKEN without requiring credentials for public repos.
github_api_wget() {
  local output="$1"
  local url="$2"
  local user_agent="MirvkBuntu-clone/1.0"
  local -a args=(
    --timeout=30
    --tries=3
    --waitretry=2
    --header="Accept: application/vnd.github+json"
    --header="X-GitHub-Api-Version: 2022-11-28"
    --header="User-Agent: ${user_agent}"
    -qO "${output}"
    "${url}"
  )

  if [[ -n "${GITHUB_TOKEN:-}" ]]; then
    args+=(--header="Authorization: Bearer ${GITHUB_TOKEN}")
  elif [[ -n "${GH_TOKEN:-}" ]]; then
    args+=(--header="Authorization: Bearer ${GH_TOKEN}")
  fi

  wget "${args[@]}"
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
    find "$destination" -type f -name '*.sh' -exec chmod +x -- {} + 2>/dev/null || true
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
  local filtered=0

  clone_directory() {
    local remote_path="$1"
    local local_directory="$2"
    local listing entry_type entry_name entry_path target encoded
    local api_tmp

    if should_skip_path "$remote_path"; then
      printf 'clone: skipping generated/cache path: %s\n' "$remote_path"
      filtered=$((filtered + 1))
      return 0
    fi

    api_tmp="$(mktemp)" || return 1
    encoded="$(url_encode_path "$remote_path")" || {
      rm -f "$api_tmp"
      return 1
    }

    if ! github_api_wget "$api_tmp" "${API_ROOT}/contents/${encoded}?ref=${SOURCE_REF}"; then
      printf 'clone: ERROR: cannot read source directory: %s\n' "$remote_path" >&2
      if [[ -s "$api_tmp" ]]; then
        printf 'clone: GitHub API response:\n' >&2
        sed -n '1,8p' "$api_tmp" >&2
      fi
      rm -f "$api_tmp"
      return 1
    fi

    listing="$(cat "$api_tmp")"
    rm -f "$api_tmp"

    if ! jq -e 'type == "array"' <<<"$listing" >/dev/null 2>&1; then
      printf 'clone: ERROR: invalid directory response: %s\n' "$remote_path" >&2
      printf '%s\n' "$listing" | sed -n '1,8p' >&2
      return 1
    fi

    while IFS=$'\t' read -r entry_type entry_name; do
      [[ -n "$entry_name" ]] || continue
      entry_path="${remote_path}/${entry_name}"
      target="${local_directory}/${entry_name}"

      if should_skip_path "$entry_path"; then
        printf 'clone: skipping generated/cache path: %s\n' "$entry_path"
        filtered=$((filtered + 1))
        continue
      fi

      case "$entry_type" in
        dir)
          mkdir -p "$target" || return 1
          clone_directory "$entry_path" "$target" || return 1
          ;;
        file)
          mkdir -p "$(dirname "$target")" || return 1

          if [[ -e "$target" ]]; then
            skipped=$((skipped + 1))
            continue
          fi

          encoded="$(url_encode_path "$entry_path")" || return 1
          printf 'clone: downloading %s\n' "$entry_path"
          if wget -q --show-progress --tries=3 --waitretry=2 -O "$target" "${RAW_ROOT}/${encoded}"; then
            count=$((count + 1))
          else
            rm -f "$target"
            printf 'clone: ERROR: failed download: %s\n' "$entry_path" >&2
            return 1
          fi
          ;;
        *)
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

  if ! find "$destination" -type f -name '*.sh' -exec chmod +x -- {} +; then
    printf 'clone: ERROR: cannot chmod +x shell scripts under %s\n' "$destination" >&2
    return 1
  fi

  touch "$marker" || return 1
  printf 'clone: %s downloaded, %s already present, %s generated/cache paths skipped: %s\n' \
    "$count" "$skipped" "$filtered" "$source_path"
}
