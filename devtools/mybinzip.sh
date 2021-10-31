#!/bin/bash
#
# From the top level of the Snack directory.

. `dirname $0`/common.sh

# TCL_DIR=/c/Tcl$TCLVERSION-$BUILD_ARCH$DEBUG_SUFFIX
# TCL_BIN_DIR=$TCL_DIR/bin
# TCL_LIB_DIR=$TCL_DIR/lib
# SNACK_LIB_DIR=$TCL_LIB_DIR/snack$SNACK_PATCHLEVEL
# TCLSH_BIN=$TCL_BIN_DIR/tclsh86${EXE_DEBUG_SUFFIX}$EXE_SUFFIX
# BUILDDIR=build-$BUILD_ARCH$DEBUG_SUFFIX
# SRCDIR=../$PLATFORM
pushd $TCL_LIB_DIR
zip -ur $ABS_BUILDDIR/tcl-snack-$SNACK_PATCHLEVEL-$BUILD_ARCH$DEBUG_SUFFIX.zip `basename $SNACK_LIB_DIR`
popd
