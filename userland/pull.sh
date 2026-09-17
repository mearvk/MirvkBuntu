#!/bin/bash

# Define the remote URL (ensure it ends with a trailing slash)
REMOTE_URL="https://github.com/mearvk/Ubuntu.Determinant.Beta.Restricted/tree/main/userland"

# Run wget with the correct recursive, reject, and structure flags
wget --recursive \
     --no-parent \
     --no-host-directories \
     --cut-dirs=0 \
     --no-clobber \
     --reject "index.html*" \
     "$REMOTE_URL"

