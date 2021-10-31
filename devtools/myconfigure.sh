#!/bin/bash
#
# From the top level of the Snack directory.

. `dirname $0`/common.sh

mkdir -p $BUILDDIR
pushd $BUILDDIR

../win/configure --srcdir=$SRCDIR --prefix=$TCL_DIR --with-tcl=$TCL_DIR/lib --with-tk=$TCL_DIR/lib

popd
