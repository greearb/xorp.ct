#!/bin/sh

#
# $XORP: xorp/utils/bogon-be-gone.sh,v 1.4 2002/12/09 11:13:07 pavlin Exp $
#

#
# This is a wrapper to give in place sed editing.  The role of the sed
# script is to coerce code into a more xorp coding style friendly
# manner.  It targets some of the common xorp developer bogons that
# everyone makes and is not an indent replacement or a magic format
# fixer.
#
# When you use this script check the diffs and run compilation and
# tests before committing the changes back to the repository.  You will
# get eye strain looking at the diffs :-)
#
# NB You may want to run this script multiple times across the same files
# because some of the sed expressions overlap and don't do the job fully
# otherwise.
#
# NB2 You should probably give a heads up before commiting changes
# proposed by this script since it tends to get lots of nitty things that
# may interfere with other people's pending commits.
#

SCRIPT_NAME="bogon-be-gone.sh"
DEBOGON_SCRIPT=$PWD/$0

# Shell script name ends in "sh", invoked sed script name ends in "sed"
SED_SCRIPT=`echo "$DEBOGON_SCRIPT" | sed -e 's@sh$@sed@'`

if [ ! -f $SED_SCRIPT ] ; then
    echo "Could not find sed script." >2
    echo 1
fi

if [ $# -eq 0 ] ; then
    cat <<-EOF
	usage: ${SCRIPT_NAME} [ file1.hh file1.cc ... ]
	Strips assorted xorp developer bogons from source files.
EOF
    exit 1
fi

# Poor man's inplace sed
for i in $* ; do
    if [ ! -f $i ] ; then
	echo "$i is not a file, exiting" >2
	exit 1
    fi
    cat $i | sed -f $SED_SCRIPT > $i.debog
    cmp $i $i.debog >/dev/null
    if [ $? -ne 0 ] ; then
	mv $i.debog $i
	echo "Updating $i"
    else
	rm $i.debog
    fi
done
