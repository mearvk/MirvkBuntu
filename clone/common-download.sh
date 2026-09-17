#!/usr/bin/env bash
set -euo pipefail

SOURCE_REPO="${SOURCE_REPO:-mearvk/Ubuntu.Determinant.Beta.Restricted}"
SOURCE_REF="${SOURCE_REF:-main}"
DEST_ROOT="${DEST_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
API_ROOT="https://api.github.com/repos/${SOURCE_REPO}/contents"
RAW_ROOT="https://raw.githubusercontent.com/${SOURCE_REPO}/${SOURCE_REF}"

command -v wget >/dev/null 2>&1 || { echo "ERROR: wget is required." >&2; exit 127; }
command -v jq >/dev/null 2>&1 || { echo "ERROR: jq is required." >&2; exit 127; }

download_path() {
    local remote_path="$1"
    local local_path="${DEST_ROOT}/${remote_path}"

    [[ -f "${local_path}" ]] && return 0

    mkdir -p "$(dirname "${local_path}")"
    wget --no-clobber --no-verbose --show-progress \
        --header="Accept: application/vnd.github+json" \
        "${RAW_ROOT}/${remote_path}" -O "${local_path}"
}

walk_path() {
    local remote_path="$1"
    local payload
    payload="$(wget -qO- --header="Accept: application/vnd.github+json" "${API_ROOT}/${remote_path}?ref=${SOURCE_REF}")"

    jq -e 'type == "array"' >/dev/null <<<"${payload}" || {
        echo "ERROR: GitHub API did not return a directory listing for ${remote_path}" >&2
        return 1
    }

    while IFS= read -r item; do
        local item_path item_type
        item_path="$(jq -r '.path' <<<"${item}")"
        item_type="$(jq -r '.type' <<<"${item}")"

        case "${item_type}" in
            file|symlink) download_path "${item_path}" ;;
            dir) walk_path "${item_path}" ;;
        esac
    done < <(jq -c '.[]' <<<"${payload}")
}

[[ $# -eq 1 ]] || { echo "Usage: $0 <source-directory>" >&2; exit 2; }
SOURCE_DIRECTORY="$1"
mkdir -p "${DEST_ROOT}/${SOURCE_DIRECTORY}"
walk_path "${SOURCE_DIRECTORY}"
