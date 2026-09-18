#!/usr/bin/env bash
set -euo pipefail

# push-batches.sh — stream files into <=200 MB batches and push each batch.
# Canonical repository:
# https://github.com/mearvk/Ubuntu.Determinant.Beta.Restricted

REPO_URL="https://github.com/mearvk/Ubuntu.Determinant.Beta.Restricted.git"
REPO_DIR="$(cd "$(dirname "$0")/.." && pwd)"
MAX_BATCH_BYTES=$((200 * 1024 * 1024))

cd "$REPO_DIR"

git remote get-url origin >/dev/null 2>&1 \
  && git remote set-url origin "$REPO_URL" \
  || git remote add origin "$REPO_URL"

BRANCH="$(git branch --show-current)"
if [[ -z "$BRANCH" ]]; then
  echo "Detached HEAD: checkout a branch before pushing." >&2
  exit 1
fi

printf 'Repository: %s\n' "$REPO_URL"
printf 'Local root: %s\n' "$REPO_DIR"
printf 'Branch: %s\n' "$BRANCH"
printf 'Batch limit: 200 MB\n\n'

read -r -p 'GitHub username [mearvk]: ' GITHUB_USER
GITHUB_USER="${GITHUB_USER:-mearvk}"
read -r -s -p 'GitHub PAT (input hidden): ' GITHUB_PAT
echo
[[ -n "$GITHUB_PAT" ]] || { echo 'PAT is required.' >&2; exit 1; }

# The PAT is supplied only to this git invocation. It is never written into
# the remote URL or repository configuration.
git_push() {
  GIT_TERMINAL_PROMPT=0 \
  git -c credential.helper='!f() { printf "username=%s\\npassword=%s\\n" "$GITHUB_USER" "$GITHUB_PAT"; }; f' \
    push -u origin "$BRANCH"
}

BATCH_FILE="$(mktemp)"
trap 'rm -f "$BATCH_FILE"; unset GITHUB_PAT' EXIT

batch_num=0
batch_size=0
batch_files=0
total_scanned=0
total_size=0

flush_batch() {
  (( batch_files > 0 )) || return 0
  batch_num=$((batch_num + 1))
  local mb
  mb=$(awk "BEGIN {printf \"%.1f\", $batch_size / 1048576}")

  echo "--- Batch $batch_num: $batch_files files, $mb MB ---"
  git add --pathspec-from-file="$BATCH_FILE"
  git commit -m "Batch $batch_num: $batch_files files ($mb MB)" --quiet
  git_push

  batch_size=0
  batch_files=0
  : > "$BATCH_FILE"
}

# Include tracked modifications and untracked files, while preserving paths
# containing spaces. Rename/copy metadata is intentionally handled by git.
while IFS= read -r -d '' entry; do
  path="${entry:3}"
  [[ -e "$path" ]] || continue
  size=$(stat -c '%s' -- "$path" 2>/dev/null || stat -f '%z' -- "$path")
  printf '%s\0' "$path" >> "$BATCH_FILE"
  batch_size=$((batch_size + size))
  batch_files=$((batch_files + 1))
  total_scanned=$((total_scanned + 1))
  total_size=$((total_size + size))
  (( batch_size >= MAX_BATCH_BYTES )) && flush_batch
done < <(git status --porcelain=v1 -z)

flush_batch

total_mb=$(awk "BEGIN {printf \"%.1f\", $total_size / 1048576}")
echo "Done: $batch_num batch(es), $total_scanned files, $total_mb MB."
