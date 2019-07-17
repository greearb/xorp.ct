#!/bin/bash

#  Publish HTML web page to Candela Tech web server.

#  Run from root directory of source repository.

VERSION=$(cat xorp/VERSION)
CTVER=5.3.8

echo ""
echo "Packaging xorp version: $VERSION  CT Version: $CTVER"
echo ""

# Build xorp..need this for kdocs at least.
echo "Building xorp with: scons -j4 > /tmp/pkg_xorp_log.txt"
cd xorp;
scons -j4 > /tmp/pkg_xorp_log.txt 2>&1 || exit 1
cd ..

# Build web pages
echo "Building web pages."
cd www
make || exit 1

# Create directory to hold docs
rm -fr releases
mkdir -p releases/$VERSION/docs/
cd ..

# Build kdocs..this can take a while, even on a fast machine.
cd docs/kdoc
echo "Building doxygen code documentation..."
doxygen doxygen.cfg >> /tmp/pkg_xorp_log.txt 2>&1 || exit 1
cd html
mkdir -p ../../../www/releases/$VERSION/docs/kdoc/
mv html ../../../www/releases/$VERSION/docs/kdoc/
mv latex ../../../www/releases/$VERSION/docs/kdoc/
cd ..
cd ..
cd ..

echo "Copying files to the release directory..."
#  Copy some release files into place.
cp xorp/RELEASE_NOTES www/releases/$VERSION/docs/

mkdir -p www/releases/$VERSION/docs/config
cp -ar xorp/rtrmgr/config/* www/releases/$VERSION/docs/config/

# Package up source code for uploading
tar --exclude xorp/obj -cjf www/releases/$VERSION/xorp-$VERSION-src.tar.bz2 xorp

# If binaries are found, package them as well.
if [ -d /mnt/d2/pub/candela_cdrom.$CTVER ]
then
    cp /mnt/d2/pub/candela_cdrom.$CTVER/xorp*[1-9].tgz  www/releases/$VERSION/ || exit 1
    cp /mnt/d2/pub/candela_cdrom.$CTVER/xorp_win32.zip  www/releases/$VERSION/ || exit 1
else
    echo "Could not find xorp binaries directory: /mnt/d2/pub/candela_cdrom.$CTVER"
fi

cd www
# Create list of files to ignore
find html_src -name "*" -print > ../ignore_files.txt
echo GNUmakefile >> ../ignore_files.txt
echo html_src >> ../ignore_files.txt
echo README.1st >> ../ignore_files.txt


# Tar up the www tree, ignoring some files.
echo "Creating tarball for uploading..."
rm ../xorp_www.tar.bz2
tar -X ../ignore_files.txt -cjf ../xorp_www.tar.bz2 *
cd ..
