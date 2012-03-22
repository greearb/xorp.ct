#!/bin/bash
# Script for building and packaging up a xorp windows binary

# This needs to be run as root (or via sudo) to have a good
# chance of working...

#  This works on modern Fedora ( on 2012-03-12: Fedora 16),
# with these pkgs installed:

#mingw32-gcc-4.6.1-3.fc16.i686
#mingw32-gcc-c++-4.6.1-3.fc16.i686
#mingw32-cpp-4.6.1-3.fc16.i686
#mingw32-filesystem-69-11.fc16.noarch
#mingw32-binutils-2.21-2.fc16.i686
#mingw32-runtime-3.18-4.fc16.noarch
#mingw32-w32api-3.17-1.fc16.noarch
#mingw32-pthreads-2.8.0-15.20110511cvs.fc16.noarch
#mingw32-zlib-1.2.5-5.fc16.noarch
#mingw32-openssl-1.0.0d-1.fc16.noarch
#mingw32-pdcurses-3.4-8.fc15.noarch
#mingw32-libgnurx-2.5.1-7.fc15.noarch


# This works on Fedora 13, with these pkgs installed:

#mingw32-pdcurses-3.4-8.fc15.noarch
#mingw32-cpp-4.5.3-1.fc15.i686
#mingw32-binutils-2.21-1.fc15.i686
#mingw32-w32api-3.15-2.fc15.noarch
#mingw32-openssl-1.0.0d-1.fc15.noarch
#mingw32-libgnurx-2.5.1-7.fc15.noarch
#mingw32-zlib-1.2.5-3.fc15.noarch
#mingw32-gcc-c++-4.5.3-1.fc15.i686
#mingw32-gcc-4.5.3-1.fc15.i686
#mingw32-runtime-3.15.2-5.fc13.noarch
#mingw32-pthreads-2.8.0-13.fc15.noarch
#mingw32-filesystem-69-3.fc15.noarch



# In addition, you need ( on 2012-03-12) the fix this bug:
#==
# . . . include/routprot.h:51: error:
# 'IP_LOCAL_BINDING' does not name a type
#==
# Bug listed in
#http://sourceforge.net/tracker/?func=detail&aid=3388721&group_id=2435&atid=102435
# or
#http://lists.fedoraproject.org/pipermail/mingw/2011-February/003442.html
#
# But first check "src/winsup/w32api/include/" on cygwin.com :
#http://cygwin.com/cgi-bin/cvsweb.cgi/src/winsup/w32api/include/?cvsroot=src
# or directly
#  "CVS log for src/winsup/w32api/include/routprot.h" on
#http://cygwin.com/cgi-bin/cvsweb.cgi/src/winsup/w32api/include/routprot.h?cvsroot=src
#

# mingw cross-compile arguments
SARGS="strip=yes shared=no build=mingw32 STRIP=i686-pc-mingw32-strip \
       CC=i686-pc-mingw32-gcc CXX=i686-pc-mingw32-g++ \
       RANLIB=i686-pc-mingw32-ranlib  AR=i686-pc-mingw32-ar \
       LD=i686-pc-mingw32-ld"

JNUM=4

if [ "$1 " != " " ]
    then
    JNUM=$1
fi

# Clean up any previous installed xorp code.
rm -fr /usr/local/xorp

# Build
echo "Building..."
scons -j$JNUM $SARGS || exit 1

if [ "$2 " == "b " ]
    then
    exit 0
fi

echo "Installing..."
scons $SARGS install || exit 2

echo "Copy some files..."
# Copy some run-time libraries to the xorp dir for packaging
cp /usr/i686-pc-mingw32/sys-root/mingw/bin/zlib1.dll /usr/local/xorp/sbin/ || exit 4
cp /usr/i686-pc-mingw32/sys-root/mingw/bin/libgcc_s_sjlj-1.dll /usr/local/xorp/sbin/ || exit 5
cp /usr/i686-pc-mingw32/sys-root/mingw/bin/libgnurx-0.dll /usr/local/xorp/sbin/ || exit 6
cp /usr/i686-pc-mingw32/sys-root/mingw/bin/libcrypto-10.dll /usr/local/xorp/sbin/ || exit 7
if [ -f /usr/i686-pc-mingw32/sys-root/mingw/bin/libstdc++-6.dll ]
    then
    cp /usr/i686-pc-mingw32/sys-root/mingw/bin/libstdc++-6.dll /usr/local/xorp/sbin/ || exit 8
fi


PWD=$(pwd)
userdir=$(expr match "$PWD" '\(/home/[0-Z]*/\).*')

if [ "_$userdir" == "_" ]
then
    userdir = "./"
fi

cd /usr/local || exit 3

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
