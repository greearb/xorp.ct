#!/bin/sh -
# Copyright 2004 Mark Handley <mjh@xorp.org>
# All Rights Reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY EDSON BRANDI ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT EDSON BRANDI BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
# OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
# BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
# USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
# DAMAGE.


DIALOG="/usr/bin/dialog"
MNTDIR="/mnt/floppy"
TMPDIR="/tmp"
FLOPPYNUM="fd0"
FLOPPYDEV="/dev/${FLOPPYNUM}"
MANIFEST="${MNTDIR}/manifest.dat"
ETCDIR="/etc"
MNTETC="${MNTDIR}/ETC"
PASSWDSRC="${MNTDIR}/ETC/MPASSWD.TXT"
PASSWDDST="${ETCDIR}/master.passwd"
GROUPSRC="${MNTDIR}/ETC/GROUP.TXT"
GROUPDST="${ETCDIR}/group"
XORPCFGSRC="${MNTDIR}/ETC/XORP.CFG"
XORPCFGDST="${ETCDIR}/xorp.cfg"
SSHDSASRC="${MNTDIR}/ETC/SSHDSA.TXT"
SSHDSADST="${ETCDIR}/ssh/ssh_host_dsa_key"
SSHRSASRC="${MNTDIR}/ETC/SSHRSA.TXT"
SSHRSADST="${ETCDIR}/ssh/ssh_host_rsa_key"
SSHV1SRC="${MNTDIR}/ETC/SSHV1.TXT"
SSHV1DST="${ETCDIR}/ssh/ssh_host_key"
USEFLOPPY="true"
TITLE="XORP LiveCD"

#this runs during the boot process, so we need to be pretty robust to
#path errors and so forth
MOUNT="/sbin/mount"
UMOUNT="/sbin/umount"
MKTEMP="/usr/bin/mktemp"
TOUCH="/usr/bin/touch"
CAT="/bin/cat"
DMESG="/sbin/dmesg"
GREP="/usr/bin/grep"
REBOOT="/sbin/reboot"
NEWFSMSDOS="/sbin/newfs_msdos"
FDFORMAT="/usr/sbin/fdformat"
PASSWD="/usr/bin/passwd"
CP="/bin/cp"
MKDIR="/bin/mkdir"
IFCONFIG="/sbin/ifconfig"
AWK="/usr/bin/awk"
RM="/bin/rm"
CHMOD="/bin/chmod"
CHOWN="/usr/sbin/chown"

welcome() {
    if [ -z ${TERM} ]; then
	TERM="cons25"; export TERM
    else
	echo ${TERM}
    fi 
    ${DIALOG} --title "${TITLE}" --infobox "XORP LiveCD is starting..." 6 70 
}

