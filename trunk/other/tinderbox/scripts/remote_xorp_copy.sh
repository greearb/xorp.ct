#!/bin/sh -e

# Copyright (c) 2001-2009 XORP, Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, Version 2, June
# 1991 as published by the Free Software Foundation. Redistribution
# and/or modification of this program under the terms of any other
# version of the GNU General Public License is not permitted.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
# see the GNU General Public License, Version 2, a copy of which can be
# found in the XORP LICENSE.gpl file.
# 
# XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
# http://xorp.net

# $XORP: other/tinderbox/scripts/remote_xorp_copy.sh,v 1.16 2008/10/02 21:32:17 bms Exp $

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
