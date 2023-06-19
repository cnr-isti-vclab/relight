#!/bin/bash
# this is a script shell sets up an ubuntu (18.04, 20.04 and 22.04) environment where
# Relight can be compiled.
#
# Run this script if you never installed any of the Relight dependencies.

INSTALL_QT=false

#checking for parameters
for i in "$@"
do
case $i in
    --install_qt)
        INSTALL_QT=true
        shift # past argument=value
        ;;
    *)
        # unknown option
        ;;
esac
done

sudo apt-get update
echo "=== installing mesa packages..."
sudo apt-get install -y mesa-common-dev libglu1-mesa-dev libjpeg-dev libeigen3-dev

if [ "$INSTALL_QT" = true ] ; then
    echo "=== installing qt packages..."
    sudo apt-get install -y qt5-default qttools5-dev-tools qtdeclarative5-dev
else
    echo "=== jumping installation of qt packages..."
fi
