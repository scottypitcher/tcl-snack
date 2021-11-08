#!/bin/bash
#
# From the top level of the snack build directory.
#

# Change to '1' for a debug build.
DEBUG=0

TCLVERSION=8.6.9

# ---Windows only configuration.
# Where to install.
WIN_BUILD_PREFIX=/c/Tcl$TCLVERSION
WIN_BUILD_PREFIX_APPEND_ARCH=yes
WIN_BUILD_PREFIX_APPEND_DEBUG=yes

# ---Linux configuration.
UNIX_BUILD_SIZE=32
# Where to install.
UNIX_BUILD_PREFIX=~/Tcl$TCLVERSION
UNIX_BUILD_PREFIX_APPEND_ARCH=yes
UNIX_BUILD_PREFIX_APPEND_DEBUG=yes

# --- Don't modify below here.

# Determine the build type.
case "`uname -s`" in
    MINGW32*)
	BUILD_PLATFORM=win
	BUILD_SIZE=32
	BUILD_ARCH=intel
	EXE_SUFFIX=.exe
    ;;
    MINGW64*)
	BUILD_PLATFORM=win
	BUILD_SIZE=64
	BUILD_ARCH=amd64
	EXE_SUFFIX=.exe
    ;;
    Linux)
	BUILD_PLATFORM=unix
	BUILD_SIZE=$UNIX_BUILD_SIZE
	if [ "$BUILD_SIZE" == "64" ] ; then
	    BUILD_ARCH=x86_64
	    SNACK_CFLAGS=-m64
	else
	    BUILD_ARCH=amd
	    SNACK_CFLAGS=-m32
	fi
	TKCFLAGS=$TCLCFLAGS
    ;;
esac

# Debugging builds ...
if [ $DEBUG -eq 1 ] ; then
    echo "(This is a DEBUG build)"
    SNACK_CFLAGS="$SNACK_CFLAGS -g"
    DEBUG_SUFFIX=-Debug
    EXE_DEBUG_SUFFIX=g
fi

# Snack specific.
SNACK_PATCHLEVEL=`$BUILD_PLATFORM/sver.sh generic/snack.h --patchlevel`

# Where we build files. ABS_BUILDDIR is used by mybinzip.
BUILDDIR=build-$BUILD_ARCH$DEBUG_SUFFIX
ABS_BUILDDIR=`readlink -f $BUILDDIR`

# And where we find the source files, relative to the build directory.
SRCDIR=../$BUILD_PLATFORM

# The prefix is different under windows and linux.
case "$BUILD_PLATFORM" in
    win)
	BUILD_PREFIX=$WIN_BUILD_PREFIX
	if [ "$WIN_BUILD_PREFIX_APPEND_ARCH" = "yes" ]; then
	    BUILD_PREFIX=$BUILD_PREFIX-$BUILD_ARCH
	fi
	if [ "$WIN_BUILD_PREFIX_APPEND_DEBUG" = "yes" ]; then
	    BUILD_PREFIX=$BUILD_PREFIX$DEBUG_SUFFIX
	fi
	;;
    unix)
	BUILD_PREFIX=$UNIX_BUILD_PREFIX
	if [ "$UNIX_BUILD_PREFIX_APPEND_ARCH" = "yes" ]; then
	    BUILD_PREFIX=$BUILD_PREFIX-$BUILD_ARCH
	fi
	if [ "$UNIX_BUILD_PREFIX_APPEND_DEBUG" = "yes" ]; then
	    BUILD_PREFIX=$BUILD_PREFIX$DEBUG_SUFFIX
	fi
	;;
    *)
	echo "Unknown BUILD_PLATFORM: $BUILD_PLATFORM"
	exit 1
	;;
esac

# Used by mytests.sh.
TCL_BIN_DIR=$BUILD_PREFIX/bin
TCLSH_BIN=$TCL_BIN_DIR/tclsh8.6
if [[ ! -x "$TCLSH_BIN" ]]; then
    TCLSH_BIN=tclsh
fi

# setup configure.
CONFIGURE="--prefix=$BUILD_PREFIX"

# Used by mybinzip.sh
TCL_LIB_DIR=$BUILD_PREFIX/lib
SNACK_LIB_DIR=$TCL_LIB_DIR/snack$SNACK_PATCHLEVEL
