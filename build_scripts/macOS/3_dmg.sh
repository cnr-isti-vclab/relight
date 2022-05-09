#!/bin/bash
# this is a script shell for setting up the application DMG for MacOS.
# Requires a properly built and deployed relight (requires to run the
# 2_deploy.sh script first).
#
# Without given arguments, relight.app will be looked for in relight/install
# folder. ReLight DMG will be placed in the same directory of relight.app.
#
# You can give as argument the INSTALL_PATH containing relight.app.

SCRIPTS_PATH=$(cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd)
INSTALL_PATH=$SCRIPTS_PATH/../../install

#checking for parameters
for i in "$@"
do
case $i in
    -i=*|--install_path=*)
        INSTALL_PATH="${i#*=}"
        shift # past argument=value
        ;;
    *)
        # unknown option
        ;;
esac
done

#get version
RL_VERSION=$(cat $SCRIPTS_PATH/../../RELIGHT_VERSION)

SOURCE_PATH=$SCRIPTS_PATH/../../

# final step create the dmg using appdmg
# appdmg is installed with 'npm install -g appdmg'",
sed "s%INST_PATH%$INSTALL_PATH%g" $SCRIPTS_PATH/resources/dmg_latest.json > $SCRIPTS_PATH/resources/dmg_final.json
sed -i '' "s%RL_VERSION%$RL_VERSION%g" $SCRIPTS_PATH/resources/dmg_final.json
sed -i '' "s%SOURCE_PATH%$SOURCE_PATH%g" $SCRIPTS_PATH/resources/dmg_final.json

mv $INSTALL_PATH/relight.app $INSTALL_PATH/ReLight$RL_VERSION.app

echo "Running appdmg"
appdmg $SCRIPTS_PATH/resources/dmg_final.json $INSTALL_PATH/ReLight$RL_VERSION-macos.dmg

rm $SCRIPTS_PATH/resources/dmg_final.json

#at this point, distrib folder contains a DMG ReLight file
echo "distrib folder now contains a DMG file"
