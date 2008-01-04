#!/bin/sh

#
# $XORP: xorp/devnotes/update_copyright.sh,v 1.6 2007/02/16 22:51:14 pavlin Exp $
#

#
# This is a script to update the copyright message for all files.
# It must be run in the top directory of a a fresh checked-out copy
# of the source code, otherwise it may overwrite something else.
#
# Note1: Before running the script you must modify the OLD_YEAR and
# NEW_YEAR variables below to match the particular replacement string
# with the copyright message.
#
# Note2: You should give a heads up before commiting changes
# proposed by this script since it will modify almost all files in the
# source tree and this may interfere with other people's pending commits.
#

#set -x

#
# XXX: MODIFY THESE AS APPROPRIATE!
#
# Note that the string needs to be run twice with different pairs
# of OLD_YEAR and NEW_YEAR: the first time to update the "X-Y" copyright years,
# and second time to update the "(c) Y" copyright year.
#
# Note the hack in defining the string so the script doesn't update itself.
#
#OLD_YEAR="(c) 2008"
#NEW_YEAR="(c) 2008-2009"
OLD_YEAR="-2008"
NEW_YEAR="-2009"
OLD_STRING="${OLD_YEAR} International Computer Science Institute"
NEW_STRING="${NEW_YEAR} International Computer Science Institute"

#
# Internal variables
#
SED_COMMAND="s/${OLD_STRING}/${NEW_STRING}/"
TMP_SUFFIX="debog"

find . -type f -print | 
while read FILENAME
do
    grep -e "${OLD_STRING}" "${FILENAME}" > /dev/null
    if [ $? -ne 0 ] ; then
	# XXX: no match found
	continue
    fi

    cat "${FILENAME}" | sed "${SED_COMMAND}" > "${FILENAME}"."${TMP_SUFFIX}"
    if [ $? -ne 0 ] ; then
	echo "Error updating ${FILENAME}"
	exit 1
    fi
    cmp "${FILENAME}" "${FILENAME}"."${TMP_SUFFIX}" >/dev/null
    if [ $? -ne 0 ] ; then
	mv "${FILENAME}"."${TMP_SUFFIX}" "${FILENAME}"
	echo "Updating ${FILENAME}"
    else
	rm "${FILENAME}"."${TMP_SUFFIX}"
    fi
done
