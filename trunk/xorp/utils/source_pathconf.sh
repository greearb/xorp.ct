#!/bin/sh

#
# This is an example of how to find and source the pathconf shell script.
# The latter sets path related variables.

#
# source_pathconf [ <search_start_dir> ]
#
# Search for and source pathconf.sh shell script.  By default the the
# search starts in the present working directory (as specified by
# $PWD).  This may be overridden by specifying a <search_start_dir>.
#
source_pathconf()
{
    local sdir sfile
    sdir=${1:-${PWD}}
    while test ! ${sdir} -ef "/" ; do
	sfile=${sdir}/utils/pathconf.sh
	if [ -f ${sdir}/utils/pathconf.sh ] ; then
	    . ${sfile}
	    return 0
	fi
	sdir=${sdir}/..
    done
    return 1
}