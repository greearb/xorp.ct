#!/bin/bash

# Xorp should be un-tarred in /usr/local
if [ `pwd` != "/usr/local/xorp" ]
    then
    echo "ERROR:  You must un-tar xorp.tgz in /usr/local so that the files are placed in /usr/local/xorp"
    exit 1
fi

# Add xorp user and group
echo "Creating xorp user and adding xorp to xorp and root groups..."
adduser xorp
usermod -a -G xorp xorp
usermod -a -G xorp root
usermod -a -G xorp lanforge
usermod -a -G root xorp

# I compile xorp on FC5, and it has a hard dependency on libpcap.0.9.4 and libcrypto.so.6
# Fake out the link here so that it will start OK.  This works on Ubuntu 8.0.4, no idea
# if it will work elsewhere. --Ben
if [ ! -f /usr/lib/libpcap.so.0.9 ]
    then
    for i in `ls /usr/lib/libpcap.so.0.9.*`
    do
      echo "Attempting to soft-link /usr/lib/libpcap.so.9 to a similar library ($i)."
      ln -s $i /usr/lib/libpcap.so.0.9
      break
    done
fi

if [ ! -f /usr/lib/libpcap.so.0.9.4 ]
    then
    echo "Attempting to soft-link /usr/lib/libpcap.so.9.4 to a similar library...."
    ln -s /usr/lib/libpcap.so.0.9.* /usr/lib/libpcap.so.0.9.4
fi

if [ -d /usr/lib64 ]
then
  if [ ! -f /usr/lib64/libpcap.so.0.9.4 ]
      then
      echo "Attempting to soft-link /usr/lib64/libpcap.so.9.4 to a similar library...."
      ln -s /usr/lib64/libpcap.so.0.9.* /usr/lib64/libpcap.so.0.9.4
  fi
fi


if [ ! -f /lib/libcrypto.so.6 ]
    then
    if [ -f /lib/libcrypto.so.7 ]
	then
	echo "Attempting to soft-link /lib/libcrypto.so.6 to a similar library...."
	ln -s /lib/libcrypto.so.7 /lib/libcrypto.so.6
    else
	echo "WARNING:  Can't figure out how to link libcrypto.so.6 properly, contact support"
	echo "  if xorp fails to work (it's possible libcrypto is elsewhere on your system.)"
    fi
fi

echo "Completed installation setup for Xorp, testing library linkage...."

if /usr/local/xorp/bin/xorpsh -h > /dev/null 2>&1
    then
    echo "Seems to work fine..."
else
    echo "ERROR:  xorpsh returned error code.  Please contact support."
fi
