#!/bin/bash

SCRIPTS_PATH="$(dirname "$(realpath "$0")")"/..
SOURCE_PATH=$SCRIPTS_PATH/../..
RESOURCES_PATH=$SCRIPTS_PATH/../../resources
INSTALL_PATH=$SCRIPTS_PATH/../../install
PACKAGES_PATH=$SCRIPTS_PATH/../../packages

#checking for parameters
for i in "$@"
do
case $i in
    -i=*|--install_path=*)
        INSTALL_PATH="${i#*=}"
        shift # past argument=value
        ;;
    -p=*|--packages_path=*)
        PACKAGES_PATH="${i#*=}"
        shift # past argument=value
        ;;
    *)
        # unknown option
        ;;
esac
done

#get version
RL_VERSION=$(cat $SCRIPTS_PATH/../../RELIGHT_VERSION)

# final step create the dmg using appdmg
# appdmg is installed with 'npm install -g appdmg'",
sed "s%INST_PATH%$INSTALL_PATH%g" $SCRIPTS_PATH/resources/dmg_latest.json > $SCRIPTS_PATH/resources/dmg_final.json
sed -i '' "s%RL_VERSION%$RL_VERSION%g" $SCRIPTS_PATH/resources/dmg_final.json
sed -i '' "s%SOURCE_PATH%$SOURCE_PATH%g" $SCRIPTS_PATH/resources/dmg_final.json

mv $INSTALL_PATH/relightlab.app $INSTALL_PATH/RelightLab$RL_VERSION.app

mkdir $PACKAGES_PATH

appdmg $SCRIPTS_PATH/resources/dmg_final.json $PACKAGES_PATH/RelightLab$RL_VERSION-macos.dmg

# Cleanup: sometimes appdmg/hdiutil leaves the temporary image mounted or
# the automatic detach may fail because the mountpoint is already gone.
# Try to find any /Volumes entries that start with RelightLab and detach
# them by device (safer), with retries. Do not fail the script if nothing
# is mounted.
for vol in /Volumes/RelightLab*; do
    if [ -d "$vol" ]; then
        # find the device associated with this mount
        device=$(hdiutil info | awk -v v="$vol" '$0 ~ v { getline; print $1 }' | head -n1)
        if [ -n "$device" ]; then
            # try a couple times to detach
            for attempt in 1 2 3; do
                if hdiutil detach "$device" >/dev/null 2>&1; then
                    break
                else
                    sleep 1
                fi
            done
            # if still mounted, try force detach but don't fail
            hdiutil detach -force "$device" >/dev/null 2>&1 || true
        fi
    fi
done

rm $SCRIPTS_PATH/resources/dmg_final.json
