#!/usr/bin/env bash
# Run clang-format in a container so the host does not need LLVM installed.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

docker run --rm -v "$ROOT":/workspace -w /workspace \
  silkeh/clang:18-bookworm \
  bash -lc '
    set -e
    files=$(find firmware/src firmware/include firmware/lib -type f \( -name "*.cpp" -o -name "*.h" \) 2>/dev/null || true)
    if [ -z "$files" ]; then echo "no C++ files"; exit 0; fi
    clang-format --dry-run --Werror $files
  '
