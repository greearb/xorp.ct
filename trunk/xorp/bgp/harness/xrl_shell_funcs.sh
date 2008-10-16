#!/bin/sh

#
# $XORP: xorp/bgp/harness/xrl_shell_funcs.sh,v 1.12 2008/09/29 23:40:35 atanu Exp $
#

CALLXRL=${CALLXRL:-../../libxipc/call_xrl -w 20}
BASE=${BASE:-test_peer} # Set BASE in callers environment.

#
# Command to the coordinator.
#
coord()
{
    echo "Coord $* "
    $CALLXRL "finder://coord/coord/0.1/command?command:txt=$*"

    if [ "${NOBLOCK:-false}" = "true" ]
    then
	return
    fi

    # Try ten times for the operation to complete, waiting 10 seconds
    local i
    for i in 1 2 3 4 5 6 7 8 9 10
    do
	if ! $CALLXRL "finder://coord/coord/0.1/pending" |grep true > /dev/null
	then
	    return
	fi
	echo "Operation in coordinator still pending try number: $i"
	sleep 1
    done

    echo "***OPERATION DID NOT COMPLETE. TRY INCREASING THE TIME IN PENDING***"

    return -1
}

status()
{
#    echo "Status $* "
    $CALLXRL "finder://coord/coord/0.1/status?peer:txt=$1" |
    sed -e 's/^status:txt=//' -e 's/+/ /g'
}

pending()
{
    echo "Pending "
    $CALLXRL "finder://coord/coord/0.1/pending"
}

#
# Commands to the test peers
#
register()
{
    echo "Register $* "
    $CALLXRL "finder://$BASE/test_peer/0.1/register?coordinator:txt=$1"
}

packetisation()
{
    echo "Packetisation $* "
    $CALLXRL "finder://$BASE/test_peer/0.1/packetisation?protocol:txt=$1"
}

use_4byte_asnums()
{
    echo "Packetisation $* "
    $CALLXRL "finder://$BASE/test_peer/0.1/use_4byte_asnums?use:bool=$1"
}

connect()
{
    echo "Connect $* "
    $CALLXRL "finder://$BASE/test_peer/0.1/connect?host:txt=$1&port:u32=$2"
}

send()
{
    echo "Send $* "
    $CALLXRL "finder://$BASE/test_peer/0.1/send?data:txt=$*"
}

listen()
{
    echo "Listen $* "
    $CALLXRL "finder://$BASE/test_peer/0.1/listen?address:txt=$1&port:u32=$2"
}

bind()
{
    echo "Bind $* "
    $CALLXRL "finder://$BASE/test_peer/0.1/bind?address:txt=$1&port:u32=$2"
}

disconnect()
{
    echo "Disconnect $* "
    $CALLXRL "finder://$BASE/test_peer/0.1/disconnect"
}

reset()
{
    echo "Reset $* "
    $CALLXRL "finder://$BASE/test_peer/0.1/reset"
}

terminate()
{
    echo "Terminate "
    $CALLXRL "finder://$BASE/test_peer/0.1/terminate"
}

# We have arguments.
if [ $# != 0 ]
then
    $*
fi

# Local Variables:
# mode: shell-script
# sh-indentation: 4
# End:
