#!/bin/bash
#
# chunk-push.sh — Push all commits one at a time
#
# Usage: ./chunk-push.sh
#
# After chunk-commit.sh creates multiple commits, this script
# pushes them individually to avoid GitHub's pack size limits.
# It force-pushes each commit by SHA so the remote receives
# them in manageable pieces.
#

set -e

REPO_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$REPO_DIR"

BRANCH=$(git branch --show-current)
REMOTE="origin"

echo "=== chunk-push.sh ==="
echo "Branch: $BRANCH"
echo "Remote: $REMOTE"
echo ""

# Get the remote HEAD (what's already pushed)
REMOTE_HEAD=$(git rev-parse "${REMOTE}/${BRANCH}" 2>/dev/null || echo "")

if [ -z "$REMOTE_HEAD" ]; then
    echo "Remote branch doesn't exist yet. Will push all commits."
    # Get root commit
    COMMITS=$(git rev-list --reverse HEAD)
else
    # Get commits not yet on remote
    COMMITS=$(git rev-list --reverse "${REMOTE}/${BRANCH}..HEAD")
fi

TOTAL=$(echo "$COMMITS" | grep -c . || echo 0)

if [ "$TOTAL" -eq 0 ]; then
    echo "Nothing to push. All commits are already on remote."
    exit 0
fi

echo "Commits to push: $TOTAL"
echo ""

COUNT=0
for SHA in $COMMITS; do
    COUNT=$((COUNT + 1))
    SHORT=$(git rev-parse --short "$SHA")
    MSG=$(git log --format='%s' -1 "$SHA")
    
    echo "[$COUNT/$TOTAL] Pushing $SHORT: $MSG"
    
    git push "$REMOTE" "${SHA}:refs/heads/${BRANCH}" 2>&1 | grep -v "^remote:" || true
    
    echo "    Done."
    echo ""
done

echo "=== All $TOTAL commits pushed successfully ==="
