#!/bin/bash
# Script for packaging up a xorp tarball for install in
# /usr/local/

rm -fr /usr/local/xorp

scons strip=yes shared=no build=mingw32 STRIP=i686-pc-mingw32-strip \
         CC=i686-pc-mingw32-gcc CXX=i686-pc-mingw32-g++ \
	 RANLIB=i686-pc-mingw32-ranlib \
	 AR=i686-pc-mingw32-ar LD=i686-pc-mingw32-ld install

# Copy some run-time libraries to the xorp dir for packaging
cp /usr/i686-pc-mingw32/sys-root/mingw/bin/libcrypto-10.dll /usr/local/xorp/sbin/
cp /usr/i686-pc-mingw32/sys-root/mingw/bin/zlib1.dll /usr/local/xorp/sbin/
cp /usr/i686-pc-mingw32/sys-root/mingw/bin/libgcc_s_sjlj-1.dll /usr/local/xorp/sbin/
cp /usr/i686-pc-mingw32/sys-root/mingw/bin/libgnurx-0.dll /usr/local/xorp/sbin/


PWD=$(pwd)
userdir=$(expr match "$PWD" '\(/home/[0-Z]*/\).*')

if [ "_$userdir" == "_" ]
then
    userdir = "./"
fi

cd /usr/local

if [ ! -d ${userdir}tmp ]
then
    echo "Creating directory: ${userdir}tmp to hold xorp package."
    mkdir -p ${userdir}tmp
fi

if zip -9 ${userdir}tmp/xorp_win32.zip -r xorp
then
    echo ""
    echo "Created package:  ${userdir}tmp/xorp_win32.zip"
    echo ""
else
    echo ""
    echo "ERROR:  There were errors trying to create: ${userdir}tmp/xorp_win32.zip"
    echo ""
fi

cd -
