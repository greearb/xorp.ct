#!/bin/sh -e

CONFIG="$(dirname $0)/config"
. $CONFIG

cd ${ROOTDIR}

remote_run()
{
    host="$1"
    shift
    ssh_flags="$1"
    shift
    ssh $ssh_flags -n $host "$*"
}

if [ $# -ne 3 ] ; then
    echo "usage: $0 <host> <workdir> <sshflags>" >/dev/stderr
    echo "executed: $0 $*" >/dev/stderr
    exit 1
fi

HOST="$1"
DESTDIR="$2"
SSH_FLAGS="$3"

remote_run "$HOST" "$SSH_FLAGS" rm -rf "${DESTDIR}/scripts"
remote_run "$HOST" "$SSH_FLAGS" rm -rf "${DESTDIR}/tmp"
remote_run "$HOST" "$SSH_FLAGS" rm -rf "${DESTDIR}/xorp"
remote_run "$HOST" "$SSH_FLAGS" rm -rf "${DESTDIR}/data"

# We make tmp as it forces DESTDIR to be created if it doesn't exist and
# doesn't cause an error if it already does since previous command deletes
# tmp.

remote_run "$HOST" "$SSH_FLAGS" mkdir -p "${DESTDIR}/tmp"

for i in xorp scripts data ; do
    tar cfp - ${i} | ssh ${SSH_FLAGS} $HOST cd ${DESTDIR} \&\& tar xfp -
    if [ $? -ne 0 ] ; then
	exit 1
    fi
done
