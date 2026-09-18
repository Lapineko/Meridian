#!/usr/bin/env bash
# Cross-compile in WSL; run on Windows for native USB access.
set -euo pipefail
exec bash "$(dirname "$0")/scripts/build-wsl.sh" "$@"
