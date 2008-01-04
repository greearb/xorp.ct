#!/bin/sh -e

# Copyright (c) 2001-2008 International Computer Science Institute
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software")
# to deal in the Software without restriction, subject to the conditions
# listed in the XORP LICENSE file. These conditions include: you must
# preserve this copyright notice, and you cannot mention the copyright
# holders in advertising related to the Software without their permission.
# The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
# notice is a summary of the XORP LICENSE file; the license in that file is
# legally binding.

# $XORP: other/tinderbox/scripts/remote_xorp_copy.sh,v 1.13 2008/01/02 23:57:56 pavlin Exp $

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
