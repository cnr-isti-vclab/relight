#!/bin/bash

set -euo pipefail

SCRIPTS_PATH="$(dirname "$(realpath "$0")")"/..
RESOURCES_PATH="$SCRIPTS_PATH/resources"
INSTALL_PATH="$SCRIPTS_PATH/../../install"
PACKAGES_PATH="$SCRIPTS_PATH/../../packages"
ROOT_PATH="$SCRIPTS_PATH/../../.."

while [ $# -gt 0 ]; do
    case $1 in
        -i=*|--install_path=*)
            INSTALL_PATH="${1#*=}"
            ;;
        -p=*|--packages_path=*)
            PACKAGES_PATH="${1#*=}"
            ;;
    esac
    shift || true
done

mkdir -p "$PACKAGES_PATH"

RELIGHT_VERSION=$(cat "$ROOT_PATH/RELIGHT_VERSION")

# Ensure licensing info ships inside installer bundle
cp "$ROOT_PATH/LICENSE" "$INSTALL_PATH/LICENSE.txt"

# Prepare NSIS script
sed "s%RELIGHT_VERSION%$RELIGHT_VERSION%g" "$RESOURCES_PATH/relightlab.nsi" > "$INSTALL_PATH/relightlab_final.nsi"
sed -i "s%DISTRIB_PATH%.%g" "$INSTALL_PATH/relightlab_final.nsi"

# Build installer
makensis.exe "$INSTALL_PATH/relightlab_final.nsi"

rm "$INSTALL_PATH/relightlab_final.nsi"

INSTALLER_SRC=$(find "$INSTALL_PATH" -maxdepth 1 -name 'RelightLab*-windows.exe' | head -n 1)
if [ -z "$INSTALLER_SRC" ]; then
    echo "Failed to locate generated installer"
    exit 1
fi

mv "$INSTALLER_SRC" "$PACKAGES_PATH/"




