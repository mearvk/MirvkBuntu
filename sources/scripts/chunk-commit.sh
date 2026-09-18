#!/bin/bash
#
# chunk-commit.sh — Commit staged files in ~200MB chunks
#
# Usage: ./chunk-commit.sh
#
# This script takes all currently staged files (git add'd),
# unstages them, then re-stages and commits them in batches
# of approximately 200MB each.
#

set -e

REPO_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$REPO_DIR"

CHUNK_SIZE=$((200 * 1024 * 1024))  # 200 MB in bytes
CHUNK_NUM=1
CURRENT_SIZE=0
BATCH_FILE=$(mktemp /tmp/chunk_batch_XXXXXX.txt)

echo "=== chunk-commit.sh ==="
echo "Chunk size target: 200 MB"
echo ""

# Get all staged files with their sizes, sorted by path
# We use git diff --cached to get staged files
git diff --cached --name-only | while IFS= read -r f; do
    if [ -f "$f" ]; then
        stat --printf='%s %n\n' "$f"
    else
        echo "0 $f"
    fi
done | sort -k2 > /tmp/chunk_staged_files.txt

TOTAL_FILES=$(wc -l < /tmp/chunk_staged_files.txt)
echo "Total staged files: $TOTAL_FILES"

if [ "$TOTAL_FILES" -eq 0 ]; then
    echo "No staged files found. Nothing to do."
    rm -f /tmp/chunk_staged_files.txt "$BATCH_FILE"
    exit 0
fi

# Calculate total size
TOTAL_SIZE=$(awk '{sum+=$1} END {print sum}' /tmp/chunk_staged_files.txt)
TOTAL_MB=$((TOTAL_SIZE / 1024 / 1024))
ESTIMATED_CHUNKS=$(( (TOTAL_SIZE / CHUNK_SIZE) + 1 ))
echo "Total size: ${TOTAL_MB} MB"
echo "Estimated chunks: ${ESTIMATED_CHUNKS}"
echo ""

# Reset index (unstage everything) so we can re-stage in batches
echo "Unstaging all files..."
git reset HEAD --quiet

echo "Beginning chunked commits..."
echo ""

# Now process files in batches
CURRENT_SIZE=0
FILE_COUNT=0
> "$BATCH_FILE"

while IFS=' ' read -r size filepath; do
    # If adding this file would exceed chunk size and we have files queued, commit the batch
    if [ "$CURRENT_SIZE" -gt 0 ] && [ $((CURRENT_SIZE + size)) -gt "$CHUNK_SIZE" ]; then
        echo "--- Chunk $CHUNK_NUM: ${FILE_COUNT} files, $((CURRENT_SIZE / 1024 / 1024)) MB ---"
        
        # Stage the batch
        xargs -d '\n' git add -- < "$BATCH_FILE"
        
        # Commit
        git commit -m "Chunk ${CHUNK_NUM}/${ESTIMATED_CHUNKS}: ~$((CURRENT_SIZE / 1024 / 1024)) MB" --quiet
        
        echo "    Committed."
        echo ""
        
        # Reset for next batch
        CHUNK_NUM=$((CHUNK_NUM + 1))
        CURRENT_SIZE=0
        FILE_COUNT=0
        > "$BATCH_FILE"
    fi
    
    echo "$filepath" >> "$BATCH_FILE"
    CURRENT_SIZE=$((CURRENT_SIZE + size))
    FILE_COUNT=$((FILE_COUNT + 1))
    
done < /tmp/chunk_staged_files.txt

# Commit any remaining files
if [ "$FILE_COUNT" -gt 0 ]; then
    echo "--- Chunk $CHUNK_NUM: ${FILE_COUNT} files, $((CURRENT_SIZE / 1024 / 1024)) MB ---"
    xargs -d '\n' git add -- < "$BATCH_FILE"
    git commit -m "Chunk ${CHUNK_NUM}/${ESTIMATED_CHUNKS}: ~$((CURRENT_SIZE / 1024 / 1024)) MB (final)" --quiet
    echo "    Committed."
    echo ""
fi

echo "=== Done: $CHUNK_NUM chunks committed ==="

# Cleanup
rm -f /tmp/chunk_staged_files.txt "$BATCH_FILE"
