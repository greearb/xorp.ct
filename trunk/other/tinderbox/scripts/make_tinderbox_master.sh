#!/bin/sh

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

# $XORP: other/tinderbox/scripts/make_tinderbox_master.sh,v 1.10 2008/10/02 21:32:16 bms Exp $

#
# Script to create and populate the tinderbox directories on a host
# which is to act as the 'master host', that is, it will invoke builds
# and checks periodically on remote 'slave hosts' and report back on
# the results of those background jobs.
#
# This script should be run as the master tinderbox user, and accepts
# a single argument: where to put the tinderbox itself. This can be
# a different directory from the sandbox user's $HOME.
#
# TBOXCVSROOT is used to check out the tinderbox source.
# TBOXHOME is inferred as the current user's home directory from ${HOME}.
#  There is a sanity check for this directory.
# TBOXMAILTO is used only as a guide.
# TBOXPATH is inferred as ${TBOXHOME}/tinderbox_master, or the first
#  positional parameter to the script. It will be created if it does
#  not already exist; if it cannot be created, a fatal error occurs.
#  IT MUST BE AN ABSOLUTE PATH.
# TBOXUSER is inferred as the current user from ${USER}.
#

####################################################################

MKDIR="/bin/mkdir -p"
TBOXCVSROOT_DEFAULT=":pserver:xorpcvs@anoncvs.xorp.org:/cvs"

TBOXUSER=${USER:?"No USER specified in environment."}
TBOXHOME=${HOME:?"No HOME specified in environment."}
TBOXPATH=${1:-${TBOXHOME}/tinderbox_master}
TBOXMAILTO=${TBOXMAILTO:-"xorp-tinderbox@xorp.org"}
TBOXCVSROOT=${TBOXCVSROOT:-${TBOXCVSROOT_DEFAULT}}

####################################################################

echo "Making master tinderbox..."

#
# If the CVSROOT is remote, force our new ssh key to be used.
#
( echo $TBOXCVSROOT | grep ':' >/dev/null 2>&1 ) \
 && export CVSRSH="ssh -i ${TBOXHOME}/.ssh/tinderbox"

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
# Create a key for tinderbox in ${TBOXHOME}/.ssh/tinderbox,
# if it doesn't already exist. Append the public key to
# ${TBOXHOME}/.ssh/authorized_keys in a manner which does not
# clobber already existing files.
#
if [ ! -f ${TBOXHOME}/.ssh/tinderbox ] ; then
	${MKDIR} ${TBOXHOME}/.ssh
	ssh-keygen -f "${TBOXHOME}/.ssh/tinderbox" -t dsa \
	  -C "Tinderbox Sandbox" -N ""
	if [ $? != 0 ] ; then
		echo "ssh-keygen failed."
		exit 12
	fi
	touch ${TBOXHOME}/.ssh/authorized_keys
	cat ${TBOXHOME}/.ssh/tinderbox.pub >> ${TBOXHOME}/.ssh/authorized_keys
fi


#
# Create a symbolic link to ${TBOXPATH} in ${TBOXHOME}, if
# they differ.
#
cd ${TBOXHOME}
if [ "x${TBOXHOME}/tinderbox_master" != "x${TBOXPATH}" ] ; then
	ln -sf ${TBOXPATH} tinderbox_master
fi

#
# Populate the tinderbox tree by checking out what it needs from CVS
# *in the right place*.
#
cd ${TBOXPATH} && \
 cvs -d ${TBOXCVSROOT} checkout -d scripts other/tinderbox/scripts && \
 cvs -d ${TBOXCVSROOT} checkout data/bgp

# Fix script permissions after cvs checkout.
find ${TBOXPATH} -type f -name '*.sh' | xargs chmod 0755

#
# Print a message telling the admin how to set up the crontab;
# use a sed substitution across a here-document.
#
tmp_crontab="/tmp/tmp_crontab.$$"
cat > ${tmp_crontab} <<__EOF__
The Tinderbox has now been built successfully. Please now create a
crontab entry with the following format (as a guide):

SHELL=/bin/sh
PATH=/etc:/bin:/sbin:/usr/bin:/usr/sbin
HOME=@TBOXPATH@
MAILTO=@TBOXMAILTO@
#
# Run Mon, Wed, Fri if it takes > 24 hours to complete all tests
#minute	hour	mday	month	wday	command
0	22	*	*	1,3,5	@TBOXPATH@/scripts/tinderbox.sh

The XORP Tinderbox should run using a strictly POSIX 1003.2 compliant
Bourne shell. This includes the FreeBSD /bin/sh, GNU bash in POSIX
mode, and many Korn-style shells.
__EOF__

cat ${tmp_crontab} | sed			\
	-e "s#@TBOXMAILTO@#${TBOXMAILTO}#g"	\
	-e "s#@TBOXPATH@#${TBOXPATH}#g"
rm -f ${tmp_crontab}

####################################################################
exit 0
