#!/usr/bin/env bash
set -euo pipefail

echo "Cleaning previous build..."
make clean >/dev/null 2>&1 || true

echo "Running build with DEBUG=1 and CC='ccache gcc -Wmissing-prototypes '..."
BUILD_LOG="/tmp/yq2_missing_prototypes.log"

# Run make and capture stdout/stderr
set +e
DEBUG=1 CC="ccache gcc -Wmissing-prototypes " make -j$(nproc 2>/dev/null || echo 4) > "$BUILD_LOG" 2>&1
MAKE_EXIT=$?
set -e

echo "Build finished with exit code $MAKE_EXIT."
echo "Extracting missing prototype and unused function warnings (ignoring missionpacks/)..."

grep -E "warning: no previous prototype for|warning: .* defined but not used" "$BUILD_LOG" | grep -v "missionpacks/" || true

echo "Full build log saved to $BUILD_LOG"
