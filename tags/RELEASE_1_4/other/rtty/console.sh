#! /bin/sh

# $Id$

# Copyright (c) 1996 by Internet Software Consortium.
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
# ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
# CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
# DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
# PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
# ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
# SOFTWARE.

default_options=''

cmdopts=''
while [ $# -gt 0 ]; do
	case $1 in
	-r) cmdopts="$opts -r"; shift ;;
	-*) shift ;;
	*) break 2 ;;
	esac
done

host=$1

[ -z "$host" ] && {
	ls DESTPATH/sock
	exit
}

if [ -s DESTPATH/opt/${host}.cons ]; then
	options=`cat DESTPATH/opt/${host}.cons`
elif [ -s DESTPATH/opt/DEFAULT.cons ]; then
	options=`cat DESTPATH/opt/DEFAULT.cons`
else
	options="$default_options"
fi

exec DESTPATH/bin/rtty $options $cmdopts DESTPATH/sock/$host
