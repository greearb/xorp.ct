#!/usr/bin/env bash

#
# $XORP: xorp/fea/test_fea_rawlink.sh,v 1.3 2008/05/01 03:10:42 pavlin Exp $
#

#
# Test FEA raw link support.
#
# This script started with no arguments will start all required process and
# terminate them at the end of the tests.
#

set -e

UNAME=$(uname)
EDSC=""

onexit()
{
    last=$?
    if [ $last = "0" ]
    then
	echo "$0: Tests Succeeded"
    else
	echo "$0: Tests Failed"
    fi
	

    if [ "$UNAME" = "FreeBSD" ]
    then
	if [ "x$EDSC" != "x" ]
	then
	    ifconfig $EDSC destroy
	fi
    fi

    trap '' 0 2
}

trap onexit 0 2

# Conditionally set ${srcdir} if it wasn't assigned (e.g., by `gmake check`)
if [ "X${srcdir}" = "X" ] ; then srcdir=`dirname $0` ; fi

. ${srcdir}/../utils/xrl_shell_lib.sh

setup_interfaces()
{
	case "$UNAME" in
	   FreeBSD)
		if [ "$(id -u)" != "0" ]
		then
		    echo "$0: Tests Skipped: Not running as root user."
		    exit 0
		fi

		EDSC=$(ifconfig edsc create)
		if [ $? != "0" ]
		then
		    echo "$0: Tests Skipped: Failed to create edsc interface."
		    exit 0
		fi

		ifconfig $EDSC up
		ifname=$EDSC
		vifname=$ifname
		;;
	   Linux)
		if [ "$(id -u)" != "0" ]
		then
		    echo "$0: Tests Skipped: Not running as root user."
		    exit 0
		fi

		ifname="lo"
		vifname=$ifname
		;;
	   *)
		echo "$0: Tests Skipped: $UNAME is neither FreeBSD or Linux."
		exit 0
		;;
	esac
}

configure_fea()
{
    echo "Configuring fea"

    export CALLXRL
    FEA_FUNCS=${srcdir}/xrl_shell_funcs.sh
    local tid=$($FEA_FUNCS start_fea_transaction)

    $FEA_FUNCS configure_all_interfaces_from_system $tid true

    $FEA_FUNCS commit_fea_transaction $tid
}


test_fea_rawlink()
{
    ./test_fea_rawlink -v $ifname $vifname
}

TESTS_NOT_FIXED=""
TESTS="test_fea_rawlink"

# Include command line
RUNITDIR="../utils"
. ${srcdir}/../utils/args.sh

if [ $START_PROGRAMS = "yes" ] ; then
    CXRL="$CALLXRL -r 10"
    runit $QUIET $VERBOSE -c "$0 -s -c $*" <<EOF
    ../libxipc/xorp_finder = $CXRL finder://finder/finder/0.2/get_xrl_targets
    ../fea/xorp_fea        = $CXRL finder://fea/common/0.1/get_target_name
EOF
    trap '' 0
    exit $?
fi

export EDSC
export ifname
export vifname
setup_interfaces

if [ $CONFIGURE = "yes" ] ; then
    configure_fea
    set -e
fi

for i in $TESTS ; do
    $i
done

# Local Variables:
# mode: shell-script
# sh-indentation: 4
# End:
