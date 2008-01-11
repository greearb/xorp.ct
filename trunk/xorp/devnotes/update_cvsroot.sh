#!/bin/sh

#
# $XORP: xorp/devnotes/update_cvsroot.sh,v 1.2 2007/03/02 02:31:21 pavlin Exp $
#

#
# This is a script to update the CVS/Root files.
# It must be run from the top directory of a checked-out copy
# of the source code, otherwise it may overwrite something else.
#
# Note: Before running the script you might want to modify the
# ANON_CVS_ROOT or DEVEL_CVS_ROOT strings.
#

#
# XXX: MODIFY THIS AS APPROPRIATE!
#
ANON_CVS_ROOT=":pserver:xorpcvs@anoncvs.xorp.org:/cvs"
DEVEL_CVS_ROOT="cvs.xorp.org:/usr/local/www/data/cvs"

#
# Local variables
#
NEW_CVS_ROOT=""
TMP_CVS_ROOT_FILENAME=/tmp/xorp_cvsroot.$$

#
# Print usage and exit
#
usage()
{
    cat <<EOF
Usage: $0 [-a | -d | -h]
    -a     Set CVS Root to the anonymous CVS
           Default: ${ANON_CVS_ROOT}
    -d     Set CVS Root to the developer CVS
           Default: ${DEVEL_CVS_ROOT}
    -h     Print usage
EOF
    exit 1
}

#
# Cleanup function
#
cleanup_atexit()
{
    rm -f "${TMP_CVS_ROOT_FILENAME}"
}

#
# Test the number of the command-line arguments
#
if [ $# -lt 1 ] ; then
    usage
fi

#
# Parse the command-line arguments
#
for i in "$@"
do
    case "$i" in
	-a)
	    NEW_CVS_ROOT="${ANON_CVS_ROOT}"
	    ;;
	-d)
	    NEW_CVS_ROOT="${DEVEL_CVS_ROOT}"
	    ;;
	-h)
	    usage
	    ;;
	*)
	    usage
	    ;;
    esac
done

if [ -z "${NEW_CVS_ROOT}" ] ; then
    echo "New CVS Root is empty!"
    usage
fi

echo "New CVS Root is ${NEW_CVS_ROOT}"

#
# Trap some signals to cleanup state
#
trap "cleanup_atexit" 0 2 3 15

#
# Write a temporary copy of the new CVS Root file
#
echo -n "${NEW_CVS_ROOT}" > "${TMP_CVS_ROOT_FILENAME}"

#
# Do the file update.
# If both old and new CVS Root are identical, then don't overwrite anything.
#
find . -name CVS -and -type d -print | 
while read DIRNAME
do
    FOUND_CVS_ROOT_FILENAME="${DIRNAME}/Root"

    if [ ! -f "${FOUND_CVS_ROOT_FILENAME}" ] ; then
	continue
    fi

    cmp -s "${TMP_CVS_ROOT_FILENAME}" "${FOUND_CVS_ROOT_FILENAME}"
    if [ $? -eq 0 ] ; then
	# Old and new CVS Root are identical
	echo "Ignoring ${FOUND_CVS_ROOT_FILENAME}"
	continue
    fi

    # Copy the file
    echo "Updating ${FOUND_CVS_ROOT_FILENAME}"
    cp -p "${TMP_CVS_ROOT_FILENAME}" "${FOUND_CVS_ROOT_FILENAME}"
done
