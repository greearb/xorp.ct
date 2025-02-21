#!/bin/bash
# Script for packaging up a xorp tarball for install in
# /usr/local/

# Assume it is already built.

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
