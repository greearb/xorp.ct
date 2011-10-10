#!/bin/bash

# Usage:  cd /usr/local/xorp; ./xorp_install.bash
#
# This will create the xorp user, attempt to fix up the
# group configure properly, and also attempt to soft-link
# some libraries to help binaries compiled on other OS versions
# to work on the target systems.

# Xorp should be un-tarred in /usr/local
if [ `pwd` != "/usr/local/xorp" ]
    then
    echo ""
    echo "ERROR:  You must un-tar xorp.tgz in /usr/local so that the files"
    echo "  are placed in /usr/local/xorp, and this script must be run from"
    echo "  the /usr/local/xorp directory."
    exit 1
fi

# Add xorp user and group

#Check for adduser commands
adduserArgs=""
if adduser -h 2>&1 | grep ".--system" > /dev/null 2>&1
then
        adduserArgs="$adduserArgs --system"
fi

if adduser -h 2>&1 | grep ".--no-create-home" > /dev/null 2>&1
then
        adduserArgs="$adduserArgs --no-create-home"
fi

echo "Creating xorp user and adding xorp to xorp and root groups..."
adduser $adduserArgs xorp
usermod -a -G xorp xorp
usermod -a -G xorp root
usermod -a -G xorp lanforge
usermod -a -G root xorp

# Xorp may have been compiled on older systems and thus have dependencies
# on older libraries.  Often, we can just link to newer ones and things
# work fine.
for i in `ldd ./sbin/* ./lib/xorp/sbin/*|grep "not found"|cut -f1 -d" "|sort|uniq`
    do
    echo "Missing library: $i"
    extension=(`expr match "$i" '.*\([0-9]\)'`);
    filename=${i%$extension}
    #echo "base: $filename  extension: $extension"
    while [ $extension -lt 20 ]
        do
        #echo "extension: $extension"
	let extension++
        if uname -a|grep x86_64 > /dev/null 2>&1
	    then
	    if [ -f /lib64/$filename$extension ]
		then
		echo "Linking: /lib64/$filename$extension to /lib64/$i"
		ln -s /lib64/$filename$extension /lib64/$i
		break
	    fi
	    if [ -f /usr/lib64/$filename$extension ]
		then
		echo "Linking: /usr/lib64/$filename$extension to /usr/lib64/$i"
		ln -s /usr/lib64/$filename$extension /usr/lib64/$i
		break
	    fi
	else
	    if [ -f /lib/$filename$extension ]
		then
		echo "Linking: /lib/$filename$extension to /lib/$i"
		ln -s /lib/$filename$extension /lib/$i
		break
	    fi
	    if [ -f /usr/lib/$filename$extension ]
		then
		echo "Linking: /usr/lib/$filename$extension to /usr/lib/$i"
		ln -s /usr/lib/$filename$extension /usr/lib/$i
		break
	    fi
	fi
    done
done


echo "Completed installation setup for Xorp, testing library linkage...."

if /usr/local/xorp/sbin/xorpsh -h > /dev/null 2>&1
    then
    echo ""
    echo "SUCCESS:  Xorpsh seems to work fine..."
else
    echo ""
    echo "ERROR:  xorpsh returned error code.  Please contact support."
    echo "  You are probably missing some libraries, perhaps older versions"
    echo "  of ones existing on your system.  The command:"
    echo "  ldd /usr/local/xorp/sbin/xorpsh"
    echo "  might help you figure out which ones are missing."
fi

  
