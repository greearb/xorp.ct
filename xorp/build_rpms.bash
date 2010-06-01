#!/bin/bash

#  Thanks to Achmad Basuki for details and initial spec files.

RPMDIR=~/src/rpm/
TMPDIR=/tmp/xorp_ct_rpmdir

mkdir -p $RPMDIR/SOURCES
mkdir -p $RPMDIR/SPECS

cp package_files/xorp.ct.spec $RPMDIR/SPECS/
cp package_files/xorp.sysconfig $RPMDIR/SOURCES
cp package_files/xorp.redhat $RPMDIR/SOURCES/
cp package_files/xorp.logrotate $RPMDIR/SOURCES/
cp package_files/xorp.conf $RPMDIR/SOURCES/

# Create a clean repo.
rm -fr $TMPDIR/xorp.ct
mkdir -p $TMPDIR
git clone ./ $TMPDIR/xorp.ct
(cd $TMPDIR; tar --exclude xorp.ct/.git cfa $RPMDIR/SOURCES/xorp-ct-1.7-WIP.tar.lzma xorp.ct; cd -)

# Build rpms
rpmbuild -ba $RPMDIR/SPECS/xorp.ct.spec  # building binary and the source

