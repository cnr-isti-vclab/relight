#!/bin/bash
# this is a script shell for compiling relight in a MacOS environment.
# Requires a Qt environment which is set-up properly, and an accessible
# cmake binary.
#
# Without given arguments, relight will be built in the meshlab/src/build
# directory, and installed in $BUILD_PATH/../install.
#
# You can give as argument the BUILD_PATH and the INSTALL_PATH in the
# following way:
# bash 1_build.sh --build_path=/path/to/build --install_path=/path/to/install
# -b and -i arguments are also supported.

#default paths wrt the script folder
SCRIPTS_PATH=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
SOURCE_PATH=$SCRIPTS_PATH/../..
BUILD_PATH=$SOURCE_PATH/build
INSTALL_PATH=$SOURCE_PATH/install
CORES="-j4"
QT_DIR=""
USE_BREW_LLVM=false
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
    -j*)
        CORES=$i
        shift # past argument=value
        ;;
    -qt=*|--qt_dir=*)
        QT_DIR=${i#*=}
        shift # past argument=value
        ;;
    --use_brew_llvm)
        USE_BREW_LLVM=true
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

if [ ! -z "$QT_DIR" ]
then
    export Qt5_DIR=$QT_DIR
fi

if [ "$USE_BREW_LLVM" = true ] ; then
    export PATH="$(brew --prefix llvm)/bin:$PATH";
    export CC=/usr/local/opt/llvm/bin/clang
    export CXX=/usr/local/opt/llvm/bin/clang++
    export COMPILER=${CXX}
    export CFLAGS="-I /usr/local/include -I/usr/local/opt/llvm/include"
    export CXXFLAGS="-I /usr/local/include -I/usr/local/opt/llvm/include"
    export LDFLAGS="-L /usr/local/lib -L/usr/local/opt/llvm/lib"
fi

cd $BUILD_PATH
export NINJA_STATUS="[%p (%f/%t) ] "
cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$INSTALL_PATH $CCACHE $SOURCE_PATH
ninja
ninja install
