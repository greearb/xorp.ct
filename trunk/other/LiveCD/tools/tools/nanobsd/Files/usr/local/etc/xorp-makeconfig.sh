#!/bin/sh -

#
# $XORP: other/LiveCD/tools/tools/nanobsd/Files/usr/local/etc/xorp-makeconfig.sh,v 1.3 2008/09/22 20:10:35 bms Exp $
#

# Copyright (c) 2004-2008 XORP, Inc.
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

USEUSB="true"

if [ -f /liveusb_marker ]; then
    IS_USB_BOOT="true"
else
    IS_USB_BOOT="false"
fi

if [ ${IS_USB_BOOT} = "true" ] ; then
    # We're booting from a USB key. Use the embedded config slice.
    TITLE="XORP LiveUSB"
    USBDEV="/dev/da0"
    USB_CFG_SLICE="${USBDEV}s3"
    CFG_DIR="xorp.cfg"
else
    # We're booting from a CD. The USB key holds the config.
    TITLE="XORP LiveCD"
    USBDEV="/dev/da0"
    USB_CFG_SLICE="${USBDEV}s1"
    CFG_DIR="xorp.cfg"
fi

DIALOG="/usr/bin/dialog"
CFG_MOUNTPOINT="/cfg"
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
NEWFS_MSDOS="/sbin/newfs_msdos"
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
    ${DIALOG} --title "${TITLE}" --infobox "${TITLE} is starting..." 6 70 
}

# Don't call this routine if booting from a USB key, as we use ufs
# to store config data there.
test_usb_cfg_device() {
    tempfile=`${MKTEMP} -t testflp` || exit 1
    
    #force an unmount in case it's already mounted.
    ${UMOUNT} ${USB_CFG_SLICE} 1> /dev/null 2> /dev/null

    #mount the USB disk
    ${TOUCH} ${tempfile} 
    ${MOUNTMSDOSFS} -l -g xorp -m 775 ${USB_CFG_SLICE} ${CFG_MOUNTPOINT} 1>> ${tempfile} 2>> ${tempfile}
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
		test_usb_cfg_device
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
        ${DIALOG} --title "${TITLE}" --infobox "Trying again to mount USB disk ${USB_CFG_SLICE} on ${CFG_MOUNTPOINT} ..." 6 70 
	mount_retries=$((${mount_retries} + 1))
	test_usb_cfg_device
        return
        ;;
        2)
	USEUSB="false"
	return
	;;
        3)
	format_usb_cfg_device
        ${DIALOG} --title "${TITLE}" --infobox "Trying again to mount USB disk ${USB_CFG_SLICE} on ${CFG_MOUNTPOINT} ..." 6 70 
	test_usb_cfg_device
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
        ${DIALOG} --title "${TITLE}" --infobox "Trying again to mount USB disk ${USB_CFG_SLICE} on ${CFG_MOUNTPOINT} ..." 6 70 
	test_usb_cfg_device
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
	${UMOUNT} ${USB_CFG_SLICE} >/dev/null 2>&1 || true
	${USBLOAD}
	${MOUNTMSDOSFS} -l -g xorp -m 775 ${USB_CFG_SLICE} ${CFG_MOUNTPOINT} 1>> ${tempfile} 2>> ${tempfile}
    fi
}

format_usb_cfg_device() {
    ${DIALOG} --title "${TITLE}" --yesno "Reformat the USB drive?" 6 70 
    case $? in
	0)
        ${DIALOG} --title "${TITLE}" --infobox "Partitioning ${USBDEV}..." 5 60
	${FDISK} -I ${USBDEV}
	# Subvert the '-I' option; turn FreeBSD into FAT32
	${FDISK} -p ${USBDEV} | sed 's/0xa5/0x0e/' | ${FDISK} -f - ${USBDEV}

        ${DIALOG} --title "${TITLE}" --infobox "Adding DOS filesystem to ${USB_CFG_SLICE}..." 5 60
	${NEWFS_MSDOS} ${USB_CFG_SLICE}
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
	    ${UMOUNT} ${USB_CFG_SLICE} >/dev/null 2>&1 || true
	    ${USBLOAD}
	    if [ ${IS_USB_BOOT} = "false" ] ; then
		${MOUNTMSDOSFS} -l -g xorp -m 775 ${USB_CFG_SLICE} ${CFG_MOUNTPOINT} 1>> ${tempfile} 2>> ${tempfile}
	    fi
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
	${UMOUNT} ${USB_CFG_SLICE} >/dev/null 2>&1 || true
	${USBSAVE}
	if [ ${IS_USB_BOOT} = "false" ] ; then
	    ${MOUNTMSDOSFS} -l -g xorp -m 775 ${USB_CFG_SLICE} ${CFG_MOUNTPOINT} 1>> ${tempfile} 2>> ${tempfile}
	fi
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

# If we are not booting off USB itself, then the configuration
# device is on separate storage, and we must test for it.
# Otherwise, we must force a mount for test_marker.
if [ ${IS_USB_BOOT} = "false" ] ; then
    test_usb_cfg_device
else
    ${MOUNT} ${USB_CFG_SLICE} >/dev/null 2>&1 || true
fi

test_marker

# XXX ensure unmounted.
${UMOUNT} ${USB_CFG_SLICE} >/dev/null 2>&1 || true
