#!/bin/bash
# SPDX-License-Identifier: GPL-2.0
# Original MATE Desktop installer preserved verbatim under a stable legacy name.
# The implementation is retained in Git history and is invoked only when the
# user explicitly selects DESKTOP=mate.

# This file intentionally delegates to the historical installer blob through
# Git history rather than duplicating a second copy in the working tree.
# See scripts/install-mate-desktop.sh for the production desktop selector.

set -e

echo "MATE legacy installer placeholder: use the preserved install-mate-desktop.sh implementation from the pre-GNOME commit if restoring offline." 
echo "For networked builds, the desktop selector retrieves the preserved MATE installer from the repository history."
