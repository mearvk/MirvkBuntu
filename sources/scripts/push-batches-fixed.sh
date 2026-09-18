#!/usr/bin/env bash
# push-batches-fixed.sh — repository-root-safe 200 MB batch uploader
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(git -C "$SCRIPT_DIR" rev-parse --show-toplevel)"
GIT_DIR="$(git -C "$REPO_ROOT" rev-parse --git-dir)"
cd "$REPO_ROOT"

MAX_BATCH_BYTES=$((200 * 1024 * 1024))
BRANCH="$(git branch --show-current)"
LOG_FILE="$GIT_DIR/push-batches.log"
REMOTE_URL="https://github.com/mearvk/Ubuntu.Determinant.Beta.Restricted.git"

mkdir -p "$(dirname "$LOG_FILE")"
touch "$LOG_FILE"

printf '%s\n' '=== push-batches-fixed.sh ==='
printf 'Repository: %s\nBranch:     %s\nMax batch:  200 MB\n' "$REPO_ROOT" "$BRANCH"
printf 'Strategy:   Stream → stage → commit → push (immediate per batch)\n'
printf 'Event log:  %s\n\n' "$LOG_FILE"

log_event() {
    local level="$1" msg="$2"
    printf '[%s] %-7s %s\n' "$(date '+%Y-%m-%d %H:%M:%S')" "$level" "$msg" >> "$LOG_FILE"
}

origin="$(git remote get-url origin 2>/dev/null || true)"
if [[ -z "$origin" ]]; then
    git remote add origin "$REMOTE_URL"
    origin="$REMOTE_URL"
fi

case "$origin" in
  "$REMOTE_URL"|"${REMOTE_URL%.git}") ;;
  *)
    echo "ERROR: origin is not the expected repository: $origin" >&2
    exit 1
    ;;
esac

echo "Remote: $origin"
log_event INFO "Repository root: $REPO_ROOT"
log_event INFO "Git directory: $GIT_DIR"
log_event INFO "Branch: $BRANCH"

printf '%s\n' 'Credentials are intentionally not embedded in the remote URL.'
printf '%s\n' 'Use GitHub CLI, Git Credential Manager, SSH, or a configured PAT credential helper.'
printf '%s\n\n' 'This script never writes a PAT to the repository.'

# Collect tracked modifications and untracked files without changing directory context.
# NUL-delimited output avoids breaking on whitespace/newlines in filenames.
BATCH_FILE="$(mktemp)"
trap 'rm -f "$BATCH_FILE"' EXIT

batch_num=0
batch_size=0
batch_files=0

flush_batch() {
    (( batch_files > 0 )) || return 0
    batch_num=$((batch_num + 1))
    local mb
    mb="$(awk "BEGIN {printf \"%.1f\", $batch_size / 1048576}")"
    echo "--- Batch $batch_num: $batch_files files, $mb MB ---"
    log_event BATCH "Batch $batch_num: $batch_files files, $mb MB"

    git add --pathspec-from-file="$BATCH_FILE" --pathspec-file-nul
    git commit -m "Batch $batch_num: $batch_files files ($mb MB)"
    git push -u origin "$BRANCH"

    : > "$BATCH_FILE"
    batch_size=0
    batch_files=0
}

while IFS= read -r -d '' entry; do
    # git status --porcelain=v1 -z prefixes entries with XY status bytes.
    path="${entry:3}"
    [[ -n "$path" ]] || continue
    [[ -e "$path" || -L "$path" ]] || continue

    size="$(stat -c '%s' -- "$path" 2>/dev/null || stat -f '%z' -- "$path" 2>/dev/null || printf '0')"
    printf '%s\0' "$path" >> "$BATCH_FILE"
    batch_size=$((batch_size + size))
    batch_files=$((batch_files + 1))

    if (( batch_size >= MAX_BATCH_BYTES )); then
        flush_batch
    fi
done < <(git status --porcelain=v1 -z)

flush_batch

echo "=== Done. $batch_num batch(es) committed and pushed. ==="
log_event RUN "Completed: $batch_num batch(es)"
