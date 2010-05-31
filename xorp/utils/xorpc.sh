#!/bin/sh

#
# $XORP: xorp/utils/xorpc.sh,v 1.7 2002/12/09 11:13:07 pavlin Exp $
#

#
# Create ssh tunnels to all peripherals connected via xorpc which provide
# a HTTP interface.
# Atanu.
#

# Put the location of your ssh here. If you have ssh2 you can add the "-N" 
# flag
# SSH1
#SSH="/usr/local/bin/ssh"
# SSH2
SSH="/usr/bin/ssh -N "

for i in 9000:xorpc:80 9001:xorpsw1:80 9002:xorppwr1:80 9003:xorppwr2:80
do
	COMMAND="xterm -iconic -title $i -e $SSH -L $i xorpc.icir.org"
	$COMMAND &
done
