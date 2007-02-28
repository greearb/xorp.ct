#!/usr/bin/env bash

#
# $XORP$
#

#
# If our border router knows about this host return its AS number
#

# This web page contains the mapping between ICIR hosts and AS numbers
MAPPING_URL=http://xorpc.icir.org/bgp/index.html

MY_IP=$(host $(hostname) | cut -f 4 -d ' ')
#MY_IP=5

host_to_as()
{
    fetch -q -o - $MAPPING_URL | grep '<th>' | 
    sed -e 's@<th>@@' -e 's@</th>@@' |
    while read AS
    do
	read IP
	read HOSTNAME
	read NOTE

	#echo "$AS $IP $HOSTNAME $NOTE"

	if [ "$IP" = "$MY_IP" ]
	then
	    echo $AS
	    return
	fi
    done
}

AS=$(host_to_as)
if [ "${AS:-""}" != "" ]
then
    echo $AS
    exit 0
else
    exit -1
fi

# Local Variables:
# mode: shell-script
# sh-indentation: 4
# End:
