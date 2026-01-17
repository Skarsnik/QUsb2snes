#!/bin/sh
#=SQPACKAGER 0.1
#This is a generated file by SQPackager
#Remove the =SQPACKAGER line for sqpackager to not regenerate this file
#set -o xtrace
PREFIX="/usr/local/"
COMPILE_PREFIX=""
PROJECT_BUILD_DIR="project_build_dir"
DO_BUILD=0
QMAKE_EXEC=qmake6
DO_INSTALL=0
SKIP_MAKE=0

## Arugment handling
if [ "$1" = "--help" ]; then
    echo "Arguments are [--build] [--install] [--qmake-cmd <qmake>] [--prefix install_prefix] QMAKE_ARGS"
    echo "Note that --build or --install must be used before --prefix"
fi

set_prefix=0
set_qmake=0
set_compile_prefix=0
for arg in "$@"
do
    if [ "$set_prefix" = 1 ]; then
        PREFIX=$arg
        set_prefix=0
        shift 1
        break
    fi
    if [ "$set_qmake" = 1 ]; then
        QMAKE_EXEC=$arg
        set_qmake=0
    fi
    if [ "$set_compile_prefix" = 1 ]; then
        COMPILE_PREFIX=$arg
        set_compile_prefix=0
    fi
    if [ "$arg" = "--prefix" ]; then
        set_prefix=1
    fi
    if [ "$arg" = "--build" ]; then
        DO_BUILD=1
    fi
    if [ "$arg" = "--install" ]; then
        DO_INSTALL=1
    fi
    if [ "$arg" = "--skip-make" ]; then
        SKIP_MAKE=1
    fi
    if [ "$arg" = "--qmake-cmd" ]; then
        set_qmake=1
    fi
    if [ "$arg" = "--compile-prefix" ]; then
        set_compile_prefix=1
    fi
    shift 1
done
if [ "$set_prefix" = 1 ]; then
    echo "--prefix with no argument"
    exit 1
fi
if [ "$PREFIX" = "--build" ]; then
    echo "--prefix missing a value"
    exit 1
fi
if [ "$PREFIX" = "--install" ]; then
    echo "--prefix missing a value"
    exit 1
fi

if [ $DO_BUILD = 0 -a $DO_INSTALL = 0 ]; then
    DO_BUILD=1
    DO_INSTALL=1
fi

PROJECT_TARGET=QUsb2Snes

INSTALL_PREFIX=$PREFIX
if [ "$COMPILE_PREFIX" = "" ]; then
    COMPILE_PREFIX=$INSTALL_PREFIX
fi

DESKTOP_FILE="QUsb2Snes.desktop"
NORMALIZED_DESKTOP_FILE_NAME="fr.nyo.QUsb2Snes.desktop"
APPLICATION_NAME="QUsb2Snes"
APPLICATION_SHARE="$INSTALL_PREFIX/share/$APPLICATION_NAME"
APPLICATION_COMPILE_SHARE="$COMPILE_PREFIX/share/$APPLICATION_NAME"
TRANSLATION_DIR="$APPLICATION_SHARE/i18n/"


if [ $DO_BUILD = 1 ]; then
    echo "Building project into : $PROJECT_BUILD_DIR"
    mkdir -v $PROJECT_BUILD_DIR
    cd $PROJECT_BUILD_DIR
    #echo DEFINES+="SQPROJECT_INSTALLED" DEFINES+="SQPROJECT_UNIX_INSTALL_PREFIX=$COMPILE_PREFIX" DEFINES+="SQPROJECT_UNIX_APP_SHARE=$APPLICATION_COMPILE_SHARE" $@
    $QMAKE_EXEC -makefile DEFINES+="SQPROJECT_INSTALLED" DEFINES+="SQPROJECT_UNIX_INSTALL_PREFIX=\\\\\\\"$COMPILE_PREFIX\\\\\\\"" DEFINES+="SQPROJECT_UNIX_APP_SHARE=\\\\\\\"$APPLICATION_COMPILE_SHARE\\\\\\\"" ../QUsb2snes.pro $@
    if [ $SKIP_MAKE = 0 ]; then
        make -j 4
    fi
    if [ $? != 0 ]; then
        exit 1
    fi
    cd -
fi

if [ $DO_INSTALL = 0 ]; then
    exit 0
fi
echo "Starting installation"

echo "Installing target binary"

install -v -D $PROJECT_BUILD_DIR/$PROJECT_TARGET $INSTALL_PREFIX/bin/$PROJECT_TARGET

echo "Installing .desktop file"

install -v -d $INSTALL_PREFIX/share/applications/
install -v -Dm644 $DESKTOP_FILE $INSTALL_PREFIX/share/applications/$NORMALIZED_DESKTOP_FILE_NAME
install -v -D "ui/icons/cheer128x128.png" $INSTALL_PREFIX/share/icons/hicolor/128x128/apps/fr.nyo.QUsb2Snes.png


echo "Installing Readme file"
install -v -D README.md $INSTALL_PREFIX/share/doc/$APPLICATION_NAME/README.md

if [ -e "$PROJECT_TARGET.manpage.1" ]; then
    install -v -d $INSTALL_PREFIX/share/man/man1/
    gzip -c "$PROJECT_TARGET.manpage.1" > $INSTALL_PREFIX/share/man/man1/"$PROJECT_TARGET.1.gz"
fi


