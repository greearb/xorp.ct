#!/bin/sh
 

DEV=/dev/fd0
MNT_PNT=/mnt/floppy
FD_BASE=
RF_BASE=/
RF_BACKUP=/var/tmp
LOCK_FILE=/var/run/floppy_file.lck
LOG_FILE=/var/log/floppy_mount.log
FILE_LIST="/etc/passwd:644 /etc/xorp.cfg:644 /etc/group:644"
CFG_FILE=/etc/xorp.cfg

discover_interfaces() {

	echo "interfaces {" > $CFG_FILE

	for i in `ifconfig -a | awk -F : '$2~/flags=/ {print $1}'`
	do
      		echo "    interface $i {" >> $CFG_FILE
		if [ "${i}" != "lo0" ]; then
		    echo "        description: \"detected interface\"" >> $CFG_FILE
		else
		    echo "        description: \"loopback interface\"" >> $CFG_FILE
		fi
       		echo "    }"  >> $CFG_FILE
       		echo " ">> $CFG_FILE
	done

	echo "}" >> $CFG_FILE

	echo  "fea {" >> $CFG_FILE
	echo  "    enable-unicast-forwarding4: true" >> $CFG_FILE
	echo "}" >> $CFG_FILE

}

copy_files_in () {
	/usr/local/xorp/bin/xorp_load.py -mode loadin $MNT_PNT/manifest.dat
}

copy_files_back() {
	/usr/local/xorp/bin/xorp_load.py -mode saveout $MNT_PNT/manifest.dat
}

unmount_fd() {
	
	echo -n "Mounting floppy disk... "
	#umount it first, just in case it got left mounted....
	umount /mnt/floppy 1>/dev/null 2>/dev/null
	mount -t msdos $DEV $MNT_PNT 1>$LOG_FILE 2>$LOG_FILE
	if [ $? -eq 0 ]; then 
	    	echo "OK"
	    	copy_files_back
	    	sync
    	    	echo -n "Unmounting floppy disk... "
	    	umount $MNT_PNT 1>$LOG_FILE 2>$LOG_FILE
	    	
		if [ $? -eq 0 ]; then 
	    		echo "OK"
		else
	    		echo "FAIL"
		fi
      	fi
}

mount_fd() {
	
	echo -n "Mounting floppy disk... "
	#umount it first, just in case it got left mounted....
	umount /mnt/floppy 1>/dev/null 2>/dev/null
	mount -t msdos $DEV $MNT_PNT 1>$LOG_FILE 2>$LOG_FILE
	if [ $? -eq 0 ]; then 
	    	echo "OK"
	    	copy_files_in
	    	sync
    	    	echo -n "Unmounting floppy disk... "
	    	umount $MNT_PNT 1>$LOG_FILE 2>$LOG_FILE
	    	
		if [ $? -eq 0 ]; then 
	    		echo "OK"
		else
	    		echo "FAIL"
		fi
	else
	    echo "FAILED to load any config from floppy, creating config."
	    discover_interfaces
      	fi
}

start_services() {
    if [ -r /etc/defaults/rc.conf ]; then
        . /etc/defaults/rc.conf
        source_rc_confs
    elif [ -r /etc/rc.conf ]; then
        . /etc/rc.conf
    fi

    case ${sshd_enable} in
    [Yy][Ee][Ss])
        if [ -x ${sshd_program:-/usr/sbin/sshd} ]; then
            echo -n ' sshd';
            ${sshd_program:-/usr/sbin/sshd} ${sshd_flags}
        fi
        ;;
    esac
}


start() {
        /bin/sh /usr/local/xorp/bin/xorp-makeconfig.sh
	echo "Loading XORP config"
       	mount_fd

	echo "Starting services"
	start_services

	echo "Starting XORP router manager process"
	/usr/local/xorp/bin/xorp_rtrmgr -b $CFG_FILE 1>/dev/null 2>/dev/null &
	exit 0
}

stop() {
	echo "Syncing back XORP to floppy"
	killall xorp_rtrmgr 
	unmount_fd
	exit 0
}

reload() {
	echo "Reloading doesn't make sense, don't do it"
	exit 0
}

case "$1" in
start)
        start
        ;;
stop)
        stop
        ;;
*)
        echo "Usage: `basename $0` {start|stop}" >&2
        ;;
esac

exit 0
