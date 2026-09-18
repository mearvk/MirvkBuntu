#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd -- "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$SCRIPT_DIR/build/classes"
SOURCE_DIR="$SCRIPT_DIR/src/main/java"
RESOURCE_DIR="$SCRIPT_DIR/src/main/resources"

JAVA_BIN="${JAVA_BIN:-java}"
JAVAC_BIN="${JAVAC_BIN:-javac}"

if ! command -v "$JAVA_BIN" >/dev/null 2>&1; then
  echo "ERROR: Java runtime not found: $JAVA_BIN" >&2
  exit 1
fi
if ! command -v "$JAVAC_BIN" >/dev/null 2>&1; then
  echo "ERROR: javac not found: $JAVAC_BIN" >&2
  exit 1
fi

# No Gradle is required. Set JAVAFX_SDK to the JavaFX SDK/lib directory,
# or point it at a directory containing the JavaFX module JARs.
if [[ -n "${JAVAFX_SDK:-}" ]]; then
  JAVAFX_LIB="$JAVAFX_SDK/lib"
  [[ -d "$JAVAFX_LIB" ]] || JAVAFX_LIB="$JAVAFX_SDK"
else
  JAVAFX_LIB=""
  for candidate in \
    "$PROJECT_ROOT/javafx-sdk/lib" \
    "$PROJECT_ROOT/lib/javafx" \
    "/usr/share/openjfx/lib" \
    "/usr/share/java/openjfx"; do
    if [[ -d "$candidate" ]] && compgen -G "$candidate/*.jar" >/dev/null; then
      JAVAFX_LIB="$candidate"
      break
    fi
  done
fi

if [[ -z "$JAVAFX_LIB" || ! -d "$JAVAFX_LIB" ]]; then
  echo "ERROR: JavaFX SDK/lib directory not found." >&2
  echo "Set JAVAFX_SDK to your JavaFX SDK directory." >&2
  echo "Example: JAVAFX_SDK=/opt/javafx-sdk-25 ./launch-desktop.sh" >&2
  exit 1
fi

JAVAFX_CP="$(printf '%s:' "$JAVAFX_LIB"/*.jar)"
JAVAFX_CP="${JAVAFX_CP%:}"

rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

mapfile -t SOURCES < <(find "$SOURCE_DIR" -type f -name '*.java' -print | sort)
if (( ${#SOURCES[@]} == 0 )); then
  echo "ERROR: No Java sources found in $SOURCE_DIR" >&2
  exit 1
fi

echo "Ubuntu White Desktop — direct JDK launcher"
echo "Java: $($JAVA_BIN -version 2>&1 | head -n 1)"
echo "JavaFX: $JAVAFX_LIB"
echo "Compiling ${#SOURCES[@]} source file(s)..."

"$JAVAC_BIN" \
  --module-path "$JAVAFX_LIB" \
  --add-modules javafx.controls,javafx.graphics,javafx.base \
  -cp "$JAVAFX_CP" \
  -d "$BUILD_DIR" \
  "${SOURCES[@]}"

if [[ -d "$RESOURCE_DIR" ]]; then
  cp -a "$RESOURCE_DIR"/. "$BUILD_DIR"/
fi

cd "$PROJECT_ROOT"
exec "$JAVA_BIN" \
  --enable-native-access=ALL-UNNAMED \
  --module-path "$JAVAFX_LIB" \
  --add-modules javafx.controls,javafx.graphics,javafx.base \
  -cp "$BUILD_DIR" \
  org.ubuntu.white.desktop.DesktopSynthesizer
