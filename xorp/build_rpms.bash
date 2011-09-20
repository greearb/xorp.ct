#!/bin/bash

#  Thanks to Achmad Basuki for details and initial spec files.

# NOTE:  You might need to edit ~/.rpmmacros to change the _topdir to be ~/rpmbuild
#        to have this script work correctly.

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

#rpmbuild -bb $RPMDIR/SPECS/xorp.ct.spec  # build just the binary
#rpmbuild -bs $RPMDIR/SPECS/xorp.ct.spec  # build just the source rpm

# Files will be written something like this:
#Wrote: /home/greearb/rpmbuild/SRPMS/xorp-1.8.3-1.fc13.src.rpm
#Wrote: /home/greearb/rpmbuild/RPMS/i686/xorp-1.8.3-1.fc13.i686.rpm
#Wrote: /home/greearb/rpmbuild/RPMS/i686/xorp-debuginfo-1.8.3-1.fc13.i686.rpm
