#!/bin/sh -
# Copyright 2001 2002 Edson Brandi <ebrandi.home@uol.com.br>
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


#This code was originally derived from the FreeBSD Live CD 

# Check UID
if [ "`id -u`" != "0" ]; then
    echo "Sorry, this must be done as root."
    exit 1
fi

DIALOG=${DIALOG=/usr/bin/dialog}
done_p="[ ]"
done_g="[ ]"
done_d="[ ]"
done_cl="[ ]"
done_k="[ ]"
done_ki="[ ]"
done_x="[ ]"
done_xi="[ ]"
done_e="[ ]"
done_c="[ ]"
done_i="[ ]"
done_f="[ ]"
done_q="[ ]"

# This function will create the directories structure that LiveCD will need
# They will all be created under $CHROOTDIR
prepare_directories() {

    # check if $CHROOTDIR exists, if so, take schg flag out away from 
    # executable files and remove the directories.
    if [ -r $CHROOTDIR ]; then
        chflags -R noschg $CHROOTDIR/
        rm -rf $CHROOTDIR/
    fi

    # Creates the directory that will be LiveCD's root 
    mkdir -p $CHROOTDIR 

    # Creates LiveCD related directories

    mtree -deU -f $LIVEDIR/files/LIVECD.raiz.dist -p $CHROOTDIR >> $LIVEDIR/log
    #mkdir /var/spool/clientmqueue

    done_p="[*]"
    # Shows dialog information box about the directories creation.
    dialog --title "XORP LiveCD" --msgbox "The necessary folders were created." 5 60
}

# warning routines
aviso() {
    dialog --title "XORP LiveCD" --infobox "The compilation process failed, please see log file at \n $LIVEDIR/log" -1 -1
    echo "The compilation process was aborted ..."
    exit 1
}

check_prerequisites() {
    if [ -n "$CHECKP" ]; then
         return
    fi

    CHECKP="done"

    dialog --title "XORP LiveCD" --infobox "Checking prerequisites for LIVE CD creation..." 5 60
    sleep 1
    if [ ! -d "/usr/src" ]; then
	dialog --title "XORP LiveCD" --msgbox "/usr/src does not exist on this machine." 5 60
	exit 1
    fi

    if [ ! -d "/usr/ports" ]; then
	dialog --title "XORP LiveCD" --msgbox "/usr/ports does not exist on this machine." 5 60
	exit 1
    fi
    
    if [ ! -d "/usr/ports/lang/python" ]; then
	dialog --title "XORP LiveCD" --msgbox "/usr/ports/lang/python does not exist on this machine." 5 60
	exit 1
    fi
    
    if [ ! -d "$XORPSRCDIR" ]; then
	dialog --title "XORP LiveCD" --msgbox "XORP source directory $XORPSRCDIR does not exist." 5 60
	exit 1
    fi
    
    if [ ! -f "$KERNELDIR/GENERIC" ]; then
	dialog --title "XORP LiveCD" --msgbox "kernel config file $KERNELDIR/GENERIC does not exist." 5 70
	exit 1
    fi
    
    dialog --title "XORP LiveCD" --msgbox "Finished checking prerequisites.  Looks good." 5 60
    cd $LIVEDIR
}

# This function makes buildworld
run_buildworld() {

# Shows dialog asking for buildworld confirmation. If the user does not confirm
# we will return execution to main_dialog.
dialog --title "XORP LiveCD" --yesno "The entire \"make buildworld\" process can take from 2 to 4 hours on on a fast Pentium III class machine. Do you want to continue?" 6 70 

case $? in
     1)
     dialog --title "XORP LiveCD" --msgbox "Buildworld aborted" 5 60
     ;;
     255)
     echo "The compilation process was aborted ..."
     exit 1 
     ;;
     0)
     # Makes buildworld, creating logs under $LIVEDIR/log
     dialog --title "XORP LiveCD" --infobox "running make buildworld..." 5 60
     cd /usr/src && make buildworld >> $LIVEDIR/log || aviso

     #Build python
     dialog --title "XORP LiveCD" --infobox "building python..." 5 60
     cd /usr/ports/lang/python || aviso
     make clean >> $LIVEDIR/log
     make LOCALBASE=/usr/local DESTDIR=/usr/tmp/live_root NO_PKG_REGISTER=true >>  $LIVEDIR/log || aviso

     # Shows dialog telling the user that the buildworld process was done.
     dialog --title "XORP LiveCD" --msgbox "Binary files done." 5 60
     done_b="[*]"
     cd $LIVEDIR
     ;;
