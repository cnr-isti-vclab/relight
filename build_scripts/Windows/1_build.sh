#!/bin/bash
# this is a script shell for compiling relight in a windows environment.
# Requires a VS >= 2017 and Qt environments which are set-up properly,
# and an accessible cmake binary.
#
# Without given arguments, relight will be built in the relightlab/build
# directory, and installed in $BUILD_PATH/../install.
#
# You can give as argument the BUILD_PATH and the INSTALL_PATH in the
# following way:
# sh 1_build.sh --build_path=/path/to/build --install_path=/path/to/install
# -b and -i arguments are also supported.

#default paths wrt the script folder
SCRIPTS_PATH="$(dirname "$(realpath "$0")")"
SOURCE_PATH=$SCRIPTS_PATH/../..
BUILD_PATH=$SOURCE_PATH/build
INSTALL_PATH=$SOURCE_PATH/install
QT_DIR=""
CCACHE=""

#check parameters
for i in "$@"
do
case $i in
    -b=*|--build_path=*)
        BUILD_PATH="${i#*=}"
        shift # past argument=value
        ;;
    -i=*|--install_path=*)
        INSTALL_PATH="${i#*=}"
        shift # past argument=value
        ;;
    -qt=*|--qt_dir=*)
        QT_DIR=${i#*=}
        shift # past argument=value
        ;;
    --ccache)
        CCACHE="-DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache"
        shift # past argument=value
        ;;
    *)
        # unknown option
        ;;
esac
done

#create build path if necessary
if ! [ -d $BUILD_PATH ]
then
    mkdir -p $BUILD_PATH
fi

#create install path if necessary
if ! [ -d $INSTALL_PATH ]
then
    mkdir -p $INSTALL_PATH
fi

BUILD_PATH=$(realpath $BUILD_PATH)
INSTALL_PATH=$(realpath $INSTALL_PATH)

if [ ! -z "$QT_DIR" ]
then
    export Qt5_DIR=$QT_DIR
fi

# Set vcpkg toolchain file if it exists
VCPKG_TOOLCHAIN=""
if [ -f "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" ]; then
    VCPKG_TOOLCHAIN="-DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
elif [ ! -z "$CMAKE_TOOLCHAIN_FILE" ]; then
    VCPKG_TOOLCHAIN="-DCMAKE_TOOLCHAIN_FILE=$CMAKE_TOOLCHAIN_FILE"
fi

cd $BUILD_PATH
export NINJA_STATUS="[%p (%f/%t) ] "
cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$INSTALL_PATH $CCACHE $VCPKG_TOOLCHAIN $SOURCE_PATH
ninja
ninja install
