#!/bin/bash
#
# Run the test suite. Please refer to the file name.

TCLVERSION=8.6.10

# Linux: either 32 or 64 bit.
LINUX_BUILD_SIZE=32

DEBUG=0

# ---
# Determine the build size.
case "`uname -s`" in
    MINGW32*)
	PLATFORM=win
	BUILD_SIZE=32
	BUILD_ARCH=intel
    ;;
    MINGW64*)
	PLATFORM=win
	BUILD_SIZE=64
	BUILD_ARCH=amd64
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

# If debugging, set this
if [ $DEBUG -eq 1 ] ; then
    echo "(This is a DEBUG build)"
    SYMBOLS=--enable-symbols=all
    EXE_SUFFIX=g
    DEBUG_SUFFIX=-Debug
else
    SYMBOLS=--disable-symbols
    EXE_SUFFIX=
fi


# The build folder
SUFFIX=-$BUILD_ARCH$DEBUG_SUFFIX

# The target directory
/c/Tcl$TCLVERSION$SUFFIX/bin/tclsh86$EXE_SUFFIX tests/all.tcl

