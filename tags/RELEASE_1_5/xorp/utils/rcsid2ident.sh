#!/bin/sh

#
# $XORP$
#

#
# This script replaces the first instance of the static variable "yyrcsid"
# that contains the RCS/CVS ident string with #ident.
# This replacement is needed because some compilers (e.g., gcc-4.1)
# generate a warning about variable "yyrcsid" being unused.
#

if [ $# -ne 1 ] ; then
    echo "Usage: $0 <filename>"
    exit 1
fi

ed $1 <<EOF
/static char const/
d
/yyrcsid/
s/yyrcsid\[\] =/#ident/
s/;//
wq
EOF
