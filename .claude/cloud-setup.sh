#!/bin/bash
# Cloud environment setup script for KSP-UE5 (paste into the environment's "Setup script" field at claude.ai/code).
# Runs as root on Ubuntu 24.04 before Claude Code starts; must finish in < 5 min to be cached.
# GCC, Clang, cmake, ninja and Python 3 are pre-installed. This script only warms the kspcore build so the first
# gate run in a session is fast, and verifies the toolchain once.
set -euo pipefail
cd "$(git rev-parse --show-toplevel 2>/dev/null || echo "$PWD")"
if [ -f core/run_tests.sh ]; then
  chmod +x core/run_tests.sh
  # Build only (no tests) to populate build/linux-rel; the cache snapshot keeps it.
  ( cd core && ./run_tests.sh -b ) || echo "warm build failed; sessions will build on demand"
fi
echo "kspcore cloud setup done: $(clang++ --version | head -1); cmake $(cmake --version | head -1 | awk '{print $3}'); python $(python3 --version)"
