#!/bin/sh -

#
# $XORP$
#

# Copyright (c) 2004-2008 International Computer Science Institute
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# The names and trademarks of copyright holders may not be used in
# advertising or publicity pertaining to the software without specific
# prior permission. Title to copyright in this software and any associated
# documentation will at all times remain with the copyright holders.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

USEUSB="true"
TITLE="XORP LiveCD"

USBDEV="/dev/da0"
USBSLICE1="${USBDEV}s1"

DIALOG="/usr/bin/dialog"
CFG_MOUNTPOINT="/cfg"
CFG_DIR="xorp.cfg"
MARKERFILE="${CFG_MOUNTPOINT}/${CFG_DIR}/marker.dat"
ETCDIR="/etc"
XORPCFGDST="${ETCDIR}/local/xorp.conf"

#this runs during the boot process, so we need to be pretty robust to
#path errors and so forth
MOUNT="/sbin/mount"
MOUNTMSDOSFS="/sbin/mount_msdosfs"
UMOUNT="/sbin/umount"
MKTEMP="/usr/bin/mktemp"
TOUCH="/usr/bin/touch"
CAT="/bin/cat"
DMESG="/sbin/dmesg"
GREP="/usr/bin/grep"
REBOOT="/sbin/reboot"
NEWFSMSDOS="/sbin/newfs_msdos"
FDISK="/sbin/fdisk"
PASSWD="/usr/bin/passwd"
PW="/usr/sbin/pw"
CP="/bin/cp"
MKDIR="/bin/mkdir"
IFCONFIG="/sbin/ifconfig"
AWK="/usr/bin/awk"
RM="/bin/rm"
CHMOD="/bin/chmod"
CHOWN="/usr/sbin/chown"

USBLOAD="/sbin/usb_load"
USBSAVE="/sbin/usb_save"

mount_retries=0

welcome() {
    if [ -z ${TERM} ]; then
	TERM="cons25"; export TERM
    else
	echo ${TERM}
    fi 
    ${DIALOG} --title "${TITLE}" --infobox "XORP LiveCD is starting..." 6 70 
}

test_usb() {
    tempfile=`${MKTEMP} -t testflp` || exit 1
    
    #force an unmount in case it's already mounted.
    ${UMOUNT} ${USBSLICE1} 1> /dev/null 2> /dev/null

    #mount the USB disk
    ${TOUCH} ${tempfile} 
    ${MOUNTMSDOSFS} -l -g xorp -m 775 ${USBSLICE1} ${CFG_MOUNTPOINT} 1>> ${tempfile} 2>> ${tempfile}
    if [ $? -ne 0 ]; then
	err=`${CAT} ${tempfile}`
	${DMESG} | grep "umass0" 1>> /dev/null
	if [ $? -ne 0 ]; then
	    ${DIALOG} --title "${TITLE}" --yesno \
"This machine does not appear to have a USB disk inserted.\n\
Do you wish to retry mounting the USB disk?" 10 70
	    case $? in
	    0)
		mount_retries=$((${mount_retries} + 1))
		test_usb
		return
		;;
	    1)
		${DIALOG} --title "${TITLE}" --msgbox \
"You will be unable to save configuration files when using XORP,\n\
but you can still configure the router while it is \n\
running, or until a USB disk is inserted." 10 70
		USEUSB="false"
		return
		;;
	    esac
        fi
	echo "here"
        ${DIALOG} --title "${TITLE}" --msgbox \
"XORP failed to mount the USB disk.  The error message was:\n\n\
${err}\n\n\
This may be because there was no media present, because the\n\
USB disk was damaged, or because the USB disk is not DOS formatted." 12 70
        ${DIALOG} --title "${TITLE}" --menu "What do you want to do about the USB disk?" 11 70 4 \
	1 "Try again to mount the USB disk." \
	2 "Continue without the ability to save files." \
	3 "Attempt to reformat the USB disk." \
	4 "Reboot the machine." \
	2> $tempfile 
	
	choice=`cat $tempfile`

	case ${choice} in
	1)
        ${DIALOG} --title "${TITLE}" --infobox "Trying again to mount USB disk ${USBSLICE1} on ${CFG_MOUNTPOINT} ..." 6 70 
	mount_retries=$((${mount_retries} + 1))
        test_usb
        return
        ;;
        2)
	USEUSB="false"
	return
	;;
        3)
	format_usb
        ${DIALOG} --title "${TITLE}" --infobox "Trying again to mount USB disk ${USBSLICE1} on ${CFG_MOUNTPOINT} ..." 6 70 
        test_usb
        return
	;;
        4)
	${REBOOT}
	exit 1
	;;
	esac
    fi
    
    #at this point the USB disk should be mounted
    ${DIALOG} --title "${TITLE}" --infobox "Testing we can write to the USB disk..." 6 70 
    rwtest=${CFG_MOUNTPOINT}/rwtest
    writable="true"
    ${RM} -f ${rwtest}
    if [ -e ${rwtest} ]; then 
	writable="false"
    else 
        ${TOUCH} ${CFG_MOUNTPOINT}/rwtest
        if [ ! -e ${rwtest} ]; then 
	    writable="false"
	fi
    fi
    if [ ${writable} == "false" ]; then
	${DIAGLOG} --title "${TITLE}" --menu "XORP does not appear to have permission to write to the USB disk.\n\
Perhaps the USB disk is write protected?\n\n\What do you want to do about the USB disk?" 11 70 4 \
	1 "Try again to mount the USB disk." \
	2 "Continue without the ability to save files." \
	3 "Reboot the machine." \
	2> $tempfile 
	
	choice=`cat $tempfile`

	case ${choice} in
	1)
        ${DIALOG} --title "${TITLE}" --infobox "Trying again to mount USB disk ${USBSLICE1} on ${CFG_MOUNTPOINT} ..." 6 70 
        test_usb
        return
        ;;
        2)
	USEUSB="false"
	return
	;;
        3)
	${REBOOT}
	exit 1
	;;
	esac
    fi
    ${RM} -f ${rwtest}
    ${DIALOG} --title "${TITLE}" --infobox "USB disk test succeeded." 6 70 
    USEUSB="true"

    if [ ${mount_retries} -ne 0 ]; then
	${UMOUNT} ${USBSLICE1} >/dev/null 2>&1 || true
	${USBLOAD}
	${MOUNTMSDOSFS} -l -g xorp -m 775 ${USBSLICE1} ${CFG_MOUNTPOINT} 1>> ${tempfile} 2>> ${tempfile}
    fi
}

