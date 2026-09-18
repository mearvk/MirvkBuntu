#!/usr/bin/env bash
set -euo pipefail

SCRIPT="$(cd "$(dirname "$0")" && pwd)/determinant-expand-path.sh"

assert_eq() {
  local expected="$1" actual="$2" label="$3"
  if [ "$actual" != "$expected" ]; then
    echo "FAIL: $label" >&2
    echo "  expected: $expected" >&2
    echo "  actual:   $actual" >&2
    exit 1
  fi
}

assert_eq '..' "$(bash "$SCRIPT" '..')" 'one parent remains ..'
assert_eq '../..' "$(bash "$SCRIPT" '...')" 'triple dot means two parents'
assert_eq '../../..' "$(bash "$SCRIPT" '....')" 'quadruple dot means three parents'
assert_eq 'alpha/../../beta' "$(bash "$SCRIPT" 'alpha/.../beta')" 'triple-dot component expands'
assert_eq 'alpha/../../../beta' "$(bash "$SCRIPT" 'alpha/..../beta')" 'quadruple-dot component expands'
assert_eq 'alpha...beta' "$(bash "$SCRIPT" 'alpha...beta')" 'dots inside filename are untouched'

echo 'PASS: Determinant extended parent-path policy'
