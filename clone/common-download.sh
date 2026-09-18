#!/usr/bin/env bash
set -euo pipefail

SOURCE_REPO="${SOURCE_REPO:-mearvk/MirvkBuntu}"
SOURCE_REF="${SOURCE_REF:-main}"
API_ROOT="https://api.github.com/repos/$SOURCE_REPO"
RAW_ROOT="https://raw.githubusercontent.com/$SOURCE_REPO/$SOURCE_REF"
SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

die() {
  printf 'clone: ERROR: %s\n' "$*" >&2
  return 1
}

url_encode_path() {
  printf '%s' "$1" | jq -sRr '@uri' | sed 's#%2F#/#g'
}

should_skip_path() {
  local path="$1" component
  IFS='/' read -r -a components <<< "$path"
  for component in "${components[@]}"; do
    case "$component" in
      .gradle|.gradle-dist|node_modules|__pycache__|.idea|.git|.svn|.hg)
        return 0
        ;;
    esac
  done
  case "$path" in
    userland/chromium/chromium-src/.gemini|userland/chromium/chromium-src/.gemini/*)
      return 0
      ;;
    userland/chromium/chromium-src/agents|userland/chromium/chromium-src/agents/*)
      return 0
      ;;
  esac
  return 1
}

github_api_wget() {
  local output="$1" url="$2"
  local -a args=(
    --timeout=30 --tries=4 --waitretry=2
    --retry-on-http-error=403,408,429,500,502,503,504
    --header="Accept: application/vnd.github+json"
    --header="X-GitHub-Api-Version: 2022-11-28"
    --header="User-Agent: MirvkBuntu-clone/2.1"
    --content-on-error -qO "$output" "$url"
  )
  if [[ -n "${GITHUB_TOKEN:-}" ]]; then
    args+=(--header="Authorization: Bearer $GITHUB_TOKEN")
  elif [[ -n "${GH_TOKEN:-}" ]]; then
    args+=(--header="Authorization: Bearer $GH_TOKEN")
  fi
  wget "${args[@]}"
}

github_raw_download() {
  local output="$1" url="$2"
  local -a args=(
    --timeout=60 --tries=4 --waitretry=2
    --retry-on-http-error=403,408,429,500,502,503,504
    --header="User-Agent: MirvkBuntu-clone/2.1"
    --output-document="$output" "$url"
  )
  if [[ -n "${GITHUB_TOKEN:-}" ]]; then
    args+=(--header="Authorization: Bearer $GITHUB_TOKEN")
  elif [[ -n "${GH_TOKEN:-}" ]]; then
    args+=(--header="Authorization: Bearer $GH_TOKEN")
  fi
  wget "${args[@]}"
}

git_blob_sha() {
  local file="$1" size
  size="$(wc -c < "$file")"
  { printf 'blob %s\0' "$size"; cat "$file"; } | sha1sum | awk '{print $1}'
}

ensure_directory() {
  local directory="$1"
  if [[ -d "$directory" ]]; then
    return 0
  fi
  if ! mkdir -p -- "$directory"; then
    die "cannot create directory: $directory"
    return 1
  fi
  [[ -d "$directory" ]] || {
    die "directory was not created: $directory"
    return 1
  }
}

clone_tree() {
  [[ $# -eq 2 ]] || { die "clone_tree requires <source-path> <destination>"; return 1; }

  local source_path="${1%/}" destination="$2" marker
  marker="$destination/.clone-complete"

  printf 'clone: source repository = %s\n' "$SOURCE_REPO"
  printf 'clone: source ref        = %s\n' "$SOURCE_REF"
  printf 'clone: source path       = %s\n' "$source_path"
  printf 'clone: destination       = %s\n' "$destination"

  command -v wget >/dev/null 2>&1 || { die "wget is required"; return 1; }
  command -v jq >/dev/null 2>&1 || { die "jq is required"; return 1; }
  command -v sha1sum >/dev/null 2>&1 || { die "sha1sum is required"; return 1; }

  # A completed local clone is authoritative for this reconciliation step.
  # Do this before contacting GitHub so an existing, valid directory is not
  # reported as unreadable merely because the remote API is unavailable.
  if [[ -f "$marker" ]]; then
    printf 'clone: already complete locally: %s -> %s\n' "$source_path" "$destination"
    find "$destination" -type f -name '*.sh' -exec chmod +x -- {} + 2>/dev/null || true
    return 0
  fi

  if [[ -d "$destination" ]]; then
    printf 'clone: existing destination directory found: %s\n' "$destination"
  else
    ensure_directory "$destination" || return 1
  fi

  # Verify the authoritative source path before repairing a partial destination.
  local source_check_tmp
  source_check_tmp="$(mktemp)"
  if ! github_api_wget "$source_check_tmp" "$API_ROOT/contents/$(url_encode_path "$source_path")?ref=$SOURCE_REF&per_page=1&page=1"; then
    printf 'clone: ERROR: cannot read remote source directory: %s\n' "$source_path" >&2
    [[ -s "$source_check_tmp" ]] && sed -n '1,12p' "$source_check_tmp" >&2
    rm -f "$source_check_tmp"
    return 1
  fi
  if ! jq -e 'type == "array" or type == "object"' "$source_check_tmp" >/dev/null 2>&1; then
    printf 'clone: ERROR: invalid remote source response: %s\n' "$source_path" >&2
    sed -n '1,12p' "$source_check_tmp" >&2
    rm -f "$source_check_tmp"
    return 1
  fi
  rm -f "$source_check_tmp"

  local count=0 updated=0 skipped=0 filtered=0

  clone_directory() {
    local remote_path="$1" local_directory="$2"
    local api_tmp listing page page_count
    local entry_type entry_name entry_path target encoded expected_sha remote_size local_size temp actual_sha

    if should_skip_path "$remote_path"; then
      printf 'clone: skipping generated/cache path: %s\n' "$remote_path"
      filtered=$((filtered + 1))
      return 0
    fi

    ensure_directory "$local_directory" || return 1
    page=1

    while :; do
      api_tmp="$(mktemp)"
      encoded="$(url_encode_path "$remote_path")"

      if ! github_api_wget "$api_tmp" "$API_ROOT/contents/$encoded?ref=$SOURCE_REF&per_page=100&page=$page"; then
        printf 'clone: ERROR: cannot read remote source directory: %s (page %s)\n' "$remote_path" "$page" >&2
        [[ -s "$api_tmp" ]] && sed -n '1,12p' "$api_tmp" >&2
        rm -f "$api_tmp"
        return 1
      fi

      listing="$(cat "$api_tmp")"
      rm -f "$api_tmp"

      if ! jq -e 'type == "array"' <<<"$listing" >/dev/null 2>&1; then
        printf 'clone: ERROR: invalid remote directory response: %s\n' "$remote_path" >&2
        printf '%s\n' "$listing" | sed -n '1,12p' >&2
        return 1
      fi

      page_count="$(jq 'length' <<<"$listing")"

      while IFS=$'\t' read -r entry_type entry_name expected_sha remote_size; do
        [[ -n "$entry_name" ]] || continue
        entry_path="$remote_path/$entry_name"
        target="$local_directory/$entry_name"

        if should_skip_path "$entry_path"; then
          printf 'clone: skipping generated/cache path: %s\n' "$entry_path"
          filtered=$((filtered + 1))
          continue
        fi

        case "$entry_type" in
          dir)
            clone_directory "$entry_path" "$target" || return 1
            ;;
          file)
            ensure_directory "$(dirname "$target")" || return 1
            if [[ -f "$target" ]]; then
              local_size="$(wc -c < "$target")"
              if [[ "$local_size" == "$remote_size" ]] && [[ "$(git_blob_sha "$target")" == "$expected_sha" ]]; then
                skipped=$((skipped + 1))
                continue
              fi
              printf 'clone: refreshing changed file %s\n' "$entry_path"
              updated=$((updated + 1))
            else
              count=$((count + 1))
            fi

            temp="$target.clone-part"
            rm -f "$temp"
            encoded="$(url_encode_path "$entry_path")"

            github_raw_download "$temp" "$RAW_ROOT/$encoded" || {
              rm -f "$temp"
              printf 'clone: ERROR: failed download: %s\n' "$entry_path" >&2
              return 1
            }

            actual_sha="$(git_blob_sha "$temp")"
            if [[ "$actual_sha" != "$expected_sha" ]]; then
              rm -f "$temp"
              printf 'clone: ERROR: SHA-1 mismatch for %s (expected %s, got %s)\n' "$entry_path" "$expected_sha" "$actual_sha" >&2
              return 1
            fi

            chmod --reference="$target" "$temp" 2>/dev/null || true
            mv -f "$temp" "$target"
            ;;
          symlink|submodule)
            printf 'clone: ERROR: unsupported non-file GitHub entry %s: %s\n' "$entry_type" "$entry_path" >&2
            return 1
            ;;
          *)
            printf 'clone: ERROR: unsupported GitHub entry type %s: %s\n' "$entry_type" "$entry_path" >&2
            return 1
            ;;
        esac
      done < <(jq -r '.[] | [.type,.name,.sha,(.size // 0)] | @tsv' <<<"$listing")

      (( page_count < 100 )) && break
      page=$((page + 1))
    done
  }

  if ! clone_directory "$source_path" "$destination"; then
    printf 'clone: incomplete clone: %s -> %s\n' "$source_path" "$destination" >&2
    rm -f "$marker"
    return 1
  fi

  find "$destination" -type f -name '*.sh' -exec chmod +x -- {} +
  touch "$marker"
  printf 'clone: reconciled %s new, %s updated, %s unchanged, %s filtered: %s\n' "$count" "$updated" "$skipped" "$filtered" "$source_path"
}
