#!/bin/sh 

# $XORP: other/tinderbox/scripts/tinderbox.sh,v 1.3 2003/01/20 20:36:23 hodson Exp $

CONFIG="$(dirname $0)/config"
. ${CONFIG}

filter_and_post_log()
{
    local subject toaddr errfile filtfile msgfile

    subject="$1: $2"
    toaddr="$3"
    errfile="$4"

    # File containing filtered error log
    filtfile=${errfile}.filt

    # File containing email message
    msgfile=${errfile}.mail.msg

    cat ${errfile} | awk -f ${SCRIPTSDIR}/filter-error.awk > ${filtfile}

    cat ${SCRIPTSDIR}/boilerplate ${filtfile} | 			      \
      sed -e "s/@To@/${toaddr}/"					      \
	  -e "s/@From@/${FROMADDR}/"					      \
	  -e "s/@ReplyTo@/${REPLYTOADDR}/"				      \
	  -e "s/@Subject@/${subject}/"					      \
      > ${msgfile}
    ${POST} ${msgfile}

#    rm -f ${filtfile} ${msgfile}
}

#
# init_log_header
#
# arguments should be obvious...
#
init_log_header()
{
    local outfile cfg host env dir now sinfo 
    outfile="$1"
    cfg="$2"
    host="$3"
    env="$4"
    dir="$5"
    now=`date "+%Y-%m-%d %H:%M:%S %Z"`
    sinfo=`ssh -n ${host} 'uname -s -r'`
    cat >${outfile} <<EOF
-------------------------------------------------------------------------------
Configuration:		${cfg}
Host:			${host}
Environment:		${env}
Build directory:	${dir}
Start time:		${now}
System Info:		${sinfo}
-------------------------------------------------------------------------------

EOF
}

#
# extoll - send success message 
#
# $1 = Host message relevant to
# $2 = Subject
# $3 = log file
#
extoll()
{
    filter_and_post_log "$1" "$2" "${REPORTOKADDR}" "${3}"
}

#
# harp - send fail message 
#
# $1 = Host message relevant to
# $2 = Subject
# $3 = log file
#
harp()
{
    filter_and_post_log "$1" "$2" "${REPORTBADADDR}" "${3}"
}

run_tinderbox() {
    #
    # Copy and build latest code on tinderbox machines
    #
    for cfg in ${TINDERBOX_CONFIGS} ; do
	eval cfg_host=\$host_$cfg
	eval cfg_home=\$home_$cfg
	eval cfg_env=\$env_$cfg

	errfile="${LOGDIR}/0/${cfg_host}-${cfg}"
	header="${errfile}.header"

	cat /dev/null > ${errfile}
	if [ -z "${cfg_host}" ] ; then
	    echo "Configuration \"$cfg\" has no host." > ${errfile}
	    harp "${cfg}" "Misconfiguration" "${errfile}"
	    continue
	fi

	if [ -z "${cfg_home}" ] ; then
	    echo "Configuration \"$cfg\" has no directory." > ${errfile}
	    harp "${cfg}" "Misconfiguration" "${err_file}"
	    continue
	fi

	echo "Remote copy ${cfg_host} ${cfg_home}" > ${errfile}
	${SCRIPTSDIR}/remote_xorp_copy.sh ${cfg_host} ${cfg_home} >>${errfile} 2>&1
	if [ $? -ne 0 ] ; then
	    harp "${HOSTNAME}" "remote copy failed to ${cfg_host}" "${errfile}"
	    continue
	fi

	init_log_header "${header}" "${cfg}" "${cfg_host}" "${cfg_env}" "${cfg_home}" 

	cp ${header} ${errfile}
	ssh -n ${cfg_host} "env ${cfg_env} ${cfg_home}/scripts/build_xorp.sh " >>${errfile} 2>&1
	if [ $? -ne 0 ] ; then
	    harp ${cfg} "remote build failed" "${errfile}"
	    continue
	fi

	cp ${header} ${errfile}
	ssh -n ${cfg_host} "env ${cfg_env} ${cfg_home}/scripts/build_xorp.sh check" >>${errfile} 2>&1
	if [ $? -ne 0 ] ; then
	    harp ${cfg} "remote check failed" "${errfile}"
	    continue
	fi

	cp ${header} ${errfile}
	echo "No problems to report." >> ${errfile}
	extoll "${cfg}" "build and test good" "${errfile}"
    done
}

checkout() {
    local errfile
    errfile="$1"

    cat /dev/null > ${errfile}

    #
    # Checkout latest code
    #
    ${SCRIPTSDIR}/co_xorp.sh >${errfile} 2>&1
    if [ $? -ne 0 ] ; then
	harp "$LOCALHOST: checkout failed" "${errfile}"
	exit
    fi
}

roll_over_logs()
{
    log_history=10
    rm -f ${LOGDIR}/${log_history}
    log=${log_history}
    while [ ${log} -ne 0 ] ; do
	next_log=`expr ${log} - 1`
	if [ -d ${LOGDIR}/${next_log} ] ; then
	    mv ${LOGDIR}/${next_log} ${LOGDIR}/${log}
	fi
	log=${next_log}
    done
}

roll_over_logs   
mkdir -p ${LOGDIR}/0

checkout "${LOGDIR}/0/checkout.log"
run_tinderbox

