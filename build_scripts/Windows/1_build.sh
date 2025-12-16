#!/bin/bash

# Script that configures and builds RelightLab on Windows via CMake/Ninja.

set -euo pipefail

SCRIPTS_PATH="$(dirname "$(realpath "$0")")"
SOURCE_PATH="$SCRIPTS_PATH/../.."
BUILD_PATH="$SOURCE_PATH/build"
INSTALL_PATH="$SOURCE_PATH/install"

CCACHE_FLAGS=(-DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache)
CMAKE_TOOLCHAIN_ARGS=-DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake"

mkdir -p "$BUILD_PATH" "$INSTALL_PATH"

BUILD_PATH="$(realpath "$BUILD_PATH")"
INSTALL_PATH="$(realpath "$INSTALL_PATH")"

if [ -f "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" ]; then
    CMAKE_TOOLCHAIN_ARGS+=(-DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake)
elif [ -n "${CMAKE_TOOLCHAIN_FILE:-}" ]; then
    CMAKE_TOOLCHAIN_ARGS+=(-DCMAKE_TOOLCHAIN_FILE="$CMAKE_TOOLCHAIN_FILE")
fi

cd "$BUILD_PATH"
export NINJA_STATUS="[%p (%f/%t) ] "
cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$INSTALL_PATH" "${CCACHE_FLAGS[@]}" "${CMAKE_TOOLCHAIN_ARGS[@]}" "$SOURCE_PATH"
ninja
ninja install
