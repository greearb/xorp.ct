#!/bin/sh

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

default_options='-b 9600 -w 8 -p none'
default_sock_prot='ug=rw,o='
default_sock_owner='root.wheel'
default_log_prot='u=rw,g=r,o='

for host
do
	echo -n "startsrv($host):"
	#
	# kill any existing ttysrv on this port
	#
	if [ -f DESTPATH/pid/$host ]; then
		pid=`cat DESTPATH/pid/$host`
		while ps w$pid >/tmp/startsrv$$ 2>&1
		do
			grep -q ttysrv /tmp/startsrv$$ && {
				echo -n " $pid killed"
				kill $pid
				sleep 1
			} || {
				break
			}
		done
		rm DESTPATH/pid/$host /tmp/startsrv$$
	fi
	#
	# start up a new one
	#
	if [ -s DESTPATH/opt/${host}.srv ]; then
		options=`cat DESTPATH/opt/${host}.srv`
	elif [ -s DESTPATH/opt/DEFAULT.srv ]; then
		options=`cat DESTPATH/opt/DEFAULT.srv`
	else
		options="$default_options"
	fi

	if [ -s DESTPATH/prot/${host}.sock ]; then
		sock_prot=`cat DESTPATH/prot/${host}.sock`
	elif [ -s DESTPATH/prot/DEFAULT.sock ]; then
		sock_prot=`cat DESTPATH/prot/DEFAULT.sock`
	else
		sock_prot="$default_sock_prot"
	fi

	if [ -s DESTPATH/owner/${host}.sock ]; then
		sock_owner=`cat DESTPATH/owner/${host}.sock`
	elif [ -s DESTPATH/owner/DEFAULT.sock ]; then
		sock_owner=`cat DESTPATH/owner/DEFAULT.sock`
	else
		sock_owner="$default_sock_owner"
	fi

	if [ -s DESTPATH/prot/${host}.log ]; then
		log_prot=`cat DESTPATH/prot/${host}.log`
	elif [ -s DESTPATH/prot/DEFAULT.log ]; then
		log_prot=`cat DESTPATH/prot/DEFAULT.log`
	else
		log_prot="$default_log_prot"
	fi

	rm -f DESTPATH/sock/$host DESTPATH/pid/$host
	DESTPATH/bin/ttysrv $options \
		-t DESTPATH/dev/$host \
		-s DESTPATH/sock/$host \
		-l DESTPATH/log/$host \
		-i DESTPATH/pid/$host \
		> DESTPATH/out/$host 2>&1 &
	echo -n " "
	tries=5
	while [ $tries -gt 0 -a ! -f DESTPATH/pid/$host ]; do
		echo -n "."
		sleep 1
		tries=`expr $tries - 1`
	done
	if [ ! -f DESTPATH/pid/$host ]; then
		echo " [startup failed]"
	else
		newpid=`cat DESTPATH/pid/$host`
		chmod $sock_prot DESTPATH/sock/$host
		chown $sock_owner DESTPATH/sock/$host
		chmod $log_prot DESTPATH/log/$host
		echo " $newpid started"
	fi
done

exit
