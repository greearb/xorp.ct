#! /bin/sh

# Conditionally set ${srcdir} if it wasn't assigned (e.g., by `gmake check`)
if [ "X${srcdir}" = "X" ] ; then srcdir=`dirname $0` ; fi

(exec ${srcdir}/FlowerCheck ./test_flower_malloc)
TEST_RESULT=$?
rm flower_report* > /dev/null 2>&1
exit $TEST_RESULT
