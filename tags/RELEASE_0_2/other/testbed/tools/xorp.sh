#!/bin/sh
# xorp
#
# $XORP: other/testbed/tools/xorp.sh,v 1.8 2002/07/23 21:01:51 atanu Exp $
#
# Installed in /usr/local/etc/rc.d/ on testbed machines.
#

# Set the time from www.icir.org

echo -e "\nXORP testbed configuration"

if [ -e /usr/sbin/ntpdate ]
then
	echo "Setting time time"
	/usr/sbin/ntpdate -b www.icir.org
fi

if [ -d /usr/local/xorp ]
then
	cd /usr/local/xorp
	PATH=$PATH:/usr/local/bin
	PYTHON=/usr/local/bin/python
	SCRIPT=xtifset.py
	UTILS=xtutils.py
	XML=xtxml.py
	VARS=xtvars.py
	CONFIG=config.xml
	HOSTS_TEMPLATE=hosts.template
	HOSTS=/etc/hosts
	if [ -x ${PYTHON} -a -x ${SCRIPT} -a -f ${XML} -a -f ${UTILS} -a \
	-f ${CONFIG} -a -f ${VARS} ]
	then
		echo -e "Configure testbed interfaces"
		HOST=`uname -n`
		if ! ./${SCRIPT} -n $HOST -c ${CONFIG}
		then
			echo -e "This host is too smart to be configured"
		else
			./${SCRIPT} -i -c ${CONFIG}
			./${SCRIPT} -r -c ${CONFIG}
		fi

		if [ -f ${HOSTS_TEMPLATE} ]
		then
			echo "Creating /etc/hosts"

			./${SCRIPT} -H -c ${CONFIG} | \
				cat ${HOSTS_TEMPLATE} - > ${HOSTS}
		fi
			
	fi
fi