test_floppy() {
    tempfile=`${MKTEMP} -t testflp` || exit 1
    
    #force an unmount in case it's already mounted.
    ${UMOUNT} ${FLOPPYDEV} 1> /dev/null 2> /dev/null

    #mount the floppy
    ${TOUCH} ${tempfile} 
    ${MOUNT} -t msdos ${FLOPPYDEV} ${MNTDIR} 1>> ${tempfile} 2>> ${tempfile}
    if [ $? -ne 0 ]; then
	err=`${CAT} ${tempfile}`
	${DMESG} | grep ${FLOPPYNUM} 1>> /dev/null
	if [ $? -ne 0 ]; then
	    ${DIALOG} --title "${TITLE}" --msgbox \
"This machine does not appear to have a floppy drive installed.\n\n\
This means you will be unable to save configuration files when \n\n\
using XORP, but you can still configure the router while it is \n\n\
running." 10 70
            USEFLOPPY="false"
            return
        fi
	echo "here"
        ${DIALOG} --title "${TITLE}" --msgbox \
"XORP failed to mount the floppy drive.  The error message was:\n\n\
${err}\n\n\
This may be because there was no floppy in the drive, because the\n\
floppy was damaged, or because the floppy is not DOS formatted." 12 70
        ${DIALOG} --title "${TITLE}" --menu "What do you want to do about the floppy drive?" 11 70 4 \
	1 "Try again to mount the floppy." \
	2 "Continue without the ability to save files." \
	3 "Attempt to reformat the floppy." \
	4 "Reboot the machine." \
	2> $tempfile 
	
	choice=`cat $tempfile`

	case ${choice} in
	1)
        ${DIALOG} --title "${TITLE}" --infobox "Trying again to mount floppy drive ${FLOPPYDEV} on ${MNTDIR} ..." 6 70 
        test_floppy
        return
        ;;
        2)
	USEFLOPPY="false"
	return
	;;
        3)
	format_floppy
        ${DIALOG} --title "${TITLE}" --infobox "Trying again to mount floppy drive ${FLOPPYDEV} on ${MNTDIR} ..." 6 70 
        test_floppy
        return
	;;
        4)
	${REBOOT}
	exit 1
	;;
	esac
    fi
    
    #at this point the floppy should be mounted
    ${DIALOG} --title "${TITLE}" --infobox "Testing we can write to the floppy..." 6 70 
    rwtest=${MNTDIR}/rwtest
    writable="true"
    ${RM} -f ${rwtest}
    if [ -e ${rwtest} ]; then 
	writable="false"
    else 
        ${TOUCH} ${MNTDIR}/rwtest
        if [ ! -e ${rwtest} ]; then 
	    writable="false"
	fi
    fi
    if [ ${writable} == "false" ]; then
	${DIAGLOG} --title "${TITLE}" --menu "XORP does not appear to have permission to write to the floppy.\n\
Perhaps the floppy is write protected?\n\n\What do you want to do about the floppy drive?" 11 70 4 \
	1 "Try again to mount the floppy." \
	2 "Continue without the ability to save files." \
	3 "Reboot the machine." \
	2> $tempfile 
	
	choice=`cat $tempfile`

	case ${choice} in
	1)
        ${DIALOG} --title "${TITLE}" --infobox "Trying again to mount floppy drive ${FLOPPYDEV} on ${MNTDIR} ..." 6 70 
        test_floppy
        return
        ;;
        2)
	USEFLOPPY="false"
	return
	;;
        3)
	${REBOOT}
	exit 1
	;;
	esac
    fi
    ${RM} -f ${rwtest}
    ${DIALOG} --title "${TITLE}" --infobox "Floppy drive test succeeded." 6 70 
    USEFLOPPY="true"
}

format_floppy() {
    ${DIALOG} --title "${TITLE}" --yesno "Reformat the floppy drive?" 6 70 
    case $? in
	0)
        ${DIALOG} --title "${TITLE}" --infobox "Formatting ${FLOPPYDEV}..." 5 60
	${FDFORMAT} -y ${FLOPPYDEV}
	if [ $? -ne 0 ]; then
	    ${DIALOG} --title "${TITLE}" --msgbox "Low level format failed." 5 60
	    return
	fi
        ${DIALOG} --title "${TITLE}" --infobox "Adding DOS filesystem to ${FLOPPYDEV}..." 5 60
	${NEWFSMSDOS} -f 1440 -L XORP ${FLOPPYDEV}
	if [ $? -ne 0 ]; then
	    ${DIALOG} --title "${TITLE}" --msgbox "Failed to add DOS filesystem" 5 60
	    return
	fi
	FORMATDONE="true"
	${DIALOG} --title "${TITLE}" --msgbox "Format completed successfully" 5 60
	return
        ;;
        1)
        ${DIALOG} --title "${TITLE}" --infobox "Formatting aborted" 5 60
        return
        ;;
    esac
}

