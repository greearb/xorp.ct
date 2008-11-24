#!/bin/sh

#
# $XORP: xorp/utils/rcsid2ident.sh,v 1.1 2005/04/29 21:43:04 pavlin Exp $
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
/#ifdef __unused/,/#endif/d
/static char const/d
s/yyrcsid\[\] = \(".*"\);/#ident \1/
wq
EOF
