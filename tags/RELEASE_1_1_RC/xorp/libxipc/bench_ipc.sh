#!/usr/local/bin/bash

#
# $XORP$
#

#
# A script to perform IPC performance measurements
#

# Conditionally set ${srcdir} if it wasn't assigned (e.g., by `gmake check`)
if [ "X${srcdir}" = "X" ] ; then srcdir=`dirname $0` ; fi

BINDIR=.

${BINDIR}/xorp_finder &
FINDER_PID=$!

test_pf()
{
    local pfname=$1
    local pfenv=$2
    local pfcmd=$3
    local rawfile=${pfname}.log
    local i

    echo "-------------------------------------"
    echo "Collecting measurements for ${pfname}"
    echo "-------------------------------------"

    for i in 0 1 2 3 4 5 6 7 8 9 10 12 15 18 20 25; do
	XORP_PF=${pfenv} ${BINDIR}/test_xrl_sender ${pfcmd} -n $i
    done | tee ${rawfile}
    outfile=${pfname}.dat
    cat ${rawfile} | awk -f ${srcdir}/bench_ipc.awk > $outfile

    echo "Output:"
    echo "    Raw data file       = ${rawfile}"
    echo "    Processed data file = ${outfile}"
}

test_pf "inproc" "i" "-m 0 -r"
test_pf "udp" "u" "-m 0 -r"
test_pf "tcp" "t" "-m 0 -r"

kill ${FINDER_PID}
