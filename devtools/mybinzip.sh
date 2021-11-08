#!/bin/bash
#
# From the top level of the Snack directory.

. `dirname $0`/common.sh

echo "Working in: $TCL_LIB_DIR"
pushd $TCL_LIB_DIR
zip -ur $ABS_BUILDDIR/tcl-snack-$SNACK_PATCHLEVEL-$BUILD_ARCH$DEBUG_SUFFIX.zip `basename $SNACK_LIB_DIR`
popd
