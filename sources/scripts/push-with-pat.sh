#!/usr/bin/env bash
set -euo pipefail

REPO="mearvk/Ubuntu.Determinant.Beta.Restricted"
REMOTE_URL="https://github.com/${REPO}.git"

printf 'GitHub username: '
read -r GITHUB_USER
printf 'GitHub PAT (input hidden): '
read -rs GITHUB_PAT
printf '\n'

if [[ -z "$GITHUB_USER" || -z "$GITHUB_PAT" ]]; then
  echo 'Username and PAT are required.' >&2
  exit 1
fi

# Keep the PAT out of the remote URL, Git config, shell history, and disk.
export GITHUB_USER GITHUB_PAT
export GIT_TERMINAL_PROMPT=0

if git remote get-url origin >/dev/null 2>&1; then
  echo 'Using existing origin remote.'
else
  git remote add origin "$REMOTE_URL"
fi

ORIGIN_URL="$(git remote get-url origin)"
case "$ORIGIN_URL" in
  https://github.com/mearvk/Ubuntu.Determinant.Beta.Restricted.git|https://github.com/mearvk/Ubuntu.Determinant.Beta.Restricted)
    ;;
  *)
    echo "origin does not point to $REMOTE_URL" >&2
    exit 1
    ;;
esac

echo 'Pushing current branch with PAT authentication...'

# Git credential protocol helper. GitHub uses the PAT as the HTTPS password.
# This helper is configured only for this Git invocation.
CREDENTIAL_HELPER='!f() { printf "protocol=https\\nhost=github.com\\npath=mearvk/Ubuntu.Determinant.Beta.Restricted.git\\nusername=%s\\npassword=%s\\n\\n" "$GITHUB_USER" "$GITHUB_PAT"; }; f'

git -c credential.helper="$CREDENTIAL_HELPER" \
    -c credential.useHttpPath=true \
    push origin HEAD

unset GITHUB_PAT GITHUB_USER

echo 'Push completed. The PAT was not written to Git configuration.'
