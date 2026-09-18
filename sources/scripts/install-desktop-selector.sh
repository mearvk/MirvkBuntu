#!/usr/bin/env bash
set -euo pipefail

# Ubuntu Determinant Desktop selector.
# Default is GNOME. Set DESKTOP=mate to retain the MATE desktop.
DESKTOP="${DESKTOP:-gnome}"
BASE_URL="https://raw.githubusercontent.com/mearvk/Ubuntu.Determinant.Beta.Restricted/main/scripts"

case "${DESKTOP,,}" in
  gnome) SCRIPT="install-gnome-desktop.sh" ;;
  mate)  SCRIPT="install-mate-desktop-legacy.sh" ;;
  *) echo "ERROR: Unknown DESKTOP='${DESKTOP}'. Use gnome or mate." >&2; exit 2 ;;
esac

if [ -x "/tmp/${SCRIPT}" ]; then
  exec "/tmp/${SCRIPT}"
fi

if ! command -v curl >/dev/null 2>&1; then
  echo "ERROR: curl is required to retrieve the ${DESKTOP} desktop installer." >&2
  exit 1
fi

curl --fail --location --retry 3 --output "/tmp/${SCRIPT}.download" "${BASE_URL}/${SCRIPT}"
chmod 755 "/tmp/${SCRIPT}.download"
exec "/tmp/${SCRIPT}.download"
