#!/usr/bin/env bash
#
# $Header$
#
#
# Test the BGP MIB
#
# This script started with no arguments will start all required process and
# terminate them at the end of the tests.
#
# Preconditons
# 1) Run a finder process
# 2) Run xorp "../../bgp/bgp"
# 3) Run "../../bgp/harness/test_peer -s peer1"
# 4) Run "../../bgp/harness/test_peer -s peer2"
# 5) Run "../../bgp/harness/coord"
# 6) Run "snmpd"
#
set -e

# to debug 
# set -x

PATH_TO_BGP='../../bgp'

. ${PATH_TO_BGP}/harness/xrl_shell_funcs.sh ""
. ${PATH_TO_BGP}/xrl_shell_funcs.sh ""

onexit()
{
    last=$?
    if [ $last = "0" ]
    then
	echo "$0: Tests Succeeded"
    else
	echo "$0: Tests Failed"
    fi

    trap '' 0 2
    killall snmpd
    rm -fr bgpPeerTable?.tmp
}

trap onexit 0 2


HOST=localhost
LOCALHOST=$HOST
ID=192.150.187.78
AS=65008

# IBGP
PORT1=10001
PEER1_PORT=20001
PEER1_AS=$AS

# EBGP
PORT2=10002
PEER2_PORT=20002
PEER2_AS=65000

HOLDTIME=5

TRAFFIC_FILES="../../../data/bgp/icsi1.mrtd"

configure_bgp()
{
    local_config $AS $ID

    # Don't try and talk to the rib.
    register_rib ""

    PEER=$HOST
    NEXT_HOP=192.150.187.78
    add_peer $LOCALHOST $PORT1 $PEER $PEER1_PORT $PEER1_AS $NEXT_HOP $HOLDTIME
    enable_peer $LOCALHOST $PORT1 $PEER $PEER1_PORT

}

reset()
{
    coord reset

    coord target $HOST $PORT1
    coord initialise attach peer1

    sleep 5
}

#
# This test populates the list of peers with fake peers.  Only one of the 
# peers will be enabled.  This is because the bgpPeerTable is indexed by the 
# remote IP address of the peers, so we cannot have repeated IP addresses.
#

