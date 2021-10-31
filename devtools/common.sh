#!/bin/bash
#
# From the top level of the Snack directory.
#

# Change this to match the c:/Tcl..... directory name where your TCL is installed.
TCLVERSION=8.6.10

# Change to '1' for a debug build.
DEBUG=1

# Linux only.
# BUILD_SIZE=32


# --- Don't modify below here.

# Determine the build type.
case "`uname -s`" in
    MINGW32*)
	PLATFORM=win
	BUILD_SIZE=32
	BUILD_ARCH=intel
	EXE_SUFFIX=.exe
    ;;
    MINGW64*)
	PLATFORM=win
	BUILD_SIZE=64
	BUILD_ARCH=amd64
	EXE_SUFFIX=.exe
    ;;
    Linux)
	PLATFORM=unix
	BUILD_SIZE=$LINUX_BUILD_SIZE
	if [ "$BUILD_SIZE" == "64" ] ; then
	    BUILD_ARCH=x86_64
	else
	    BUILD_ARCH=amd
	fi
    ;;
esac

# Debugging builds ...
if [ $DEBUG -eq 1 ] ; then
    echo "(This is a DEBUG build)"
    SNACK_CFLAGS=-g
    DEBUG_SUFFIX=-Debug
    EXE_DEBUG_SUFFIX=g
fi

# Snack specific.
SNACK_PATCHLEVEL=`$PLATFORM/sver.sh generic/snack.h --patchlevel`

# Determine directory and file names.
TCL_DIR=/c/Tcl$TCLVERSION-$BUILD_ARCH$DEBUG_SUFFIX
TCL_BIN_DIR=$TCL_DIR/bin
TCL_LIB_DIR=$TCL_DIR/lib
SNACK_LIB_DIR=$TCL_LIB_DIR/snack$SNACK_PATCHLEVEL
TCLSH_BIN=$TCL_BIN_DIR/tclsh86${EXE_DEBUG_SUFFIX}$EXE_SUFFIX
BUILDDIR=build-$BUILD_ARCH$DEBUG_SUFFIX
ABS_BUILDDIR=`readlink -f $BUILDDIR`
SRCDIR=../$PLATFORM

echo "SNACK_PATCHLEVEL=$SNACK_PATCHLEVEL"
echo "SNACK_LIB_DIR=$SNACK_LIB_DIR"
