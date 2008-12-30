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

# $XORP: other/tinderbox/scripts/tinderbox.sh,v 1.26 2008/07/23 04:42:55 pavlin Exp $

CONFIG="$(dirname $0)/config"
. ${CONFIG}

filter_and_post_log()
{
    local subject toaddr errfile filtfile msgfile

    d=$(date "+%y%m%d-%H%M")
    subject="[$d] - $1"
    toaddr="$2"
    errfile="$3"

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
    ssh_flags="$6"
    now=$(date "+%Y-%m-%d %H:%M:%S %Z")
    sinfo=$(ssh ${ssh_flags} -n ${host} 'uname -s -r')
    cat >${outfile} <<EOF
-------------------------------------------------------------------------------
Configuration:    ${cfg}
Host:             ${host}
SSH flags:        ${ssh_flags}
Environment:      ${env}
Build directory:  ${dir}
Start time:       ${now}
System Info:      ${sinfo}
-------------------------------------------------------------------------------

EOF
}

#
# extoll - send success message
#
# $1 = Config message relevant to + message
# $2 = log file
#
extoll()
{
    filter_and_post_log "PASS: $1" "${REPORTOKADDR}" "${2}"
}

#
# harp - send fail message
#
# $1 = Message relevant to
# $2 = log file
#
harp()
{
    filter_and_post_log "FAIL: $1" "${REPORTBADADDR}" "${2}"
}

run_tinderbox() {
    #
    # Copy and build latest code on tinderbox machines
    #
    for cfg in ${TINDERBOX_CONFIGS} ; do
	eval cfg_host=\$host_$cfg
	eval cfg_home=\$home_$cfg
	eval cfg_env=\$env_$cfg
	eval cfg_buildflags=\$buildflags_$cfg
	eval cfg_sshflags=\$sshflags_$cfg
	eval cfg_cross_arch=\$cross_arch_$cfg
	eval cfg_cross_root=\$cross_root_$cfg

	# Add the cross-compilation environment
	if [ "${cfg_cross_arch}" ] ; then
	    tmp_cross_root_arch="${cfg_cross_root}/${cfg_cross_arch}"
	    tmp_cross_bin_prefix="${cfg_cross_root}/bin/${cfg_cross_arch}"

	    cfg_env="${cfg_env} CC=${tmp_cross_bin_prefix}-gcc"
	    cfg_env="${cfg_env} CXX=${tmp_cross_bin_prefix}-g++"
	    cfg_env="${cfg_env} LD=${tmp_cross_bin_prefix}-ld"
	    cfg_env="${cfg_env} RANLIB=${tmp_cross_bin_prefix}-ranlib"
	    cfg_env="${cfg_env} NM=${tmp_cross_bin_prefix}-nm"

	    tmp_conf_args="--host=${cfg_cross_arch} --with-openssl=${tmp_cross_root_arch}/usr/local/ssl"
	    cfg_env="${cfg_env} CONFIGURE_ARGS='${tmp_conf_args}'"
	fi

	# Add the common environment
	cfg_env="${cfg_env} ${COMMON_ENV}"

	errfile="${LOGDIR}/0/${cfg_host}-${cfg}"
	header="${errfile}.header"

	init_log_header "${header}" "${cfg}" "${cfg_host}" "${cfg_env}" "${cfg_home}" "${cfg_sshflags}"
	cp ${header} ${errfile}
 
	if [ -z "${cfg_host}" ] ; then
	    echo "FAIL: Configuration \"$cfg\" has no host." >> ${errfile}
	    harp "${cfg} configuration" "${errfile}"
	    continue
	fi

	if [ -z "${cfg_home}" ] ; then
	    echo "FAIL: Configuration \"$cfg\" has no directory." >> ${errfile}
	    harp "${cfg} configuration" "${err_file}"
	    continue
	fi

	echo "Remote copy ${cfg_host} ${cfg_home} ${cfg_sshflags}" >> ${errfile}
	${SCRIPTSDIR}/remote_xorp_copy.sh "${cfg_host}" "${cfg_home}" "${cfg_sshflags}" >> ${errfile} 2>&1
	if [ $? -ne 0 ] ; then
	    harp "${cfg} remote copy" "${errfile}"
	    continue
	fi

	# Log in to host and build xorp
	build_errfile="${errfile}-build"
	cp ${header} ${build_errfile}
	ssh ${cfg_sshflags} -n ${cfg_host} "env ${cfg_env} ${cfg_home}/scripts/build_xorp.sh ${cfg_buildflags}" >>${build_errfile} 2>&1
	if [ $? -ne 0 ] ; then
	    harp "${cfg} remote build" "${build_errfile}"
	    continue
	fi

	# Kill tinderbox XORP processes that may be lying around from
	# earlier runs.
	ssh ${cfg_sshflags} -n ${cfg_host}	"killall -r 'xorp_*'" 2>/dev/null

	# Log into host and run regression tests only if this is not
	# cross-compilation setup
	if [ ! "${cfg_cross_arch}" ] ; then
	    check_errfile="${errfile}-check"
	    cp ${header} ${check_errfile}
	    ssh ${cfg_sshflags} -n ${cfg_host} "env ${cfg_env} ${cfg_home}/scripts/build_xorp.sh check" >>${check_errfile} 2>&1
	    if [ $? -ne 0 ] ; then
		harp "${cfg} remote check" "${check_errfile}"
		continue
	    fi
	fi

	cp ${header} ${errfile}
	echo "No problems to report." >> ${errfile}
	extoll "${cfg}" "${errfile}"
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
	harp "$LOCALHOST checkout" "${errfile}"
	exit
    fi
}

roll_over_logs()
{
    log_history=15
    rm -rf ${LOGDIR}/${log_history}
    log=${log_history}
    while [ ${log} -ne 0 ] ; do
	next_log=$((${log} - 1))
	if [ -d ${LOGDIR}/${next_log} ] ; then
	    mv ${LOGDIR}/${next_log} ${LOGDIR}/${log}
	fi
	log=${next_log}
    done
}

#
# Implement some simple locking to make sure we don't run two
# instances of tinderbox.sh from cron at the same time.
#
tinderbox_lockfile="${LOGDIR}/tinderbox.lock"
if [ -f "${tinderbox_lockfile}" ]; then
	echo "Tinderbox was invoked whilst already running."
	exit 255
fi

touch ${tinderbox_lockfile}

roll_over_logs
mkdir -p ${LOGDIR}/0

checkout "${LOGDIR}/0/checkout"
run_tinderbox

rm -f ${tinderbox_lockfile}
