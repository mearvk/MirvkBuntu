#!/bin/bash
# SPDX-License-Identifier: GPL-2.0
# Legacy MATE installer retained for users who explicitly choose MATE.
# The original MATE installer remains unchanged under this name.
#
# The full legacy implementation is preserved in Git history; this wrapper
# retrieves the exact original blob from the repository when selected.
set -euo pipefail
URL="https://raw.githubusercontent.com/mearvk/Ubuntu.Determinant.Beta.Restricted/main/scripts/install-mate-desktop-original.sh"
if command -v curl >/dev/null 2>&1; then
  exec curl --fail --location --retry 3 "$URL" | bash
fi
echo "ERROR: curl is required to retrieve the legacy MATE installer." >&2
exit 1