test_manifest() {
    if [ ${USEFLOPPY} != "false" ]; then
	if [ -r ${MANIFEST} ]; then
   	    #everything is fine
	    ${DIALOG} --title "${TITLE}" --infobox "Manifest file found.\n\nXORP is starting." 5 60
	    return
	fi
	if [ ${FORMATDONE} = "true" -o ${USEFLOPPY} = "false" ]; then
	    ${DIALOG} --title "${TITLE}" --msgbox "The next steps are to create a xorp config file and set passwords." 5 70    
	else
	    ${DIALOG} --title "${TITLE}" --msgbox "The floppy does not contain a XORP file manifest.\n\
We will need to create a xorp config file and set passwords." 7 70    
	fi
    else
	${DIALOG} --title "${TITLE}" --msgbox \
"Even though we can't save the config for next time, we still \n\
need to create an initial config.  The next steps are to create\n\
a xorp config file and set passwords." 8 70    
    fi
    ${DIALOG} --title "${TITLE}" --infobox "Enter the root password." 5 70    
    ${PASSWD}
    ${DIALOG} --title "${TITLE}" --infobox "Enter the password for the \"xorp\" router user account." 5 70    
    ${PASSWD} -l xorp

    
    if [ ${USEFLOPPY} != "false" ]; then
	${TOUCH} ${MANIFEST}
	if [ ! -d ${MNTETC} ]; then
	    ${MKDIR} -p ${MNTETC} 2> ${tempfile}
	    if [ $? -ne 0 ]; then
		err=`${CAT} ${tempfile}`
		${DIALOG} --title "${TITLE}" --msgbox "Failed to create directory ${MNTETC}\n${err}" 7 70    
	    fi
	fi
	${CP} ${PASSWDDST} ${PASSWDSRC} 2> ${tempfile}
	if [ $? -ne 0 ]; then
	    err=`${CAT} ${tempfile}`
	    ${DIALOG} --title "${TITLE}" --msgbox "Failed to copy ${PASSWDDST} to ${PASSWDSRC}\n${err}" 7 70    
	    NOPASSWD="true"
	else
	    echo "${PASSWDSRC} ${PASSWDDST} 600 root wheel 0" >> ${MANIFEST} 
	fi

	
	#Preserve the ssh host keys which were auto-generated
	if [ -r ${SSHV1DST} ]; then
	    ${CP} ${SSHV1DST} ${SSHV1SRC} 2> ${tempfile}
	    if [ $? -ne 0 ]; then
		err=`${CAT} ${tempfile}`
		${DIALOG} --title "${TITLE}" --msgbox "Failed to copy ${SSHV1DST} to ${SSHV1SRC}\n${err}" 7 70    
	    else
		echo "${SSHV1SRC} ${SSHV1DST} 600 root wheel 0" >> ${MANIFEST} 
	    fi
	fi
	if [ -r ${SSHDSADST} ]; then
	    ${CP} ${SSHDSADST} ${SSHDSASRC} 2> ${tempfile}
	    if [ $? -ne 0 ]; then
		err=`${CAT} ${tempfile}`
		${DIALOG} --title "${TITLE}" --msgbox "Failed to copy ${SSHDSADST} to ${SSHDSASRC}\n${err}" 7 70    
	    else
		echo "${SSHDSASRC} ${SSHDSADST} 600 root wheel 0" >> ${MANIFEST} 
	    fi
	fi
	if [ -r ${SSHRSADST} ]; then
	    ${CP} ${SSHRSADST} ${SSHRSASRC} 2> ${tempfile}
	    if [ $? -ne 0 ]; then
		err=`${CAT} ${tempfile}`
		${DIALOG} --title "${TITLE}" --msgbox "Failed to copy ${SSHRSADST} to ${SSHRSASRC}\n${err}" 7 70    
	    else
		echo "${SSHRSASRC} ${SSHRSADST} 600 root wheel 0" >> ${MANIFEST} 
	    fi
	fi



    fi

    create_config
    #finished creating manifest file

    ${DIALOG} --title "${TITLE}" --msgbox "Configuration is complete." 5 70    
}

