#!/bin/bash
#
# From the top level of the Snack directory.

. `dirname $0`/common.sh

echo "Working in: $TCL_LIB_DIR"


case "$BUILD_PLATFORM" in
    win)
	pushd $TCL_LIB_DIR
	zip -ur $ABS_BUILDDIR/tcl-snack-$SNACK_PATCHLEVEL-$BUILD_ARCH$DEBUG_SUFFIX.zip `basename $SNACK_LIB_DIR`
	popd
	;;
    unix)
	tar -czvf $ABS_BUILDDIR/tcl-snack-$SNACK_PATCHLEVEL-$BUILD_ARCH$DEBUG_SUFFIX.tar.gz -C $TCL_LIB_DIR `basename $SNACK_LIB_DIR`
	;;
    *)
	echo "Unknown BUILD_PLATFORM: $BUILD_PLATFORM"
	exit 1
	;;
esac

