#!/bin/bash
# This is a script shell for compiling and deploying ReLight in a Linux environment.
#
# Requires a Qt environment which is set-up properly, and an accessible
# cmake binary.
#
# Without given arguments, RelightLab will be built in the relightlab/build,
# the folder relightlab/install will be a portable version of RelightLab and
# the AppImage will be placed in the relightlab folder.
#
# You can give as argument the build path, the install path (that will contain
# the portable version of RelightLab), and the number of cores to use to build RelightLab
# (default: 4).
# The AppImage will be placed in the parent directory of the install path.
#
# Example of call:
# bash make_it.sh --build_path=path/to/build --install_path=path/to/install -j8

SCRIPTS_PATH="$(dirname "$(realpath "$0")")"
SOURCE_PATH=$SCRIPTS_PATH/../..
BUILD_PATH=$SOURCE_PATH/build
INSTALL_PATH=$SOURCE_PATH/install
CORES="-j4"
QT_DIR_OPTION=""

#check parameters
for i in "$@"
do
case $i in
    -b=*|--build_path=*)
        BUILD_PATH="${i#*=}"
        shift # past argument=value
        ;;
    -i=*|--install_path=*)
        INSTALL_PATH="${i#*=}"/usr/
        shift # past argument=value
        ;;
    -j*)
        CORES=$i
        shift # past argument=value
        ;;
    -qt=*|--qt_dir=*)
        QT_DIR_OPTION=-qt=${i#*=}
        shift # past argument=value
        ;;
    *)
        # unknown option
        ;;
esac
done

bash $SCRIPTS_PATH/1_build.sh -b=$BUILD_PATH -i=$INSTALL_PATH $QT_DIR_OPTION $CORES
bash $SCRIPTS_PATH/2_deploy.sh -i=$INSTALL_PATH $QT_DIR_OPTION
