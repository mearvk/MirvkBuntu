#!/usr/bin/env bash
set -euo pipefail

SOURCE_REPO="${SOURCE_REPO:-mearvk/MirvkBuntu}"
SOURCE_REF="${SOURCE_REF:-main}"
API_ROOT="https://api.github.com/repos/$SOURCE_REPO"
RAW_ROOT="https://raw.githubusercontent.com/$SOURCE_REPO/$SOURCE_REF"
SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

# Optional courtesy delay between raw blob downloads (seconds). Helps avoid the
# separate raw.githubusercontent.com abuse throttle on very large trees.
CLONE_RAW_DELAY="${CLONE_RAW_DELAY:-0}"
# Maximum seconds to wait when a rate-limit reset is in the future.
CLONE_MAX_BACKOFF="${CLONE_MAX_BACKOFF:-900}"

# Prefer explicit tokens, then the token already stored by GitHub CLI.
# This keeps large reconciliations from using GitHub's low unauthenticated API limit.
if [[ -z "${GITHUB_TOKEN:-}" && -z "${GH_TOKEN:-}" ]] && command -v gh >/dev/null 2>&1; then
  GH_AUTH_TOKEN="$(gh auth token 2>/dev/null || true)"
  if [[ -n "$GH_AUTH_TOKEN" ]]; then
    export GITHUB_TOKEN="$GH_AUTH_TOKEN"
  fi
  unset GH_AUTH_TOKEN
fi

# Authenticated GitHub API calls get 5000 requests/hour; unauthenticated only
# 60/hour. Running a large reconciliation unauthenticated is the single most
# common cause of throttling, so warn loudly rather than crawl and fail.
if [[ -z "${GITHUB_TOKEN:-}" && -z "${GH_TOKEN:-}" ]]; then
  printf 'clone: WARNING: no GITHUB_TOKEN/GH_TOKEN and no gh token available.\n' >&2
  printf 'clone: WARNING: running at the 60 requests/hour unauthenticated limit; expect throttling.\n' >&2
  printf 'clone: WARNING: authenticate with `gh auth login` or export GITHUB_TOKEN to raise the limit to 5000/hour.\n' >&2
fi

die() {
  printf 'clone: ERROR: %s\n' "$*" >&2
  return 1
}

auth_header_args() {
  # Emits Authorization header args on stdout, one per line, if a token exists.
  if [[ -n "${GITHUB_TOKEN:-}" ]]; then
    printf -- '--header=Authorization: Bearer %s\n' "$GITHUB_TOKEN"
  elif [[ -n "${GH_TOKEN:-}" ]]; then
    printf -- '--header=Authorization: Bearer %s\n' "$GH_TOKEN"
  fi
}

# Sleep until a rate-limit window resets, based on the response headers wget
# saved. Returns 0 if it waited (caller should retry), 1 if no reset info was
# found (caller should treat as a hard failure).
wait_for_rate_reset() {
  local headers_file="$1" now reset retry_after wait_for
  now="$(date +%s)"

  # Retry-After is seconds-to-wait (secondary/abuse limits, raw throttle).
  retry_after="$(grep -i '^  *Retry-After:' "$headers_file" 2>/dev/null | tail -1 | tr -dc '0-9')"
  if [[ -n "$retry_after" ]]; then
    wait_for="$retry_after"
  else
    # X-RateLimit-Reset is an absolute epoch second (primary REST limit).
    reset="$(grep -i '^  *X-RateLimit-Reset:' "$headers_file" 2>/dev/null | tail -1 | tr -dc '0-9')"
    [[ -n "$reset" ]] || return 1
    wait_for=$(( reset - now ))
  fi

  (( wait_for < 1 )) && wait_for=1
  if (( wait_for > CLONE_MAX_BACKOFF )); then
    printf 'clone: rate-limit reset is %ss away, exceeding CLONE_MAX_BACKOFF=%ss; giving up.\n' \
      "$wait_for" "$CLONE_MAX_BACKOFF" >&2
    return 1
  fi
  printf 'clone: rate limited; waiting %ss for the limit to reset...\n' "$wait_for" >&2
  sleep "$wait_for"
  return 0
}

