#!/bin/bash
#
# From the top level of the Snack directory.

. `dirname $0`/common.sh

mkdir -p $BUILDDIR
pushd $BUILDDIR

CFLAGS=$SNACK_CFLAGS
export CFLAGS
configure="../$BUILD_PLATFORM/configure $CONFIGURE"
if [ -n "$SRCDIR" ]; then
    configure="$configure --srcdir=$SRCDIR"
fi
echo "$configure"
$configure

popd
