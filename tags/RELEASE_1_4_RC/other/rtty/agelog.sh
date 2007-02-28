#! /bin/sh

# Copyright (c) 1996 by Internet Software Consortium.
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
# ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
# CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
# DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
# PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
# ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
# SOFTWARE.

# (this is just a local copy to make sure it's available; master source for
# this is elsewhere)
#
# $Id$
#
# agelog -- age log files, by Dave Lennert as told to Bob Desinger and
#           James Brister
#
# Usage: $0 [-m] [-p Bpid] [-s Bsig] logFile \
#           [-p Apid] [-s Asig] howMany [stashDir]
#
#
# The most recent <howMany> logs are kept around by renaming logFile to
# logFile.0, after similarly rolling logFile.0 => logFile.1 => logFile.2 => ...
# If <stashDir> is named, the old logs will be kept in that directory.  If not
# given, the old logs are left in the same directory as the original logFile.
#
# Example:
# `agelog /usr/adm/sulog 2' will, if run weekly, keep 3 files around:
#	/usr/adm/sulog    containing this week's log info
#	/usr/adm/sulog.0  containing last week's log info
#	/usr/adm/sulog.1  containing the week before last's log info
#
# A typical crontab entry:
# #	Keep the most recent 2 weeks worth of uucp logs around in
# #	/tmp/Oldlogs/*, one per day, so that old LOGFILEs will be in
# #	/tmp/Oldlogs/LOGFILE.{0,1,2,...}
# 00 1 * * * /usr/local/agelog /usr/spool/uucp/LOGFILE 14 /tmp/Oldlogs
#
#
# Modification Tue Oct  9 16:48:56 1990 James Brister
#
# This now accepts some options:
#	-m 	if given before the log file name then mv will be used instead
# 	   	of cp to move the file.
#	-p pid	if given before the log file name then a signal will be
#		sent to the specified pid before the moves are done.
#		if given after the log file name then the signal is sent
#		after the moves. The default signal is HUP
#	-s sig	if given before the log file name then the signal sent
#		before the move is changed from HUP to sig. If specified
#		after the log file name then the signal sent after the move
#		is changed from HUP to sig.
#	-h	just displays the usage and exits.
#
# examples:
#	agelog -p 9999 somelog 3
#		this will send a HUP signal to pid 9999 then save 3
#		versions of the log files (using cp for the final move).
#
#	agelog -m -p 8888 somelog -p 8888 4
#		this will send a HUP signal to pid 8888 before saving the
#		4 versions of the log files and then after. It will use mv
#		for the final move (not cp).
#
#	agelog -p 7777 -s ALRM somelog -p 7777 2
#		this will send a ALRM signal to pid 7777, then it will save
#		the log files, then it will send a HUP to pid 7777
#
#	NOTE:   the changing of the BEFORE signal doesn't affect the AFTER
#	        signal. Likewise with the pid's
#		
#

# set -vx

# Initialize:

PATH=/usr/ucb:/usr/bin:/bin:/etc:/usr/lib  # BSD systems have /usr/ucb
export PATH

# traps:  0=exit  1=hangup  2=interrupt  3=quit  15=terminate
trap 'echo 1>&2 "$0: Ow!"; exit 15' 1 2 3 15

MOVE=cp			# default is to COPY log file, not MOVE it.
ASIGNAL=HUP		# signal to send AFTER the move/copy
APID=0			# pid to send signal to AFTER the move/copy
BSIGNAL=HUP		# signal to send BEFORE the move/copy.
BPID=0			# pid to send signal to BEFORE the move/copy
USAGE="Usage: `basename $0` [-m] [-p Bpid] [-s Bsig] logFile 
[-p Apid] [-s Asig] howMany [stashDir]"


#
# Digest arguments:
#

# get the BEFORE arguments

while [ `expr "$1" : '-.*'` -gt 0 ]
do
	case "$1" in
		-h)	echo $USAGE
			exit 0 
			;;
		-m) 	MOVE=mv 
			;;
		-p) 	BPID=$2
			shift 
			;;
		-s)	BSIGNAL=$2
			shift
			;;
		-*)	echo 1>&2 $USAGE
			exit 2
			;;
	esac
	shift
done


# now get the log file name

if [ 0 -eq $# ]
then
	echo $USAGE
	exit 2
else
	log="$1"	# logFileName
	shift
fi


# now get the AFTER arguments

while [ `expr "$1" : '-.*'` -gt 0 ]
do
	case "$1" in
		-p) 	APID=$2 
			shift 
			;;
		-s)	ASIGNAL=$2
			shift
			;;
		-*)	echo 1>&2 $USAGE
			exit 2
			;;
	esac
	shift
done

# now get the numer of copies to save and the stash directory

if [ 0 -eq $# ]
then
	echo 1>&2 $USAGE
	exit 2
else
	max="$1"	# howMany
	shift
fi

if [ 0 -eq $# ]
then	# no directory to stash them in; use log's directory
	head=`expr $log : '\(.*/\).*'`	# /a/b/x => /a/b/
else	# user specified a directory
	if [ ! -d "$1" ]
	then
		echo 1>&2 "$0: $1 is not a directory"
		exit 2
	else
		head="$1/"
	fi
fi

#
# Send signal if required BEFORE move
#

if [ 0 -ne $BPID ]
then
	kill -$BSIGNAL $BPID
fi


#
# Rename log.$max-1 => ... => log.3 => log.2 => log.1
#

arch="${head}`basename $log`"	# name of archive files, sans {.0, .1, ...}

older=`expr $max - 1`	# ensure we had a number in $2
if [ $? -eq 2 ]
then	# not a number, or some problem
	echo 1>&2 "$0: cannot decrement $max"
	exit 2
fi

while [ $older -gt 0 ]
do	# age the logfiles in the stashdir
	old=`expr $older - 1`
	if [ -f $arch.$old ]
	then
		mv $arch.$old $arch.$older
	fi
	older=`expr $older - 1`
done


#
# Old logfiles are all rolled over; now move the current log to log.0
#


# Use cp instead of mv to retain owner & permissions for the original file,
# and to avoid prematurely aging any info going into the file right now,
# unless the user has given the -m option

if [ -f $log ]
then
	# don't create an old log if $2 was 0
	test $max -gt 0 && $MOVE $log $arch.0
	cp /dev/null $log	# clear out log
fi

#
# Now send signals if required
#

if [ 0 -ne $APID ]
then
	kill -$ASIGNAL $APID
fi

exit 0
