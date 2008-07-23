#!/bin/sh

# Copyright (c) 2001-2008 XORP, Inc.
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

# $XORP: other/tinderbox/scripts/make_tinderbox_slave.sh,v 1.3 2008/01/04 03:01:42 pavlin Exp $

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
