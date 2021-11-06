#/bin/bash

if test "$2" = "--patchlevel" ; then
    KEY=SNACK_PATCH_LEVEL
else
    KEY=SNACK_VERSION
fi
RESULT=`grep $KEY "$1"`
echo "RESULT=$RESULT"
THIS_ONE=0
for i in $RESULT; do
    if test "$THIS_ONE" = "1" ; then
	echo "THIS ONE is $i"
	LEN=`expr length "$i"`
	echo "LEN is $LEN"
	END=`expr $LEN - 3`
	echo "END is $END"
	VERSION=`expr substr "$i" 2 $END`
    fi
    if test "$i" = "$KEY"; then
	THIS_ONE=1
    fi
done
echo "final VERSION is '$VERSION'"
echo $VERSION
