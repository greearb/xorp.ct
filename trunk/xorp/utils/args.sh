#
# $XORP: xorp/bgp/harness/args.sh,v 1.3 2003/10/23 00:13:47 pavlin Exp $
#

#
# All the command line processing for the test scripts in one place.
#

START_PROGRAMS="yes"
CONFIGURE="yes"
QUIET=""
VERBOSE=""
RESTART="yes"

# -q (Quiet)
# -v (Verbose)
# -s (Single) Do not start ancillary programs. They must already be running.
# -t (Tests) The tests to run.
# -a (All) Run all the tests including the ones that trigger bugs.
# -b (Bugs) Run only the tests that trigger bugs.
# -c (Configure) In (Single) mode BGP and the RIB are not configured. 
#                Force configuration.
# -l (Leave) Leave the ancillary programs running between tests.

while getopts "qvsbt:abcl" args
do
    case $args in
    q)
	QUIET="-q"
	;;
    v)
	VERBOSE="-v"
	;;
    s)
	START_PROGRAMS="no"
	CONFIGURE="no"
	;;
    t)
	TESTS="$OPTARG"
	;;
    a)
	TESTS="${TESTS} ${TESTS_NOT_FIXED}"
	;;
    b)
	TESTS=${TESTS_NOT_FIXED}
	;;
    c)
	REALLY_CONFIGURE="yes"
	;;
    l)
	RESTART="no"
	;;
    *)
	echo "default"
	;;
    esac
done

if [ ${REALLY_CONFIGURE:-""} = "yes" ]
then
    CONFIGURE="yes"
fi

if [ $START_PROGRAMS = "yes" -a $RESTART = "yes" ]
then
    for i in $TESTS
    do
	COMMAND="$0 $QUIET $VERBOSE -l -t $i"
	echo "Entering $COMMAND"
	$COMMAND
	echo "Leaving $COMMAND"
    done
    trap '' 0
    exit 0
fi
