#!/bin/bash


# Build XORP binaries and copy them to the right folder.

VER=1.8.5

FC5=192.168.100.23  # v-fc5-32
FC8=192.168.100.24  # v-fc8-32
F11=192.168.100.25  # v-f11-32
F13=192.168.100.20
F14=192.168.100.34

FC8_64=192.168.100.31
F11_64=192.168.100.32
F13_64=192.168.100.3
F14_64=192.168.100.35


CDDIR="/mnt/d2/pub/xorp.$VER"

mkdir -p $CDDIR
chmod a+rwx $CDDIR

echo "Starting build at:"
date

DOXORP="cd ~/git/xorp.ct.github/xorp && scons -j4 && cd -"
DOXORPMING="cd ~/git/xorp.ct.github/xorp && scons -j4 strip=yes shared=no build=mingw32 STRIP=i686-pc-mingw32-strip \
         CC=i686-pc-mingw32-gcc CXX=i686-pc-mingw32-g++ \
	 RANLIB=i686-pc-mingw32-ranlib \
	 AR=i686-pc-mingw32-ar LD=i686-pc-mingw32-ld && cd -"
PULL32="cd ~/git/btbits && git pull && cd - && bash bin/btpull.sh"
BUILD32="$PULL32"
PULL64="cd ~/btbits/x64_btbits && git pull && cd - && bash bin/btpull.sh"
BUILD64="$PULL64"
PKGXORP="cd ~greearb/git/xorp.ct.github/xorp && bash ./lf_pkg.bash"
PKGXORPMING="cd ~greearb/git/xorp.ct.github/xorp && bash ./win32_pkg.bash"
CLEANUP="exit 0"
BUILDRPM="cd ~greearb/git/xorp.ct.github/xorp && bash ./build_rpms.bash && mv ~/rpmbuild/RPMS/*/xorp-$VER* /root/rpmbuild/SRPMS/xorp-$VER* $CDDIR"
BUILDDEB="cd ~greearb/git/xorp.ct.github/xorp && make -f Makefile.deb && mv lanforge-xorp_$VER* $CDDIR"

# Build XORP
echo "Building XORP..."

ssh $FC5 "$BUILD32 && $DOXORP && $CLEANUP" > build-5.txt 2>&1 || echo "FAILED FC5 build." &
ssh $F11 "$BUILD32 && $DOXORP && $CLEANUP" > build-11.txt 2>&1 || echo "FAILED F11 build."&
ssh $F13 "$BUILD32 && $DOXORP && $DOXORPMING && $CLEANUP" > build-13.txt 2>&1 || echo "FAILED F13-32 build." &
ssh $F14 "$BUILD32 && $DOXORP && $CLEANUP" > build-14.txt 2>&1 || echo "FAILED F14-32 build." &

ssh $FC8_64 "$BUILD64 && $DOXORP && $CLEANUP" > build-8-64.txt 2>&1 || echo "FAILED FC8-64 build." &
ssh $F11_64 "$BUILD64 && $DOXORP && $CLEANUP" > build-11-64.txt 2>&1 || echo "FAILED F11-64 build." &
ssh $F13_64 "$BUILD64 && $DOXORP && $CLEANUP" > build-13-64.txt 2>&1 || echo "FAILED F13-64 build." &
ssh $F14_64 "$BUILD64 && $DOXORP && $CLEANUP" > build-14-64.txt 2>&1 || echo "FAILED F14-64 build." &

wait

# Do xorp packaging
echo "Package xorp binaries and build RPMs."
ssh root@$FC5 "$PKGXORP && mv ~greearb/tmp/xorp_32.tgz $CDDIR/xorp_32-$VER-FC5.tgz && $CLEANUP" > build-pkg-x5.txt 2>&1 || echo "FAILED FC5 xorp package." &
ssh root@$FC8 "$PKGXORP && mv ~greearb/tmp/xorp_32.tgz $CDDIR/xorp_32-$VER-FC8.tgz && $CLEANUP" > build-pkg-x8.txt 2>&1 || echo "FAILED FC8 xorp package." &
ssh root@$F11 "$PKGXORP && mv ~greearb/tmp/xorp_32.tgz $CDDIR/xorp_32-$VER-F11.tgz && $CLEANUP" > build-pkg-x11.txt 2>&1 || echo "FAILED F11 xorp package." &
ssh root@$F13 "$PKGXORP && mv ~greearb/tmp/xorp_32.tgz $CDDIR/xorp_32-$VER-F13.tgz && $PKGXORPMING && mv ~greearb/tmp/xorp_win32.zip $CDDIR/xorp_win32-$VER.zip && $BUILDRPM && $CLEANUP" > build-pkg-x13.txt 2>&1 || echo "FAILED F13 xorp package." &
ssh root@$F14 "$PKGXORP && mv ~greearb/tmp/xorp_32.tgz $CDDIR/xorp_32-$VER-F14.tgz && $BUILDRPM && $BUILDDEB && $CLEANUP" > build-pkg-x14.txt 2>&1 || echo "FAILED F14 xorp package." &

ssh root@$FC8_64 "$PKGXORP && mv ~greearb/tmp/xorp_64.tgz $CDDIR/xorp_64-$VER-FC8.tgz && $CLEANUP" > build-pkg-x8-64.txt 2>&1 || echo "FAILED FC8-64 xorp package." &
ssh root@$F11_64 "$PKGXORP && mv ~greearb/tmp/xorp_64.tgz $CDDIR/xorp_64-$VER-F11.tgz  && $CLEANUP" > build-pkg-x11-64.txt 2>&1 || echo "FAILED F11-64 xorp package." &
ssh root@$F13_64 "$PKGXORP && mv ~greearb/tmp/xorp_64.tgz $CDDIR/xorp_64-$VER-F13.tgz && $BUILDRPM && $CLEANUP" > build-pkg-x13-64.txt 2>&1 || echo "FAILED F13-64 xorp package." &
ssh root@$F14_64 "$PKGXORP && mv ~greearb/tmp/xorp_64.tgz $CDDIR/xorp_64-$VER-F14.tgz && $BUILDRPM && $CLEANUP" > build-pkg-x14-64.txt 2>&1 || echo "FAILED F14-64 xorp package." &

wait

echo "Packaging source."
rm -fr /tmp/xorp-src-git
(cd ~/git/xorp.ct.github; git pull || echo "ERROR:  Could not pull xorp.ct!")
git clone ~/git/xorp.ct.github /tmp/xorp-src-git
cd /tmp/xorp-src-git
tar -czf $CDDIR/xorp-$VER-src.tar.gz xorp

date
