#!/bin/sh -e

CONFIG="$(dirname $0)/config"
. $CONFIG

cd $ROOTDIR

remote_run()
{
    host=$1
    shift
    ssh -n $host "$*"
}

if [ $# -eq 0 -o $# -gt 2 ] ; then
    echo "usage: $0 <host> [<workdir>]" >/dev/stderr
    echo "executed: $0 $*" >/dev/stderr
    exit 1
fi

HOST=$1
if [ $# -eq 1 ] ; then
    DESTDIR="."
else
    DESTDIR=$2
fi

remote_run $HOST rm -r "${DESTDIR}/scripts"
remote_run $HOST rm -r "${DESTDIR}/tmp"
remote_run $HOST rm -r "${DESTDIR}/xorp"
remote_run $HOST rm -r "${DESTDIR}/data"

# We make tmp as it forces DESTDIR to be created if it doesn't exist and
# doesn't cause an error if it already does since previous command deletes tmp

remote_run $HOST mkdir -p ${DESTDIR}/tmp

scp -pr xorp $1:${DESTDIR} && scp -pr scripts $1:${DESTDIR}
if [ $? -ne 0 ] ; then
    exit 1
fi
