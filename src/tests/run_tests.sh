#!/usr/bin/env bash
# Build and run the XTEngine unit tests (Linux/macOS).
# Usage: src/tests/run_tests.sh
set -e
cd "$(dirname "$0")"
BUILD_DIR=build
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
qmake ../XTEngineTests.pro
make -j"$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)"
./XTEngineTests
