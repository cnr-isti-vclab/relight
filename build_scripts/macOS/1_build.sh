#!/bin/bash

# Script that configures and builds RelightLab on macOS using CMake/Ninja.

set -euo pipefail

SCRIPTS_PATH="$(dirname "$(realpath "$0")")"
SOURCE_PATH="$SCRIPTS_PATH/../.."
BUILD_PATH="$SOURCE_PATH/build"
INSTALL_PATH="$SOURCE_PATH/install"

CCACHE_FLAGS=(-DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache)

mkdir -p "$BUILD_PATH" "$INSTALL_PATH"

BUILD_PATH="$(realpath "$BUILD_PATH")"
INSTALL_PATH="$(realpath "$INSTALL_PATH")"

OPENMP_PATH="$(brew --prefix libomp)"

cd "$BUILD_PATH"
export NINJA_STATUS="[%p (%f/%t) ] "
cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DOpenMP_ROOT="$OPENMP_PATH" -DCMAKE_INSTALL_PREFIX="$INSTALL_PATH" "${CCACHE_FLAGS[@]}" "$SOURCE_PATH"
ninja
ninja install
