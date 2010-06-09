#!/bin/bash

#  Thanks to Achmad Basuki for details and initial spec files.

set -x

RPMDIR=~/rpmbuild
TMPDIR=/tmp/xorp_ct_rpmdir
VERSION=$(cat VERSION)

mkdir -p $RPMDIR/SOURCES
mkdir -p $RPMDIR/SPECS

cp package_files/xorp.ct.spec $RPMDIR/SPECS/
cp package_files/xorp.sysconfig $RPMDIR/SOURCES
cp package_files/xorp.redhat $RPMDIR/SOURCES/
cp package_files/xorp.logrotate $RPMDIR/SOURCES/
cp package_files/xorp.conf $RPMDIR/SOURCES/

# Create a clean repo.
rm -fr $TMPDIR/xorp.ct.github
mkdir -p $TMPDIR
git clone ../ $TMPDIR/xorp.ct.github
(cd $TMPDIR/xorp.ct.github; cp README xorp/; tar cfa $RPMDIR/SOURCES/xorp-$VERSION.tar.lzma xorp; cd -)

# Build rpms
rpmbuild -ba $RPMDIR/SPECS/xorp.ct.spec  # building binary and the source