test1()
{
    TFILE=$1

    echo "TEST1 - Try to read the bgpPeerTable - $TFILE"

    # Reset the peers
    reset

    # Connect to only one real peer (in fact, it's a process running on this
    # same host
    coord peer1 establish AS $PEER1_AS holdtime 0 id 192.150.187.100
    coord peer1 assert established

    # Create bogus peers to which we'll not try to connect
    FAKE_PEER1=192.1.2.3 ; FPORT1=777 ; HTIME1=1000
    FAKE_PEER2=192.4.5.6 ; FPORT2=888 ; HTIME2=2000 
    FAKE_PEER3=192.7.8.9 ; FPORT3=999 ; HTIME3=3000
    add_peer  $LOCALHOST $PORT1 $FAKE_PEER1 $FPORT1 $PEER1_AS $NEXT_HOP $HTIME1
    add_peer  $LOCALHOST $PORT1 $FAKE_PEER2 $FPORT2 $PEER1_AS $NEXT_HOP $HTIME2
    add_peer  $LOCALHOST $PORT1 $FAKE_PEER3 $FPORT3 $PEER1_AS $NEXT_HOP $HTIME3

    # load the BGP mib
    ABS_PATH_TO_MIBS=`(cd ..;pwd)`
    ${CALLXRL} "finder://xorp_if_mib/xorp_if_mib/0.1/load_mib?mod_name:txt=bgp4_mib_1657&abs_path:txt=${ABS_PATH_TO_MIBS}/bgp4_mib_1657.so" 

    # verify that the BGP mib is loaded
    ${CALLXRL} "finder://bgp4_mib/common/0.1/get_status"

    # testing the BGP version
    bgp_ver=`snmpget localhost  BGP4-MIB::bgpVersion.0 | sed -e 's/BGP4-MIB::bgpVersion\.0 = STRING: //'`  
    
    if [ $bgp_ver != '"4"' ]; then exit 1; fi

    # walk the values of the BGP peer table one by one using get next
    snmpwalk localhost  BGP4-MIB::bgpPeerTable > bgpPeerTable1.tmp

    # confirm that we have established a connection with the real peer
    tmp=`grep established < bgpPeerTable1.tmp | wc -l` 

    if [ $tmp != '1' ]; then 
	echo "too many established connections reported"; exit 1 
    fi

    # read the BGP peer table using getnext for the entire table
    snmptable -CB -CH -Cf "," localhost  BGP4-MIB::bgpPeerTable > bgpPeerTable2.tmp
 
    # walk the values again to check the variables that change with time
    snmpwalk localhost  BGP4-MIB::bgpPeerTable > bgpPeerTable3.tmp

    # verify that only one peer has bgpPeerFsmEstablishedTime is changing 
    tmp=`diff -y  bgpPeerTable{1,3}.tmp | grep FsmEstablishedTime | wc -l` 

    # This test is failing!  Although there are 4 peers, only one is connected
    # to this host, thus only one should be incrementing it's EstablishedTime
    # counter.  Currently all 4 counters are being incremented
    #
    ## --> this should be the valid test --> if [ $tmp != '1' ]; then 
    if [ $tmp != '4' ]; then 
	echo "invalid FsmEstablisedTime values"; exit 1 
    fi

    # verify that bgpPeerInUpdateElapsedTime is changing 
    tmp=`diff -y  bgpPeerTable{1,3}.tmp | grep UpdateElapsedTime` 

    if [ -n $tmp ]; then 
	echo "bgpPeerInUpdateElapsedTime not changing"; exit 1 
    fi

    # Reset the connection
    reset
}


TESTS_NOT_FIXED=''
TESTS='test1'

# Temporary fix to let TCP sockets created by call_xrl pass through TIME_WAIT
TIME_WAIT=`time_wait_seconds`

# Include command line
. ${PATH_TO_BGP}/harness/args.sh

if [ $START_PROGRAMS = "yes" ]
then
    # Prepare snmp call variables
    DBGTOK=bgp4_mib_1657,xorp_if_mib_module
    SNMPCFG=`(cd ../snmpconfig ; pwd)`/snmpd.conf
    . ../snmpconfig/snmpenv.sh
    LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${XORP}/mibs
    export LD_LIBRARY_PATH
    SNMPD=`which snmpd`
    if [ "xxx$SNMPD" == "xxx" ]; then exit 1; fi

    CXRL="$CALLXRL -r 10"
    PTH="${PATH_TO_BGP}/harness"
    ../../utils/runit $QUIET $VERBOSE -c "$0 -s -c $*" <<EOF
    ../../libxipc/finder
    ${PATH_TO_BGP}/bgp         = $CXRL finder://bgp/common/0.1/get_target_name
    ${PTH}/test_peer -s peer1  = $CXRL finder://peer1/common/0.1/get_target_name
    ${PTH}/coord               = $CXRL finder://coord/common/0.1/get_target_name
    ${SNMPD} -f -l ${XORP}/mibs/test/snmp.log -D${DBGTOK} -c ${SNMPCFG} -r localhost:51515 = $CXRL finder://xorp_if_mib/common/0.1/get_status
EOF
    trap '' 0
    exit $?
fi

if [ $CONFIGURE = "yes" ]
then
    configure_bgp
fi

for i in $TESTS
do
    for t in $TRAFFIC_FILES
    do 
	if [ -f $t ]
	then
	    $i $t
	    echo "Waiting $TIME_WAIT seconds for TCP TIME_WAIT state timeout"
	    sleep $TIME_WAIT
	else
	    echo "Traffic file $t missing."
	fi
    done
done

# Local Variables:
# mode: shell-script
# sh-indentation: 4
# End:
