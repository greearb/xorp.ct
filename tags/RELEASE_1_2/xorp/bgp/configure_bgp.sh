#!/bin/sh

#
# $XORP: xorp/bgp/configure_bgp.sh,v 1.15 2002/12/09 10:59:36 pavlin Exp $
#

#
# Send configuration commands to a running bgp process.
#

. ./xrl_shell_funcs.sh

case `hostname` in
	aardvark)
	LOCALHOST=aardvark.icir.org
	PORT=9000
	AS=65009
	ID=192.150.187.20
	HOLDTIME=0
	local_config $AS $ID
#	register_rib rib

	PEER=xorp-c4000.icir.org
	PEER_AS=65000
	PEER_PORT=179
	NEXT_HOP=192.150.187.78
	add_peer $LOCALHOST $PORT $PEER $PEER_PORT $PEER_AS $NEXT_HOP $HOLDTIME
	enable_peer $LOCALHOST $PORT $PEER $PEER_PORT $PEER_AS

	;;
	dhcp111)
	LOCALHOST=dhcp111.icir.org
	PORT=9000
	AS=65001
	ID=192.150.187.111
	HOLDTIME=0
	local_config $AS $ID
#	register_rib rib

	PEER=xorp-c4000.icir.org
	PEER_AS=65000
	PEER_PORT=179
	NEXT_HOP=192.150.187.78
	add_peer $LOCALHOST $PORT $PEER $PEER_PORT $PEER_AS $NEXT_HOP $HOLDTIME
	enable_peer $LOCALHOST $PORT $PEER $PEER_PORT $PEER_AS
	
	;;
	tigger.icir.org)
	LOCALHOST=tigger.icir.org
	PORT=9000
	AS=65008
	ID=192.150.187.78
	HOLDTIME=0
	local_config $AS $ID
#	register_rib rib
	
	PEER=xorp-c4000.icir.org
	PEER_AS=65000
	PEER_PORT=179
	NEXT_HOP=192.150.187.78
	add_peer $LOCALHOST $PORT $PEER $PEER_PORT $PEER_AS $NEXT_HOP $HOLDTIME
	enable_peer $LOCALHOST $PORT $PEER $PEER_PORT $PEER_AS

	PEER=tigger.icir.org
	PEER_AS=65008
	PEER_PORT=9001
	NEXT_HOP=192.150.187.78
	add_peer $LOCALHOST $PORT $PEER $PEER_PORT $PEER_AS $NEXT_HOP $HOLDTIME
	enable_peer $LOCALHOST $PORT $PEER $PEER_PORT $PEER_AS 
exit
	ORIGIN=0
	AS_PATH=$AS
	NEXT_HOP=192.150.187.78
	NLRI=128.16.0.0/16
	#add_route $ORIGIN $AS_PATH $NEXT_HOP $NLRI 

	#sleep 10

	#disable_peer $PEER $PEER_AS
	#enable_peer $PEER $PEER_AS
	#add_route $ORIGIN $AS_PATH $NEXT_HOP $NLRI 

	#terminate

#	set -e # Bomb if a call fails
	while :
	do
		sleep 20
		date
		disable_peer $PEER $PEER_AS
		sleep 20
		date
		enable_peer $PEER $PEER_AS
	done

	;;
	*)
		echo "Unknown host :" `hostname`
		exit 1
	;;
esac


