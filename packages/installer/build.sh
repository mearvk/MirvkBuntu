#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT"

if ! command -v java >/dev/null 2>&1; then
  echo "Java 21+ is required." >&2
  exit 2
fi

if ! command -v mvn >/dev/null 2>&1; then
  echo "Maven is required to build the JavaFX installer." >&2
  exit 2
fi

java -version
mvn -B clean package
mvn -B javafx:run
