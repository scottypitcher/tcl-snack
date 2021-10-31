#!/bin/bash
#
# From the top level of the Snack directory.

. `dirname $0`/common.sh

pushd $BUILDDIR

make install

popd
