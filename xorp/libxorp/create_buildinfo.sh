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

# Uses sed, git, date, and uname commands, if available.
# git history is only available if compiled within a git tree.

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

cat build_info.prefix > $BINFO
if uname -mrspn > /dev/null 2>&1
    then
    echo "const char* BuildInfo::getBuildMachine() { return \"`uname -mrspn`\"; }" >> $BINFO
else
    echo "Warning:  uname -mrspn does not function on this system."
    echo "const char* BuildInfo::getBuildMachine() { return \"Unknown\"; }" >> $BINFO
fi
echo "" >> $BINFO

echo "const char* BuildInfo::getBuilder() { return \"${USER}\"; }" >> $BINFO
echo "" >> $BINFO

if date > /dev/null 2>&1
    then
    echo "const char* BuildInfo::getBuildDate() { return \"`date`\"; }" >> $BINFO
else
    echo "Warning:  'date' does not function on this system."
    echo "const char* BuildInfo::getBuildDate() { return \"Unknown\"; }" >> $BINFO
fi
echo "" >> $BINFO

if date '+%F %H:%M' > /dev/null 2>&1
    then
    echo "const char* BuildInfo::getShortBuildDate() { return \"`date '+%F %H:%M'`\"; }" >> $BINFO
else
    echo "Warning:  date +%F %H:%M does not function on this system."
    echo "const char* BuildInfo::getShortBuildDate() { return \"Unknown\"; }" >> $BINFO
fi
echo "" >> $BINFO

if which sed > /dev/null 2>&1 && which git > /dev/null 2>&1
    then
    if [ -x `which sed`  ] && [ -x `which git` ]
	then
	if git log -1 > /dev/null 2>&1
	    then
	    cat >> "${BINFO}" << EOF
const char* BuildInfo::getGitLog() { return
`git log -3 --abbrev=8 --abbrev-commit --pretty=oneline | sed -e 's|\\\\|\\\\\\\\|g' -e 's|"|\\\\"|g' -e 's|\(.*\)|"\1\\\n"|'`; }

EOF
	else
	    echo "NOTE:  Not a git repository, no git history in build-info."
	    cat >> "${BINFO}" << EOF
const char* BuildInfo::getGitLog() { return "Cannot detect, not a git repository"; }

EOF
	fi
    else
	echo "NOTE:  No functional sed and/or git, no git history in build-info."
	cat >> "${BINFO}" << EOF
const char* BuildInfo::getGitLog() { return "Cannot detect, sed and/or git is not executable"; }

EOF
    fi
else
    echo "NOTE:  No sed and/or git, no git history in build-info."
    cat >> "${BINFO}" << EOF
const char* BuildInfo::getGitLog() { return "Cannot detect, sed and/or git is not available"; }

EOF
fi


echo "const char* BuildInfo::getGitVersion() { return " >> $BINFO
if which git > /dev/null 2>&1
    then
    if git log -1 > /dev/null 2>&1
	then
	git log -1 --pretty=format:%h > last_git_md5sum.txt
	echo "\"`git log -1 --pretty=format:%h`\"; }" >> $BINFO
    else
	echo "\"00000000\"; }" >> $BINFO
    fi
else
    echo "\"00000000\"; }" >> $BINFO
fi

cd -
exit 0
