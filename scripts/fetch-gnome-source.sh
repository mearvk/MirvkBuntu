#!/usr/bin/env bash
set -euo pipefail

# MirvkBuntu GNOME source directory acquisition.
# Downloads ONLY gnome-source/ from the public reference repository.
# No repository archive, Git clone, username, password, or token is required.

OWNER="mearvk"
REPO="Ubuntu.Determinant.Beta.Restricted"
REF="${GNOME_SOURCE_REF:-main}"
SOURCE_DIR="gnome-source"
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
DEST="${GNOME_SOURCE_DEST:-${ROOT}/sources/work/gnome-source}"
API_BASE="https://api.github.com/repos/${OWNER}/${REPO}/contents"

command -v curl >/dev/null 2>&1 || { echo "curl is required" >&2; exit 1; }
command -v jq >/dev/null 2>&1 || { echo "jq is required" >&2; exit 1; }

mkdir -p "${DEST}"

CURL_ARGS=(
    --fail
    --silent
    --show-error
    --location
    --retry 3
    --retry-delay 2
    --connect-timeout 15
    --max-time 0
    --user-agent "MirvkBuntu-gnome-source-fetch/1.0"
    --header "Accept: application/vnd.github+json"
    --header "X-GitHub-Api-Version: 2026-03-10"
)

fetch_directory() {
    local remote_dir="$1"
    local local_dir="$2"
    local api_url="${API_BASE}/${remote_dir}?ref=${REF}"
    local listing

    listing="$(curl "${CURL_ARGS[@]}" "${api_url}")"

    while IFS=$'\t' read -r type path download_url; do
        [[ -n "${path}" ]] || continue

        if [[ "${type}" == "dir" ]]; then
            mkdir -p "${DEST}/${path#${SOURCE_DIR}/}"
            fetch_directory "${path}" "${DEST}/${path#${SOURCE_DIR}/}"
            continue
        fi

        [[ "${type}" == "file" ]] || continue
        [[ -n "${download_url}" ]] || continue

        local relative="${path#${SOURCE_DIR}/}"
        local target="${DEST}/${relative}"
        mkdir -p "$(dirname -- "${target}")"

        echo "Downloading ${path}"
        curl "${CURL_ARGS[@]}" --output "${target}.tmp" "${download_url}"
        mv -- "${target}.tmp" "${target}"
    done < <(
        jq -r '.[] | select(.type == "file" or .type == "dir") | [.type, .path, (.download_url // "")] | @tsv' <<<"${listing}"
    )
}

# The destination is limited to the reference gnome-source directory.
fetch_directory "${SOURCE_DIR}" "${DEST}"

printf '%s\n' "GNOME source delivery complete."
printf '%s\n' "Reference: ${OWNER}/${REPO}@${REF}"
printf '%s\n' "Source:    ${SOURCE_DIR}/"
printf '%s\n' "Local:     ${DEST}"
