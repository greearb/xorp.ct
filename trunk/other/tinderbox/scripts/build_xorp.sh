#!/bin/sh

CONFIG="$(dirname $0)/config"
. $CONFIG

TIMEOUT=3600

timeout()
{
    cat >&2 <<-EOF

****************************************************************************** 
Suspect process is wedged. Timed out after a wait of ${TIMEOUT} seconds.
****************************************************************************** 
EOF
    kill 0
    exit -1
}

#
# set_timeout <seconds>
#
set_timeout()
{
    (sleep $1 && timeout ) 2>&1 > /dev/null &
    TIMEOUT_PID=$!
}

unset_timeout()
{
    kill -9 ${TIMEOUT_PID} 2>&1 > /dev/null
}

###############################################################################

cd ${ROOTDIR}/xorp
set_timeout ${TIMEOUT}
( ./configure 1>&2 ) && ( gmake -k $@ 1>&2 )
unset_timeout


