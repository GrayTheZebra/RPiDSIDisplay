#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
build_dir="$(mktemp -d)"
trap 'rm -rf "$build_dir"' EXIT
c++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer \
  -I"$root/tests/stubs" -I"$root/src" \
  "$root/tests/graphics.cpp" "$root/src/RPiDSIDisplay.cpp" -o "$build_dir/graphics"
"$build_dir/graphics"
echo "Host graphics checks passed (not an Arduino/backend hardware test)."
