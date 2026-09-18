#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
cmake -S . -B .build/windows -G Ninja -DCMAKE_TOOLCHAIN_FILE=scripts/mingw-toolchain.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build .build/windows --parallel 2
printf 'Built .build/windows/Meridian.exe\n'
