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

mv $INSTALL_PATH/relight.app $INSTALL_PATH/ReLight$RL_VERSION.app

mkdir $PACKAGES_PATH

appdmg $SCRIPTS_PATH/resources/dmg_final.json $PACKAGES_PATH/ReLightLab$RL_VERSION-macos.dmg

rm $SCRIPTS_PATH/resources/dmg_final.json
