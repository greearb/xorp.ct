#!/bin/sh
#
# $XORP: other/tinderbox/scripts/build_xorp.sh,v 1.8 2004/10/19 08:26:48 bms Exp $
#

CONFIG="$(dirname $0)/config"
. $CONFIG

###############################################################################

XORPDIR=${ROOTDIR}/xorp
wakefile=".wake-$$"
build_command="./configure ${CONFIGURE_ARGS} && gmake -k $@"

#
# Remove any old completion tokens.
#
cd ${XORPDIR} && rm -f .wake-* 2>/dev/null

#
# Spawn a background pipeline to run the build command and capture both
# its output and its exit status. 
# Note that no job control is available in a strict POSIX Bourne Shell.
#
# XXX: Try removing inner subshell to save on a fork (replace with {}),
# previous problems were due to a typo above.
#
( ( eval $build_command ) 2>&1 ; \
	subshell_status=$? ; \
	echo ${subshell_status} > ${wakefile} \
) & subshell_pid=$!

#
# Announce the invocation of the background pipeline in the parent.
# Get current UTC epoch, and calculate when the timer is due to expire.
#
echo "Subshell launched with pid ${subshell_pid} at $(date '+%H:%M:%S %Z')."
expiry=$(date "+%s")
expiry=$((${expiry} + ${TIMEOUT}))

#
# Wait for command completion by polling for the creation of ${wakefile}
# and the completion of the subshell. Use -s instead of -f for the test
# to avoid racing the subshell when it has not yet flushed its output
# to ${wakefile}. If we time out, break out of the loop and set a flag.
#
while [ ! -s ${wakefile} ] ; do
    now=$(date "+%s")
    if [ "$now" -gt "$expiry" ] ; then
	timed_out="1"
	break
    fi
    sleep ${TIMEOUT_CHECK}
done

#
# If we timed out on notification from the subshell, kill the subshell's
# entire process group.
# XXX: Need to test what happens here using 'funkster' to simulate wedge.
# XXX: Do we need to wait for the child anyway to catch it?
# XXX: What if we've raced the child and we have no file? Erroneous error?
#
if [ "x${timed_out}" = "x1" ] ; then
	err="Process appears wedged. Timed out after ${TIMEOUT} seconds."
	bar=$(echo $err | sed 's/./=/g')
	printf "\n${bar}\n${err}\n${bar}\n"
	killcmd="kill -KILL -- -${subshell_pid} 2>/dev/null"
	eval ${killcmd}
	#
	# More elaborate stuff re: try background kill, then kill the
	# killer, below:
	#
	#&& echo "FAIL: ${killcmd} & kill_pid=$!
	#wait ${kill_pid}
	#kill -KILL ${kill_pid} 2>/dev/null && echo "FAIL: killing the killer"
	#
fi

#
# If we have a non-zero completion token, return the exit status within
# from the subshell to the parent by exiting with it.
# We have to use $(cat file) in straight up POSIX shells.
# $(<${file}) -> GNUism. $(<file) -> KSHism.
# XXX: Need to test what happens here using 'funkster' to simulate wedge.
#
if [ -s ${wakefile} ] ; then
    err=$(cat ${wakefile})
    echo "Subshell ran to completion with exit status ${err}."
    rm -f ${wakefile}
    exit ${err}
fi

#
# Exit, returning failure; the job timed out.
#
exit 100
