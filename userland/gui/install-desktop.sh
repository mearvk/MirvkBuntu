#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
GUI="$ROOT/userland/gui"
DESKTOP_DIR="${XDG_DESKTOP_DIR:-$HOME/Desktop}"
mkdir -p "$DESKTOP_DIR"
ICON_DIR="$HOME/.local/share/icons/hicolor/scalable/apps"
mkdir -p "$ICON_DIR"
cp "$GUI/assets/ubuntu-determinant-userland.svg" "$ICON_DIR/ubuntu-determinant-userland.svg"

apps=(
  "Chromium|chromium"
  "Darling|darling"
  "DRM|drm"
  "IntelliJ Community|intellij-community"
  "Java Web Server|java-web-server"
  "Java|java"
  "JDesk|jdesk"
  "OpenJDK|openjdk"
  "Semeru OpenJDK 8|semeru-openjdk-8"
  "Wine|wine"
  "X11|x11"
)

for item in "${apps[@]}"; do
  IFS='|' read -r name slug <<< "$item"
  file="$DESKTOP_DIR/Ubuntu Determinant - $name.desktop"
  cat > "$file" <<EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=Ubuntu Determinant — $name
Comment=Ubuntu Determinant userland interface
Icon=ubuntu-determinant-userland
Exec=bash -lc 'cd "$ROOT" && "$GUI/bin/ubuntu-determinant-userland-gui"'
Terminal=false
Categories=System;Development;
StartupNotify=true
EOF
  chmod +x "$file"
done

echo "Installed ${#apps[@]} Ubuntu Determinant userland desktop icons in: $DESKTOP_DIR"
