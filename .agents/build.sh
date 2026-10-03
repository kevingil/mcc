#!/usr/bin/env bash
# Configure and compile MC.C. CMake downloads raylib 5.0 on the first run.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

cmake -S . -B build
cmake --build build --parallel "$(nproc)"

test -x "$ROOT/build/mcc/mcc"
echo "built $ROOT/build/mcc/mcc"
