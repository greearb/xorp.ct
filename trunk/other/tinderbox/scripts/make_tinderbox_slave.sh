#!/bin/sh
#
# $XORP$
#
# Script to create the tinderbox directories on a host which is to act as
# a 'slave host'. This script should be run as the slave tinderbox user.
#
# TBOXHOME is inferred as the current user's home directory from ${HOME}.
#  There is a sanity check for this directory.
# TBOXPATH is inferred as ${TBOXHOME}/tinderbox, or the first
#  positional parameter to the script. It will be created if it does
#  not already exist; if it cannot be created, a fatal error occurs.
#  IT MUST BE AN ABSOLUTE PATH.
# TBOXUSER is inferred as the current user from ${USER}.
#

TBOXUSER=${USER:?"No USER specified in environment."}
TBOXHOME=${HOME:?"No HOME specified in environment."}
TBOXPATH=${1:-${TBOXHOME}/tinderbox}

MKDIR="/bin/mkdir -p"

if [ ! -d ${TBOXHOME} ] ; then
	echo "The directory ${TBOXHOME} does not exist."
	exit 10
fi

${MKDIR} ${TBOXPATH}
if [ ! \( -d ${TBOXPATH} -a -w ${TBOXPATH} -a -x ${TBOXPATH} \) ] ; then
	echo "The directory ${TBOXPATH} could not be created or is not"
	echo "writable by the user ${TBOXUSER}."
	exit 11
fi

# 
# Create a symbolic link to ${TBOXPATH} in ${TBOXHOME}, if
# they differ.
# 
cd ${TBOXHOME}
if [ "x${TBOXHOME}/tinderbox" != "x${TBOXPATH}" ] ; then
        ln -sf ${TBOXPATH} tinderbox
fi

# The end. The tinderbox itself should do the rest.
exit 0
