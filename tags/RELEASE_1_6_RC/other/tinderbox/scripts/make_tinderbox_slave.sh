#!/bin/sh

# Copyright (c) 2001-2008 XORP, Inc.
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

# $XORP: other/tinderbox/scripts/make_tinderbox_slave.sh,v 1.4 2008/07/23 04:42:54 pavlin Exp $

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
