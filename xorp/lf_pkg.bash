#!/bin/bash
# Script for packaging up a xorp tarball for install in
# /usr/local/

rm -fr /usr/local/xorp

DBG="-dbg"
SCONS_ARGS="enable_olsr=true strip=no"
if [ -f obj/*gnu*/.scons_build_args ]
then
   SCONS_ARGS=`cat obj/*gnu*/.scons_build_args`
   SCONS_ARGS=$SCONS_ARGS strip=no
fi
#scons enable_olsr=true strip=no install
echo "Scons args: $SCONS_ARGS"
echo -n 3
sleep 1
echo -n " 2"
sleep 1
echo " 1"
sleep 1

# Disable GCC colors
GCC_COLORS=
export GCC_COLORS

scons -j8 $SCONS_ARGS || exit 1
scons -j8 $SCONS_ARGS install || exit 1
cp xorp_install.bash /usr/local/xorp/
chmod a+x /usr/local/xorp/xorp_install.bash

PWD=$(pwd)
# The line below will not work on Fedora 19, maybe elsewhere.
#userdir=$(expr match "$PWD" '\(/home/[0-z]*/\).*')
reg='(/home/[0-z]*/).*'
[[ $PWD =~ $reg ]] && userdir=${BASH_REMATCH[1]}

if [ "_$userdir" == "_" ]
then
    echo "Could not find user-dir, pwd: $PWD  Will use ./ for user-dir"
    userdir="./"
fi

cd /usr/local
if [ "$1_" != "nostrip_" ]
then
	echo "Stripping files in lf_pkg.bash"
	find xorp -name "*" -print|xargs strip
	DBG=
fi

if [ ! -d ${userdir}tmp ]
then
    echo "Creating directory: ${userdir}tmp to hold xorp package."
    mkdir -p ${userdir}tmp
fi

if uname -a|grep -e i386 -e armv7l
then
    if tar -cvzf ${userdir}tmp/xorp_32${DBG}.tgz xorp
    then
	echo ""
	echo "Created package:  ${userdir}tmp/xorp_32${DBG}.tgz"
	echo ""
    else
	echo ""
	echo "ERROR:  There were errors trying to create: ${userdir}tmp/xorp_32${DBG}.tgz"
	echo ""
    fi
else
    if tar -cvzf ${userdir}tmp/xorp_64${DBG}.tgz xorp
    then
	echo ""
	echo "Created package:  ${userdir}tmp/xorp_64${DBG}.tgz"
	echo ""
    else
	echo ""
	echo "ERROR:  There were errors trying to create: ${userdir}tmp/xorp_64${DBG}.tgz"
	echo ""
    fi
fi

cd -