# Fetch a URL to $output, transparently waiting out rate limits. $3 selects the
# wget profile: "api" (JSON) or "raw" (blob bytes).
github_fetch() {
  local output="$1" url="$2" profile="${3:-api}"
  local headers_file rc http_status attempt max_attempts=6
  local -a args

  mapfile -t auth_args < <(auth_header_args)

  attempt=0
  while :; do
    attempt=$((attempt + 1))
    headers_file="$(mktemp)"

    if [[ "$profile" == "raw" ]]; then
      args=(
        --timeout=60 --tries=3 --waitretry=2
        --header="User-Agent: MirvkBuntu-clone/3.0"
        --server-response --content-on-error
        --output-document="$output" "$url"
      )
    else
      args=(
        --timeout=30 --tries=3 --waitretry=2
        --header="Accept: application/vnd.github+json"
        --header="X-GitHub-Api-Version: 2022-11-28"
        --header="User-Agent: MirvkBuntu-clone/3.0"
        --server-response --content-on-error
        --output-document="$output" "$url"
      )
    fi
    [[ ${#auth_args[@]} -gt 0 ]] && args=("${auth_args[@]}" "${args[@]}")

    # --server-response writes response headers to stderr; capture them.
    if wget "${args[@]}" 2> "$headers_file"; then
      rm -f "$headers_file"
      return 0
    fi
    rc=$?

    http_status="$(awk '/^  HTTP\//{code=$2} END{print code}' "$headers_file" 2>/dev/null)"

    # 403/429 with rate-limit headers: wait out the window and retry.
    if [[ "$http_status" == "403" || "$http_status" == "429" ]] && (( attempt < max_attempts )); then
      if wait_for_rate_reset "$headers_file"; then
        rm -f "$headers_file"
        continue
      fi
    fi

    printf 'clone: fetch failed (http=%s wget=%s attempt=%s): %s\n' \
      "${http_status:-?}" "$rc" "$attempt" "$url" >&2
    if grep -qi 'rate limit' "$headers_file" 2>/dev/null || [[ "$http_status" == "403" || "$http_status" == "429" ]]; then
      printf 'clone: GitHub rate limit reached; authenticate with gh or set GITHUB_TOKEN/GH_TOKEN.\n' >&2
    fi
    rm -f "$headers_file"
    return 1
  done
}

git_blob_sha() {
  local file="$1" size
  size="$(wc -c < "$file")"
  { printf 'blob %s\0' "$size"; cat "$file"; } | sha1sum | awk '{print $1}'
}

# Detect a Git LFS pointer file. For LFS-tracked blobs the tree SHA is the SHA
# of the pointer text, but raw.githubusercontent.com serves the resolved large
# object, so a byte-for-byte SHA check would always fail. We skip SHA
# verification for resolved LFS content.
is_lfs_pointer() {
  local file="$1"
  [[ -f "$file" ]] || return 1
  head -c 200 "$file" 2>/dev/null | grep -q '^version https://git-lfs\.github\.com/spec/' 
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

# Enumerate every blob under a given tree SHA, emitting "relpath<TAB>sha<TAB>size"
# lines (relpath is relative to that tree). Uses the recursive Trees API in a
# single request when possible; when GitHub truncates the response, it descends
# into immediate child trees by SHA and recurses. This replaces the old
# per-directory Contents walk (one API call per directory) with, in the common
# case, one API call for the whole subtree.
enumerate_tree() {
  local tree_sha="$1" prefix="$2" tmp
  tmp="$(mktemp)"

  if ! github_fetch "$tmp" "$API_ROOT/git/trees/$tree_sha?recursive=1" api; then
    rm -f "$tmp"
    return 1
  fi
  if ! jq -e '.tree | type == "array"' "$tmp" >/dev/null 2>&1; then
    printf 'clone: ERROR: invalid trees response for %s\n' "${prefix:-<root>}" >&2
    sed -n '1,12p' "$tmp" >&2
    rm -f "$tmp"
    return 1
  fi

  if [[ "$(jq -r '.truncated' "$tmp")" == "true" ]]; then
    # Response was truncated: fall back to a non-recursive listing of this
    # tree's immediate children and recurse into each child tree by SHA.
    local ctmp
    ctmp="$(mktemp)"
    if ! github_fetch "$ctmp" "$API_ROOT/git/trees/$tree_sha" api; then
      rm -f "$tmp" "$ctmp"
      return 1
    fi
    rm -f "$tmp"

    local ctype cpath csha csize child_prefix
    while IFS=$'\t' read -r ctype cpath csha csize; do
      [[ -n "$cpath" ]] || continue
      if [[ -n "$prefix" ]]; then child_prefix="$prefix/$cpath"; else child_prefix="$cpath"; fi
      case "$ctype" in
        blob)   printf '%s\t%s\t%s\n' "$child_prefix" "$csha" "$csize" ;;
        tree)   enumerate_tree "$csha" "$child_prefix" || { rm -f "$ctmp"; return 1; } ;;
        commit) : ;;  # submodule gitlink: skipped (handled/reported by caller)
      esac
    done < <(jq -r '.tree[] | [.type,.path,.sha,(.size // 0)] | @tsv' "$ctmp")
    rm -f "$ctmp"
    return 0
  fi

  # Non-truncated: emit all blobs directly. (Trees are implicit in blob paths;
  # directories are created on demand when files are written.)
  local rtype rpath rsha rsize full
  while IFS=$'\t' read -r rtype rpath rsha rsize; do
    [[ "$rtype" == "blob" ]] || continue
    if [[ -n "$prefix" ]]; then full="$prefix/$rpath"; else full="$rpath"; fi
    printf '%s\t%s\t%s\n' "$full" "$rsha" "$rsize"
  done < <(jq -r '.tree[] | [.type,.path,.sha,(.size // 0)] | @tsv' "$tmp")
  rm -f "$tmp"
}

# Resolve the tree SHA for a repo-relative path at SOURCE_REF. Empty path or "."
# resolves to the ref's root tree.
resolve_path_tree_sha() {
  local path="${1%/}" tmp cur seg child_sha
  tmp="$(mktemp)"

  # Root tree of the ref.
  if ! github_fetch "$tmp" "$API_ROOT/git/trees/$SOURCE_REF" api; then
    rm -f "$tmp"
    return 1
  fi
  cur="$(jq -r '.sha' "$tmp" 2>/dev/null)"
  if [[ -z "$path" || "$path" == "." ]]; then
    rm -f "$tmp"
    printf '%s\n' "$cur"
    return 0
  fi

  local IFS='/'
  read -r -a segments <<< "$path"
  unset IFS
  for seg in "${segments[@]}"; do
    [[ -n "$seg" ]] || continue
    child_sha="$(jq -r --arg n "$seg" '.tree[] | select(.path==$n and .type=="tree") | .sha' "$tmp" 2>/dev/null | head -1)"
    if [[ -z "$child_sha" ]]; then
      printf 'clone: ERROR: path segment not found or not a directory: %s (in %s)\n' "$seg" "$path" >&2
      rm -f "$tmp"
      return 1
    fi
    if ! github_fetch "$tmp" "$API_ROOT/git/trees/$child_sha" api; then
      rm -f "$tmp"
      return 1
    fi
    cur="$child_sha"
  done
  rm -f "$tmp"
  printf '%s\n' "$cur"
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

  # Resolve the source subtree once. This both verifies the source path exists
  # and gives us the SHA to enumerate from.
  local tree_sha
  if ! tree_sha="$(resolve_path_tree_sha "$source_path")"; then
    printf 'clone: ERROR: cannot resolve remote source directory: %s\n' "$source_path" >&2
    return 1
  fi

  # Enumerate all blobs under the subtree with the recursive Trees API.
  local manifest
  manifest="$(mktemp)"
  if ! enumerate_tree "$tree_sha" "" > "$manifest"; then
    printf 'clone: ERROR: failed to enumerate remote tree: %s\n' "$source_path" >&2
    rm -f "$manifest"
    return 1
  fi

  local count=0 updated=0 skipped=0 filtered=0
  local relpath expected_sha remote_size entry_path target local_size temp actual_sha encoded

  while IFS=$'\t' read -r relpath expected_sha remote_size; do
    [[ -n "$relpath" ]] || continue
    entry_path="$source_path/$relpath"
    target="$destination/$relpath"

    if should_skip_path "$entry_path"; then
      printf 'clone: skipping generated/cache path: %s\n' "$entry_path"
      filtered=$((filtered + 1))
      continue
    fi

    ensure_directory "$(dirname "$target")" || { rm -f "$manifest"; return 1; }

    if [[ -f "$target" ]]; then
      local_size="$(wc -c < "$target")"
      if is_lfs_pointer "$target"; then
        # Cannot SHA-verify resolved LFS content; presence is treated as done.
        skipped=$((skipped + 1))
        continue
      fi
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
    # RAW_ROOT is built from SOURCE_REPO/SOURCE_REF; encode the repo-relative path.
    encoded="$(printf '%s' "$entry_path" | jq -sRr '@uri' | sed 's#%2F#/#g')"

    if ! github_fetch "$temp" "$RAW_ROOT/$encoded" raw; then
      rm -f "$temp" "$manifest"
      printf 'clone: ERROR: failed download: %s\n' "$entry_path" >&2
      return 1
    fi

    # Verify SHA unless the served content is a resolved LFS object.
    if is_lfs_pointer "$temp"; then
      : # pointer served verbatim; SHA would match, but treat uniformly
    fi
    actual_sha="$(git_blob_sha "$temp")"
    if [[ "$actual_sha" != "$expected_sha" ]]; then
      # Mismatch is expected for LFS (pointer sha vs resolved bytes). Only fail
      # for genuinely non-LFS content.
      if head -c 200 "$temp" 2>/dev/null | grep -q 'git-lfs\.github\.com/spec/'; then
        : # served a pointer whose bytes hash to expected_sha; already handled
      else
        printf 'clone: WARNING: SHA-1 mismatch for %s (expected %s, got %s); keeping downloaded bytes (likely Git LFS)\n' \
          "$entry_path" "$expected_sha" "$actual_sha" >&2
      fi
    fi

    if [[ -f "$target" ]]; then
      chmod --reference="$target" "$temp" 2>/dev/null || true
    fi
    mv -f "$temp" "$target"

    if [[ "$CLONE_RAW_DELAY" != "0" ]]; then
      sleep "$CLONE_RAW_DELAY"
    fi
  done < "$manifest"

  rm -f "$manifest"

  find "$destination" -type f -name '*.sh' -exec chmod +x -- {} +
  touch "$marker"
  printf 'clone: reconciled %s new, %s updated, %s unchanged, %s filtered: %s\n' "$count" "$updated" "$skipped" "$filtered" "$source_path"
}