format_usb() {
    ${DIALOG} --title "${TITLE}" --yesno "Reformat the USB drive?" 6 70 
    case $? in
	0)
        ${DIALOG} --title "${TITLE}" --infobox "Partitioning ${USBDEV}..." 5 60
	${FDISK} -I ${USBDEV}
	# Subvert the '-I' option; turn FreeBSD into FAT32
	${FDISK} -p ${USBDEV} | sed 's/0xa5/0x0e/' | ${FDISK} -f - ${USBDEV}

        ${DIALOG} --title "${TITLE}" --infobox "Adding DOS filesystem to ${USBSLICE1}..." 5 60
	${NEWFS_MSDOS} ${USBSLICE1}
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

test_marker() {
    if [ ${USEUSB} != "false" ]; then
	# XXX temporary mount needed
	if [ -r ${MARKERFILE} ]; then
   	    #everything is fine
	    ${DIALOG} --title "${TITLE}" --infobox "XORP configuration found.\n\nXORP is starting." 5 60
	    # XXX no way of knowing if we got the mount at startup OK,
	    # or if this is a remount, unless we also copy the marker.
	    #if [ ${mount_retries} -ne 0 ]; then
	    ${UMOUNT} ${USBSLICE1} >/dev/null 2>&1 || true
	    ${USBLOAD}
	    ${MOUNTMSDOSFS} -l -g xorp -m 775 ${USBSLICE1} ${CFG_MOUNTPOINT} 1>> ${tempfile} 2>> ${tempfile}
	    #fi
	    exit 0
	fi
	if [ ${FORMATDONE} = "true" -o ${USEUSB} = "false" ]; then
	    ${DIALOG} --title "${TITLE}" --msgbox "The next steps are to create a XORP config file and set passwords." 5 70    
	else
	    ${DIALOG} --title "${TITLE}" --msgbox "The USB disk does not contain a XORP config marker file.\n\
We will need to create a XORP config file and set passwords." 7 70    
	fi
    else
	${DIALOG} --title "${TITLE}" --msgbox \
"Even though we can't save the config for next time, we still \n\
need to create an initial config.  The next steps are to create\n\
a XORP config file and set passwords." 8 70    
    fi
    ${DIALOG} --title "${TITLE}" --infobox "Enter the root password." 5 70    
    ${PASSWD}

    # XXX The XORP port creates a group 'xorp', but overzealous 'pw'
    # wants to clobber it. Tell it to use existing group with '-g' switch.
    ${PW} user add -n xorp -g xorp || true
    ${DIALOG} --title "${TITLE}" --infobox "Enter the password for the \"xorp\" router user account." 5 70    
    ${PASSWD} -l xorp

    create_config

    # XXX Unmount before calling USBSAVE.
    if [ ${USEUSB} != "false" ]; then
	${UMOUNT} ${USBSLICE1} >/dev/null 2>&1 || true
	${USBSAVE}
	${MOUNTMSDOSFS} -l -g xorp -m 775 ${USBSLICE1} ${CFG_MOUNTPOINT} 1>> ${tempfile} 2>> ${tempfile}
    fi

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

    # Backup old config if present.
    if [ ${USEUSB} != "false" ]; then
	${MV} ${XORPCFGDST} ${XORPCFGDST}.orig 2> ${tempfile}
    else
	${RM} -f ${XORPCFGDST} || true
    fi

    ${TOUCH} ${XORPCFGDST} 2> ${tempfile}
    if [ ! -e ${XORPCFGDST} ]; then
	err=`cat ${tempfile}`
	${DIALOG} --title "${TITLE}" --msgbox "Failed to create XORP config file ${XORPCFGDST}\n${err}" 7 70    
	return
    fi

    echo "/*XORP Configuration File*/" >> ${XORPCFGDST}

    # Interface block
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

    # We need *something* to run, so run fib2mrib.
    echo "protocols {" >> ${XORPCFGDST}
    echo " fib2mrib {" >> ${XORPCFGDST}
    echo "  disable: false" >> ${XORPCFGDST}
    echo " }" >> ${XORPCFGDST}
    echo "}" >> ${XORPCFGDST}

    ${CHOWN} xorp:xorp ${XORPCFGDST}
    ${CHMOD} 664 ${XORPCFGDST}
}

welcome
test_usb
test_marker

# XXX ensure unmounted.
${UMOUNT} ${USBSLICE1} >/dev/null 2>&1 || true