getiftype() {
    tempfile=$1
    media="??"
    ${GREP} Ethernet ${tempfile} 1> /dev/null
    if [ $? -eq 0 ]; then
	media="Ethernet"
    fi
    ${GREP} LOOPBACK ${tempfile} 1> /dev/null
    if [ $? -eq 0 ]; then
	media="Loopback interface"
    fi
    ${GREP} faith ${tempfile} 1> /dev/null
    if [ $? -eq 0 ]; then
	media="IPv6-to-IPv4 TCP relay"
    fi
    ${GREP} ppp ${tempfile} 1> /dev/null
    if [ $? -eq 0 ]; then
	media="Point-to-point protocol"
    fi
    ${GREP} sl0 ${tempfile} 1> /dev/null
    if [ $? -eq 0 ]; then
	media="Serial-line IP"
    fi
    echo ${media} > ${tempfile}
}

create_config() {
    tempfile=`${MKTEMP} -t testflp` || exit 1
    cmdfile=`${MKTEMP} -t iface` || exit 1
    resfile=`${MKTEMP} -t iface-res` || exit 1
    echo "$DIALOG --title \"${TITLE}\" --clear \\" > ${cmdfile}
    echo "--checklist \"These are the active network interfaces in your machine. \\n\\" >> ${cmdfile}
    echo "Choose the interfaces for XORP to manage.  If you don't\\n\\" >> ${cmdfile}
    echo "know what these are, the defaults are probably OK.\\n\\" \
	>> ${cmdfile}
    echo "\\nHit TAB to select OK when you're done.\" -1 -1 10 \\" \
	>> ${cmdfile}
    count=0
    iflist=`${IFCONFIG} -a | ${AWK} -F : '$2~/flags=/ {print $1}'`
    for i in $iflist
      do
      ${IFCONFIG} $i > ${tempfile}
      ${GREP} RUNNING ${tempfile} 1> /dev/null
      if [ $? -eq 0 ]; then
	  enabled="on"
      else
	  enabled="off"
      fi
      getiftype ${tempfile}
      media=" (`cat ${tempfile}`)"
      count=$((${count}+1))
      echo "\"${count}\" \"$i$media\" \"$enabled\" \\" >> $cmdfile
    done
    echo "2> ${resfile}" >> ${cmdfile}
    /bin/sh ${cmdfile}
    result=`${CAT} ${resfile}`

    count=0

    #don't know why this would exist on a MFS, but delete it if it did.
    if [ -e ${XORPCFGDST} ]; then
	${RM} -f ${XORPCFGDST}
    fi

    ${TOUCH} ${XORPCFGDST} 2> ${tempfile}
    if [ ! -e ${XORPCFGDST} ]; then
	err=`cat ${tempfile}`
	${DIALOG} --title "${TITLE}" --msgbox "Failed to create XORP config file ${XORPCFGDST}\n${err}" 7 70    
	return
    fi

    echo "/*XORP Configuration File*/" >> ${XORPCFGDST}
    echo "interfaces {" >> ${XORPCFGDST}
	
    for i in ${iflist}
    do
      count=$((${count}+1))
      k="\"${count}\""
      for j in ${result}
      do
	if [ $j = $k ]; then
	    ${IFCONFIG} $i > ${tempfile}
	    getiftype ${tempfile}
	    media="`cat ${tempfile}`"
	    echo "    interface $i {" >> ${XORPCFGDST}
	    echo "        description: \"$media\"" >> ${XORPCFGDST}
	    echo "        vif $i {" >> ${XORPCFGDST}
	    echo "        }" >> ${XORPCFGDST}
	    echo "    }" >> ${XORPCFGDST}
	fi
      done
    done
      
    echo "}" >> ${XORPCFGDST}
    ${CHOWN} xorp:xorp ${XORPCFGDST}
    ${CHMOD} 664 ${XORPCFGDST}
    if [ ${USEFLOPPY} != "false" ]; then
	${CP} ${XORPCFGDST} ${XORPCFGSRC} 2> ${tempfile}
	if [ $? -ne 0 ]; then
	    err=`${CAT} ${tempfile}`
	    ${DIALOG} --title "${TITLE}" --msgbox "Failed to copy ${SSHRSADST} to ${SSHRSASRC}\n${err}" 7 70    
	else
	    echo "${XORPCFGSRC} ${XORPCFGDST} 664 xorp xorp 0" >> ${MANIFEST} 
	fi
    fi
}

welcome
test_floppy
test_manifest
