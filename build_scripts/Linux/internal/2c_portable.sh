#!/bin/bash

shopt -s extglob

SCRIPTS_PATH="$(dirname "$(realpath "$0")")"/..
INSTALL_PATH=$SCRIPTS_PATH/../../install
QT_DIR=""

#checking for parameters
for i in "$@"
do
case $i in
    -i=*|--install_path=*)
        INSTALL_PATH="${i#*=}"
        shift # past argument=value
        ;;
    -qt=*|--qt_dir=*)
        QT_DIR=${i#*=}
        shift # past argument=value
        ;;
    *)
        # unknown option
        ;;
esac
done

if [ ! -z "$QT_DIR" ]
then
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$QT_DIR/lib
    export QMAKE=$QT_DIR/bin/qmake
fi

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$INSTALL_PATH/usr/lib

if $SCRIPTS_PATH/resources/linuxdeploy --appdir=$INSTALL_PATH --plugin qt; then
    echo "$INSTALL_PATH is now a self contained relight application"
else
    echo "linuxdeploy failed with error code $?. Script was not completed successfully."
    exit 1
fi
