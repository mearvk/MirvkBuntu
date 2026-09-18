#!/usr/bin/env bash
set -euo pipefail

# Ubuntu Determinant Desktop selector.
# This file is retained as the compatibility entry point used by the ISO
# Makefile. The original MATE implementation remains pinned and untouched;
# GNOME is the default production path and receives the Ubuntu White overlay.
#
# Choices:
#   DESKTOP=gnome (default)
#   DESKTOP=mate
#
# GNOME visual choices are passed through unchanged:
#   GNOME_THEME=ubuntu-white (default) or stock
#   UBUNTU_WHITE_ICONS=1|0
#   UBUNTU_WHITE_CSS=1|0

DESKTOP="${DESKTOP:-gnome}"
BASE_URL="https://raw.githubusercontent.com/mearvk/Ubuntu.Determinant.Beta.Restricted/main/scripts"
MATE_REV="2fc57231c2f2d35b218e044b0004a4fa2c4d0ec4"

case "${DESKTOP,,}" in
  gnome)
    SCRIPT="install-gnome-desktop.sh"
    URL="${BASE_URL}/${SCRIPT}"
    ;;
  mate)
    SCRIPT="install-mate-desktop.sh"
    URL="https://raw.githubusercontent.com/mearvk/Ubuntu.Determinant.Beta.Restricted/${MATE_REV}/scripts/${SCRIPT}"
    ;;
  *)
    echo "ERROR: Unknown DESKTOP='${DESKTOP}'. Use gnome or mate." >&2
    exit 2
    ;;
esac

if [ "${DESKTOP,,}" = "gnome" ] && [ -x "/tmp/${SCRIPT}" ]; then
  exec "/tmp/${SCRIPT}"
fi

if ! command -v curl >/dev/null 2>&1; then
  echo "ERROR: curl is required to retrieve the ${DESKTOP} desktop installer." >&2
  exit 1
fi

curl --fail --location --retry 3 --output "/tmp/${SCRIPT}.download" "${URL}"
chmod 755 "/tmp/${SCRIPT}.download"
exec "/tmp/${SCRIPT}.download"
