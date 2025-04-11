#!/usr/bin/env bash
set -euo pipefail

BUILD_TYPE=$1

# Configure CMake
cmake -S . \
  --preset "${BUILD_TYPE}" -G Ninja \
  -D CT_TREAT_WARNINGS_AS_ERRORS=ON

# Build
cmake --build "build/${BUILD_TYPE}" -j