esac
}

# This function distributes binaries under $CHROOTDIR
install_binaries() {

    dialog --title "XORP LiveCD" --infobox "creating directories..." 5 60

    cd /usr/src/etc && make distrib-dirs DESTDIR=$CHROOTDIR >> $LIVEDIR/log || aviso
    cd /usr/src/etc && make distribution DESTDIR=$CHROOTDIR >> $LIVEDIR/log || aviso
    
    dialog --title "XORP LiveCD" --infobox "Installing /etc/resolv.conf" 5 60

    # Copies /etc/resolv.conf to /etc under LiveCD ;-) 
    if [ -f /etc/resolv.conf ]; then 
	cp -p /etc/resolv.conf $CHROOTDIR/etc
    fi

    dialog --title "XORP LiveCD" --infobox "cd /usr/src..." 5 60
    cd /usr/src/ || aviso
    dialog --title "XORP LiveCD" --infobox "Running make installworld..." 5 60
    make installworld DESTDIR=$CHROOTDIR >> $LIVEDIR/log || aviso

    mkdir $CHROOTDIR/bootstrap
    cp -p $CHROOTDIR/sbin/mount $CHROOTDIR/bootstrap
    cp -p $CHROOTDIR/sbin/umount $CHROOTDIR/bootstrap

    dialog --title "XORP LiveCD" --infobox "Rebuilding sysinstall..." 5 60

    # rebuilds sysinstall
    cd /usr/src/release/sysinstall/
    make clean >> $LIVEDIR/log
    make >> $LIVEDIR/log
    make install >> $LIVEDIR/log

    dialog --title "XORP LiveCD" --infobox "Installing /stand..." 5 60

    # copies /stand to LiveCD's root
    tar -cf - -C /stand . | tar xpf - -C $CHROOTDIR/stand/

    mkdir  $CHROOTDIR/scripts/lang
    cp  $LIVEDIR/lang/*  $CHROOTDIR/scripts/lang
    rm  $CHROOTDIR/scripts/lang/livecd_*

    #Install python
    dialog --title "XORP LiveCD" --infobox "Installing python..." 5 60
    cd /usr/ports/lang/python || aviso
    make clean >> $LIVEDIR/log
    make install LOCALBASE=/usr/local DESTDIR=/usr/tmp/live_root NO_PKG_REGISTER=true >>  $LIVEDIR/log || aviso

    # Tells the user that the whole process was done ;-)
    dialog --title "XORP LiveCD" --msgbox "The binary files were distributed under $CHROOTDIR" 5 60
    done_d="[*]"
    cd $LIVEDIR
}

# This function distributes binaries under $CHROOTDIR
cleanup_binaries() {

    dialog --title "XORP LiveCD" --infobox "Deleting files not needed by XORP..." 5 60
    cd $CHROOTDIR
    rm -rf usr/bin/gcc
    rm -rf usr/bin/cc
    rm -rf usr/bin/g++
    rm -rf usr/bin/c++
    rm -rf usr/bin/vacation
    rm -rf usr/bin/talk
    rm -rf usr/sbin/lpd
    rm -rf usr/sbin/mailwrapper
    rm -rf usr/sbin/mixer
    rm -rf usr/sbin/moused
    rm -rf usr/sbin/mrinfo
    rm -rf usr/sbin/mrouted
    rm -rf usr/sbin/burncd
    rm -rf usr/sbin/cdcontrol
    rm -rf usr/sbin/dtmfdecode
    rm -rf usr/sbin/fontedit
    rm -rf usr/sbin/map-mbone
    rm -rf usr/sbin/pkg_*
    rm -rf usr/sbin/sendmail
    rm -rf usr/sbin/spray
    rm -rf usr/sbin/xten
    rm -rf usr/share/games
    rm -rf usr/share/groff_font
    rm -rf usr/share/sendmail
    rm -rf usr/libexec/cc*
    rm -rf usr/libexec/comsat
    rm -rf usr/libexec/fingerd
    rm -rf usr/libexec/ftpd
    rm -rf usr/libexec/locate*
    rm -rf usr/libexec/lpr
    rm -rf usr/libexec/rexecd
    rm -rf usr/libexec/rlogind
    rm -rf usr/libexec/rpc.*
    rm -rf usr/libexec/rshd
    rm -rf usr/libexec/sendmail
    rm -rf usr/libexec/sftp-server
    rm -rf usr/libexec/tftpd
    rm -rf usr/libexec/xtend
    rm -rf usr/libexec/smrsh
    rm -rf usr/libexec/uucp
    rm -rf usr/libexec/uucpd
    rm -rf usr/libexec/rexecd
    rm -rf usr/libexec/rexecd

# Tells the user that the whole process was done ;-)
    dialog --title "XORP LiveCD" --msgbox "Files not needed by XORP were removed from $CHROOTDIR" 5 60
    done_cl="[*]"

    cd $LIVEDIR
}

# Now, we will patch GENERIC accordingly to our changes and build LIVECD from it
build_kernel() {

    dialog --title "XORP LiveCD" --infobox "Deleting compile dir..." 5 60
    rm -rf $COMPILEDIR >> $LIVEDIR/log

# Copies GENERIC to LIVECD
    dialog --title "XORP LiveCD" --infobox "Copying generic..." 5 60
    cp $KERNELDIR/GENERIC $KERNELDIR/LIVECD >> $LIVEDIR/log && \

# Patch LIVECD up
    dialog --title "XORP LiveCD" --infobox "Patching kern config file..." 5 60
    cd $KERNELDIR && patch -p < $LIVEDIR/files/patch_generic  >> $LIVEDIR/log && \

# Lets build the kernel 
    dialog --title "XORP LiveCD" --infobox "Doing the kernel build..." 5 60
    config LIVECD  >> $LIVEDIR/log && cd $COMPILEDIR && make depend  >> $LIVEDIR/log && \
	make  >> $LIVEDIR/log && make install DESTDIR=$CHROOTDIR  >> $LIVEDIR/log || aviso

# Tells the user that this process was done.
    dialog --title "XORP LiveCD" --msgbox "kernel build complete" 5 60
    done_k="[*]"
    cd $LIVEDIR

}

# Install a kernel we built earlier
install_kernel() {

    dialog --title "XORP LiveCD" --infobox "Doing the kernel install (build if needed)..." 5 60
    cd $COMPILEDIR && make depend  >> $LIVEDIR/log && \
	make  >> $LIVEDIR/log && make install DESTDIR=$CHROOTDIR  >> $LIVEDIR/log || aviso

    cp $LIVEDIR/files/loader.conf $CHROOTDIR/boot/
    cp $LIVEDIR/files/splash.bmp $CHROOTDIR/boot/

# Tells the user that this process was done.
    dialog --title "XORP LiveCD" --msgbox "The new kernel is at $CHROOTDIR" 5 60

    done_ki="[*]"
    cd $LIVEDIR

}

# Now, we will build all the XORP binaries
build_xorp() {

    dialog --title "XORP LiveCD" --infobox "Doing XORP build..." 5 60
    sleep 1
    dialog --title "XORP LiveCD" --infobox "Doing cd to $XORPSRCDIR..." 5 60
    echo "attempting to cd to $XORPSRCDIR" >> $LIVEDIR/log
    cd $XORPSRCDIR || aviso
    dialog --title "XORP LiveCD" --infobox "Running XORP configure..." 5 60
    echo "running configure" >> $LIVEDIR/log
    ./configure >> $LIVEDIR/log || aviso
    dialog --title "XORP LiveCD" --infobox "Running gmake..." 5 60
    echo "running gmake" >> $LIVEDIR/log
    gmake >> $LIVEDIR/log || aviso

# Tells the user that this process was done.
    dialog --title "XORP LiveCD" --msgbox "XORP build in $XORPSRCDIR is complete" 5 60

    done_x="[*]"
    cd $LIVEDIR

}

# Now, we will install all the XORP binaries, templates, and xif files
install_xorp() {

    dialog --title "XORP LiveCD" --infobox "Doing XORP install..." 5 60
    echo "Doing XORP install"  >> $LIVEDIR/log
    cd $XORPSRCDIR || aviso
    gmake install prefix=$CHROOTDIR/usr/local/xorp >> $LIVEDIR/log || aviso
    dialog --title "XORP LiveCD" --infobox "Doing XORP install... done" 5 60

    cp $LIVEDIR/files/xorp_load.py $CHROOTDIR/usr/local/xorp/bin

# Tells the user that this process was done.
    dialog --title "XORP LiveCD" --msgbox "XORP binaries, templates, and XIF files were installed in $CHROOTDIR" 5 60

    done_xi="[*]"
    cd $LIVEDIR

}

# Now we will patch all changed files under /etc
patch_etc() {

    dialog --title "XORP LiveCD" --infobox "Patching etc..." 5 60
    cd $CHROOTDIR/etc && patch -p < $LIVEDIR/files/patch_rc || aviso
    rm -f $CHROOTDIR/etc/rc.orig

    dialog --title "XORP LiveCD" --infobox "Adding xorp user..." 5 60
    #Add a XORP user and group
    cd $CHROOTDIR/etc && patch -p < $LIVEDIR/files/patch_master.passwd || aviso
    rm -f $CHROOTDIR/etc/master.passwd.orig
    cd $CHROOTDIR/etc && patch -p < $LIVEDIR/files/patch_group || aviso
    rm -f $CHROOTDIR/etc/group.orig

    #rebuild the password DB to include the XORP user
    /usr/sbin/pwd_mkdb -p -d $CHROOTDIR/etc $CHROOTDIR/etc/master.passwd

    dialog --title "XORP LiveCD" --infobox "Copying in files..." 5 60
    cp $LIVEDIR/files/rc.live $CHROOTDIR/etc/rc.live

    cp $LIVEDIR/files/rc.conf $CHROOTDIR/etc
    cp $LIVEDIR/files/fstab   $CHROOTDIR/etc
    cp $LIVEDIR/files/motd   $CHROOTDIR/etc

    mkdir -p $CHROOTDIR/usr/local/etc/rc.d
    cp $LIVEDIR/files/xorp.sh   $CHROOTDIR/usr/local/etc/rc.d


    dialog --title "XORP LiveCD" --infobox "Building devices..." 5 60
    cd $CHROOTDIR/dev

    # We will need some devices if we intend to have virtual nodes, right?
    # so lets make them ;-)
    for i in 0 1 2 3 4 5 6 7 8 9
      do
      ./MAKEDEV vn$i
    done;

    # Tell the user that /etc files were patched
    dialog --title "XORP LiveCD" --msgbox "The $CHROOTDIR/etc files were changed." 5 60
    done_e="[*]"

    cd $LIVEDIR

}

# This function generates ISOs and files to populate MFS
create_iso() {

    dialog --title "XORP LiveCD" --infobox "Creating the ISO in $LIVEISODIR/LiveCD.iso" 5 60
    # Lets create the .tgz files that will be used to mount MFS
    cd $CHROOTDIR
    tar cvzfp mfs/etc.tgz etc >> $LIVEDIR/log
    tar cvzfp mfs/dev.tgz dev >> $LIVEDIR/log
    tar cvzfp mfs/root.tgz root >> $LIVEDIR/log
    tar cvzfp mfs/local_etc.tgz usr/local/etc >> $LIVEDIR/log

    # Copies all the necessary files to make a bootable CD
    cp $LIVEDIR/files/boot.catalog $CHROOTDIR/boot >> $LIVEDIR/log
    cd $CHROOTDIR

    # Now we make a Bootable ISO without emulating floppy 2.8 Mb boot style.
    if [ -f /usr/local/bin/mkisofs ] ; then

        /usr/local/bin/mkisofs -b boot/cdboot -no-emul-boot -c boot/boot.catalog  -r -J -h -V LiveCD -o $LIVEISODIR/LiveCD.iso . >> $LIVEDIR/log || aviso
	dialog --title "XORP LiveCD" --msgbox "Creation process done." 5 60

    elif then
        
	dialog --title "XORP LiveCD" --msgbox "mkisofs not found, installing from ports!" 5 75
	#
	# If mkisofs does not exist, we will install it ;-)
	cd /usr/ports/sysutils/mkisofs
	make >> $LIVEDIR/log
	make install >> $LIVEDIR/log
	make clean >> $LIVEDIR/log
	#
	dialog --title "XORP LiveCD" --msgbox "mkisofs instaled" 5 75
	#
	# makes ISO image.
	/usr/local/bin/mkisofs -b boot/cdboot -no-emul-boot -c boot/boot.catalog  -r -J -h -V LiveCD -o $LIVEISODIR/LiveCD.iso . >> $LIVEDIR/log || aviso
	dialog --title "XORP LiveCD" --msgbox "Creation process done." 5 60

    fi

    # If ISO image was successfully created, we tell the user it was done.
    if [ -f $LIVEISODIR/LiveCD.iso ]; then
	dialog --title "XORP LiveCD" --msgbox "File $LIVEISODIR/LiveCD.iso created." 5 60
    fi
    
    done_i="[*]"
    cd $LIVEDIR

}

# Burns the CD Image.
burn_cd() {

    burncd -f $CDRW -s 8 -e data $LIVEISODIR/LiveCD.iso fixate >> $LIVEDIR/log || aviso
    dialog --title "XORP LiveCD" --msgbox "Burning process done." 5 60

    done_f="[*]"
    cd $LIVEDIR

}

# Let's check if user has already confirm'd his options on config file

configure_livecd() {
    $DIALOG --title "XORP LiveCD" --clear --inputbox "Which directory are you running livecd.sh from?" 8 70 "`pwd`" 2> /tmp/input.tmp.$$
    retval=$?

    LIVEDIR=`cat /tmp/input.tmp.$$`
    rm /tmp/input.tmp.$$

    # We will TRAP to allow the user to abort the process...
    case $retval in
	0)
        echo "LIVEDIR=$LIVEDIR" > ./config
        ;;
        1)
        echo "Cancel pressed."
        exit 1
        ;;
       255)
       echo "ESC pressed."
       exit 1
       ;;
    esac

    $DIALOG --title "XORP LiveCD" --clear --inputbox "Which directory do you want to create LiveCD's / in?" 8 70 "/usr/tmp/live_root" 2>/tmp/input.tmp.$$
    retval=$?

    CHROOTDIR=`cat /tmp/input.tmp.$$`
    rm /tmp/input.tmp.$$

    # Trap again ;)
    case $retval in
	0)
	echo "CHROOTDIR=$CHROOTDIR" >> ./config
	;;
	1)
	echo "Cancel pressed."
	exit 1
	;;
	255)
	echo "ESC pressed."
	exit 1
	;;
    esac

    $DIALOG --title "XORP LiveCD" --clear --inputbox "Which directory do you want to create LiveCD.iso in?" 8 70 "/usr/tmp" 2>/tmp/input.tmp.$$
    retval=$?

    LIVEISODIR=`cat /tmp/input.tmp.$$`
    rm /tmp/input.tmp.$$

# One more Trap, with the same purpose.
    case $retval in
	0)
	echo "LIVEISODIR=$LIVEISODIR" >> ./config
	;;
	1)
	echo "Cancel pressed."
	exit 1
	;;
	255)
	echo "ESC pressed."
	exit 1
	;;
    esac

    $DIALOG --title "XORP LiveCD" --clear --inputbox "Which directory contains the XORP source tree?" 8 70 "/usr/local/xorp/xorp" 2>/tmp/input.tmp.$$
    retval=$?

    XORPSRCDIR=`cat /tmp/input.tmp.$$`
    rm /tmp/input.tmp.$$

# One more Trap, with the same purpose.
    case $retval in
	0)
	echo "XORPSRCDIR=$XORPSRCDIR" >> ./config
	;;
	1)
	echo "Cancel pressed."
	exit 1
	;;
	255)
	echo "ESC pressed."
	exit 1
	;;
    esac

    $DIALOG --title "XORP LiveCD" --clear --yesno "Do you have a CDR/CDRW writer installed in this machine?" 8 70 

    case $? in
	0)
	$DIALOG --title "XORP LiveCD" --clear --inputbox "What is the CDR/CDRW device?" 8 70 "/dev/acd0c" 2>/tmp/input.tmp.$$
	retval=$?

	CDRW=`cat /tmp/input.tmp.$$`
	rm /tmp/input.tmp.$$

     # Trap to allow user to abort process...
	case $retval in
	    0)
	    echo "CDRW=$CDRW" >> ./config
	    ;;
	    1)
	    echo "Cancel pressed."
	    exit 1
	    ;;
	    255)
	    echo "ESC pressed."
	    exit 1 
	    ;;
	esac 
	;;
	1)
	echo "CDRW=/dev/null" >> ./config
	;;
	255)
	echo "ESC pressed."
	exit 1
	;;
    esac

    echo "KERNELDIR=/usr/src/sys/i386/conf" >> ./config
    echo "COMPILEDIR=/usr/src/sys/compile/LIVECD" >> ./config

     . ./config
     verifica_config
}

# checks for config file
verifica_config() {

    if [ -f ./config ] ; then
	. ./config
    fi

    dialog --title "XORP LiveCD" --yesno " You set the $LIVEDIR/config variables as:\n\n LIVEISODIR=$LIVEISODIR \n CHROOTDIR=$CHROOTDIR \n CDRW=$CDRW \n LIVEDIR=$LIVEDIR \n XORPSRCDIR=$XORPSRCDIR \n\n Are you sure you want to continue with the \n LiveCD creation process?" 18 70 || exit 0
    touch ./config.ok


}

# Lists the packages that are actually installed at the local B0X and asks the
# user if he wants to install any of those packages.
packages() {

    DIALOG=${DIALOG=/usr/bin/dialog}

# Now, we create packages.sh that will be used to list all available applications
# as a reference to the user.

    echo "$DIALOG --title \"XORP LiveCD - Packages\" --clear \\" > $LIVEDIR/packages.sh
    echo "--checklist \"These are the packages instaled in your FreeBSD machine. \\n\\" >> $LIVEDIR/packages.sh
    echo "Choose the packages you wanna add in the LiveCD\" -1 -1 10 \\" >> $LIVEDIR/packages.sh
    for i in `ls -1 /var/db/pkg`; do echo "\"$i\" \"\" off \\" >> $LIVEDIR/packages.sh ; done
    echo "2> /tmp/checklist.tmp.\$\$" >> $LIVEDIR/packages.sh
    echo "retval=\$?" >> $LIVEDIR/packages.sh
    echo "choice=\`cat /tmp/checklist.tmp.\$\$\`" >> $LIVEDIR/packages.sh
    echo "rm -f /tmp/checklist.tmp.\$\$" >> $LIVEDIR/packages.sh
    echo "case \$retval in" >> $LIVEDIR/packages.sh
    echo "  0)" >> $LIVEDIR/packages.sh
    echo "for i in \`echo \$choice | sed -e 's/\"/#/g'\`; do pkg_create -b \`echo \$i | awk -F\"#\" '{print \$2}'\`; done ;;" >> $LIVEDIR/packages.sh
    echo "  1)" >> $LIVEDIR/packages.sh
    echo "echo  \"Cancel pressed.\";;" >> $LIVEDIR/packages.sh
    echo "  255)" >> $LIVEDIR/packages.sh
    echo "[ -z \"\$choice\" ] || echo \$choice ;" >> $LIVEDIR/packages.sh
    echo "echo \"ESC pressed.\";;" >> $LIVEDIR/packages.sh
    echo "esac" >> $LIVEDIR/packages.sh
    echo $LIVEDIR/packages.sh

    # Runs packages.sh
    /bin/sh $LIVEDIR/packages.sh

    # So, we install the packages that were chose by the User ;-)
    for i in `ls -1 *.tgz`; do /usr/sbin/pkg_add -vRf -p $CHROOTDIR/usr/local $i >> $LIVEDIR/packages.log ; done

    # Checks for First Level dependencies on the package, and install them.
    cat $LIVEDIR/packages.log | grep depends | awk -F"\`" '{print $3}' | awk -F"'" '{print $1}' > $LIVEDIR/tmp_
    for i in `cat $LIVEDIR/tmp_ `; do /usr/sbin/pkg_create -b $i ; done
    for i in `cat $LIVEDIR/tmp_ `; do /usr/sbin/pkg_add -vRf -p $CHROOTDIR/usr/local $i.tgz >> $LIVEDIR/packages2.log; done

    # Checks for Second Level dependencies on the package, and install them.
    # Second Level dependencies are dependecies' dependencies.
    cat $LIVEDIR/packages2.log | grep depends | awk -F"\`" '{print $3}' | awk -F"'" '{print $1}' > $LIVEDIR/tmp2_
    for i in `cat $LIVEDIR/tmp2_ `; do /usr/sbin/pkg_create -b $i ; done
    for i in `cat $LIVEDIR/tmp2_ `; do /usr/sbin/pkg_add -Rf -p $CHROOTDIR/usr/local $i.tgz; done

    # Removes temporary files ;)
    rm $LIVEDIR/packages.log $LIVEDIR/packages2.log
    rm  $LIVEDIR/packages.sh
    rm $LIVEDIR/tmp_ $LIVEDIR/tmp2_
    rm  $LIVEDIR/*.tgz

    # Tells the user that the selected packages were installed.
    dialog --title "XORP LiveCD" --msgbox "The selected packages were instaled on LiveCD's / partition" 5 70
    done_c="[*]"
}

# This is the Main Dialog menu :)
main_dialog() {

# exports config file's defined variables
    if [ -f ./config ] ; then
	. ./config
    fi


# Defines the temporary file
    tempfile=`/usr/bin/mktemp -t checklist`

    check_prerequisites

    # Makes main dialog menu...
    log "main_dialog()"
    dialog --menu "XORP LiveCD Tool Set" 21 70 14 \
	1 "$done_p Create folders (mkdir)" \
	2 "$done_g Create binary files (buildworld)" \
	3 "$done_d Install binary files (installworld)" \
	4 "$done_cl Remove unneeded binary files (cleanworld)" \
	5 "$done_k Create kernel (buildkernel)" \
	6 "$done_ki Install a previously built kernel (installkernel)" \
	7 "$done_x Create XORP binaries (buildxorp)" \
	8 "$done_xi Install XORP binaries (installxorp)" \
	9 "$done_e Fix up /etc (patch's)" \
	10 "$done_c Install packages on LiveCD (Customize)" \
	11 "$done_i Create ISO (mkisofs)" \
	12 "$done_f Burn CD" \
	13 "$done_q Exit" \
	2> $tempfile 
    
    opcao=`cat $tempfile`

    case ${opcao} in
	1)
        prepare_directories
        ;;
        2)
	run_buildworld
	;;
	3)
	install_binaries
	;;
	4)
	cleanup_binaries
	;;
	5)
	build_kernel
	;;
	6)
	install_kernel
	;;
	7)
	build_xorp
	;;
	8)
	install_xorp
	;;
	9)
	patch_etc
	;;
	10)
	packages
	;;
	11)
	create_iso 
	;;
	12)
	burn_cd
	;;
	13)
	rm $tempfile
	rm $LIVEDIR/config.ok
	rm /tmp/opcao_*
	exit 0
	;;
	*) 
	exit 0
	;;
    esac
}

# Makes the Loop :) 
while true
  do
  if [ -f ./config.ok ] ; then
      main_dialog
  elif then
      configure_livecd
  fi
done
