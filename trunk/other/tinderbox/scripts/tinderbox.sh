#!/bin/sh 

# $XORP: other/tinderbox/scripts/tinderbox.sh,v 1.1.1.1 2002/12/11 23:55:14 hodson Exp $

CONFIG="$(dirname $0)/config"
. $CONFIG

post_log()
{
    # File containing filtered error log
    FILTFILE=${ERRFILE}.filt

    # File containing email message
    MSGFILE=${ERRFILE}.mail.msg

    cat ${ERRFILE} | awk -f ${SCRIPTSDIR}/filter-error.awk > ${FILTFILE}
    SUBJECT="$1: $2"
    TOADDR="$3"

    cat ${SCRIPTSDIR}/boilerplate ${FILTFILE} | 			      \
      sed -e "s/@To@/${TOADDR}/"					      \
	  -e "s/@From@/${FROMADDR}/"					      \
	  -e "s/@ReplyTo@/${REPLYTOADDR}/"				      \
	  -e "s/@Subject@/${SUBJECT}/"					      \
      > ${MSGFILE}
    ${POST} $MSGFILE

    rm -f ${FILTFILE} ${MSGFILE}
}

#
# init_log_header <file> <host>
#
init_log_header()
{
    local HEADER RHOST NOW SINFO CINFO
    HEADER=$1
    RHOST=$2
    NOW=`date "+%Y-%m-%d %H:%M:%S %Z"`
    SINFO=`ssh -n ${RHOST} 'uname -s -r'`
    CINFO=`ssh -n ${RHOST} 'gcc -v 2>&1' | grep 'version'`
    cat >${HEADER} <<EOF
-------------------------------------------------------------------------------
Remote build on: ${RHOST}
Start time:	 ${NOW}
System Info:	 ${SINFO}
Compiler Info:	 ${CINFO}
-------------------------------------------------------------------------------

EOF
}

extoll()
{
    post_log "$1" "$2" "${REPORTOKADDR}"
}

harp()
{
    post_log "$1" "$2" "${REPORTBADADDR}"
}

scp_path_host() {
    echo $1 | sed 's/\([A-z0-9]*\)\..*/\1/'
}

scp_path_directory()
{
    echo $1 | sed 's/.*://'
}

run_tinderbox() {
    #
    # Copy and build latest code on tinderbox machines
    #
    for i in $REMOTEHOSTS ; do
	RHOST=`scp_path_host $i`
	DIR=`scp_path_directory $i`
	ERRFILE="${ROOTDIR}/${RHOST}"
	HEADER="${ERRFILE}.header"

	if [ "${DIR}" = "${RHOST}" -o -z "${DIR}" ] ; then
	    DIR="."
	fi

	echo "Remote copy ${RHOST} ${DIR}" > ${ERRFILE}
	${SCRIPTSDIR}/remote_xorp_copy.sh ${RHOST} ${DIR} >>${ERRFILE} 2>&1
	if [ $? -ne 0 ] ; then
	    harp "$HOSTNAME" "remote copy failed to ${RHOST}"
	    continue
	fi

	init_log_header ${HEADER} ${RHOST}

	cp ${HEADER} ${ERRFILE}
	ssh -n ${RHOST} ${DIR}/scripts/build_xorp.sh >>${ERRFILE} 2>&1
	if [ $? -ne 0 ] ; then
	    harp ${RHOST} "remote build failed"
	    continue
	fi

	cp ${HEADER} ${ERRFILE}
	ssh -n ${RHOST} ${DIR}/scripts/build_xorp.sh check >>${ERRFILE} 2>&1
	if [ $? -ne 0 ] ; then
	    harp ${RHOST} "remote check failed"
	    continue
	fi

	echo "No problems to report" > ${ERRFILE}
	extoll "${RHOST}" "build and test good"
    done
}

checkout() {
    #
    # Checkout latest code
    #
    ${SCRIPTSDIR}/co_xorp.sh >${ERRFILE} 2>&1
    if [ $? -ne 0 ] ; then
	harp "$LOCALHOST: checkout failed"
	exit
    fi
}

ERRFILE=${ROOTDIR}/`scp_path_host $LOCALHOST`
if [ ! -f ${ERRFILE} ] ; then
    cat /dev/null > ${ERRFILE}
fi

checkout
run_tinderbox

