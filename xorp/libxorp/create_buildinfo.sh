#!/bin/sh

# This script auto-creates the build_info.cc file.
# Used by: scons ... enable_buildinfo=yes
# to compile some build information into xorp.
#
#
# Usage:  ./libxorp/create_buildinfo.sh

# If build_info.cc is newer than create_buildinfo.sh, and
# if the git tag hasn't changed, then do not re-create
# build_info.cc

cd libxorp

BINFO=build_info.cc

if [ -f $BINFO ]
    then
    if [ create_buildinfo.sh -ot $BINFO ]
	then
	if [ -f last_git_md5sum.txt ]
	    then
	    git log -1 --pretty=format:%h > tst_git_md5sum.txt
	    if diff -q last_git_md5sum.txt tst_git_md5sum.txt > /dev/null
		then
		# Files are the same, do nothing.
		echo "Not re-creating $BINFO file."
		cd -
		exit 0
	    else
		echo "Re-creating $BINFO: md5sums changed."
	    fi
	else
	    echo "Re-creating $BINFO: old md5sum doesn't exist."
	fi
    else
	echo "Re-creating $BINFO: builder script newer than build_info.cc"
    fi
else
    echo "Creating $BINFO: file doesn't exist yet."
fi

git log -1 --pretty=format:%h > last_git_md5sum.txt

cat build_info.prefix > $BINFO
echo "const char* BuildInfo::getBuildMachine() { return \"`uname -mrspn`\"; }" >> $BINFO
echo "" >> $BINFO
echo "const char* BuildInfo::getBuilder() { return \"${USER}\"; }" >> $BINFO
echo "" >> $BINFO
echo "const char* BuildInfo::getBuildDate() { return \"`date`\"; }" >> $BINFO
echo "" >> $BINFO
echo "const char* BuildInfo::getShortBuildDate() { return \"`date '+%F %H:%M'`\"; }" >> $BINFO
echo "" >> $BINFO

echo "const char* BuildInfo::getGitLog() { return " >> $BINFO
echo "`git log -3 --abbrev=8 --abbrev-commit --pretty=oneline|perl -n -e'chomp; s/\\\"/\\\\\"/g; print \"\\\"\"; print; print \"\\\n\\\"\n\"'`; }" >> $BINFO

echo "const char* BuildInfo::getGitVersion() { return " >> $BINFO
echo "\"`git log -1 --pretty=format:%h`\"; }" >> $BINFO

cd -
exit 0
