#!/bin/sh

# This program is intended to help keep the XORP modified version of
# OSPFD in sync with the ftp site and patches.  We keep track of the
# applied patches in the file imaginatively titled "applied.patches".
# Any patches that have not been applied, the user is queried about.
# If they respond positively, the patch is applied.

PATCHDIR=`echo $0 | sed 's@/[^/]*$@@'`
OSPFDIR=${PATCHDIR}/..

# File containing list of applied patches
APPLIED=${PATCHDIR}/applied.patches

trap exit SIGINT

query_then_do_patch( ) {
    printf "Apply patch $1 (y/n)? "
    read OKAY
    if [ "X${OKAY}" != "Xy" -a "X${OKAY}" != "Y" ] ; then
	return 1
    fi

    # OSPFD code appears to come from version labelled directories
    # rather than a versioning system.  Change directory path to be
    # xorp ospf directory
    FILT="s@+++ ospfd2[^/]*/@+++ ${OSPFDIR}/@"
    cat $i | sed "${FILT}" | patch

    return 0
}

for i in ${PATCHDIR}/patch2.[0-9] ${PATCHDIR}/patch2.[0-9][0-9]  ; do
  grep $i ${APPLIED} >/dev/null 2>&1
  if [ $? != 0 ] ; then
    query_then_do_patch $i
    if [ $? = 0 ] ; then
      echo $i >> ${APPLIED}
    fi
  else
      echo "Patch $i already applied."
  fi
done

