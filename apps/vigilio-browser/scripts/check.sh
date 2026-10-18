#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cmake -S "$ROOT" -B "$ROOT/_build" -G Ninja -DVIGILIO_BUILD_GUI=OFF -DCMAKE_BUILD_TYPE=Debug
cmake --build "$ROOT/_build"
ctest --test-dir "$ROOT/_build" --output-on-failure -L fast
if command -v clang-format >/dev/null 2>&1; then
    find "$ROOT/src" "$ROOT/tests" -name '*.cpp' -o -name '*.h' | while read -r file; do clang-format --dry-run --Werror "$file"; done
else
    echo 'clang-format NOT_RUN (not installed)'
fi
if command -v clang-tidy >/dev/null 2>&1; then
    echo 'clang-tidy available: run with compile_commands.json on supported compiler'
else
    echo 'clang-tidy NOT_RUN (not installed)'
fi
