#!/bin/sh

# Copyright (c) 2001-2009 XORP, Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, Version 2, June
# 1991 as published by the Free Software Foundation. Redistribution
# and/or modification of this program under the terms of any other
# version of the GNU General Public License is not permitted.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
# see the GNU General Public License, Version 2, a copy of which can be
# found in the XORP LICENSE.gpl file.
# 
# XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
# http://xorp.net

# $XORP: xorp/tests/install_templates.sh,v 1.5 2008/10/02 21:58:30 bms Exp $

# Take a local copy of the template files and modify them for use with
# the tests.

# Create an empty boot file.
touch empty.boot

TEMPLATES=templates

if [ ! -d $TEMPLATES ]
then
    mkdir $TEMPLATES
fi

cd $TEMPLATES
cp ../../etc/templates/* .

for i in *
do
    if grep xorp_fea $i
    then
    ed $i <<\EOF
1,$s/xorp_fea/xorp_fea_dummy/g
wq
EOF
    fi
done

# Remove the setting of local-ip and then add back the setting of
# local-ip, peer-port and local-port as noops.
ed bgp.tp <<EOF
/local-ip {/
d
d
d
i
local-ip {
%set:;
}
peer-port {
%set:;
}
local-port {
%set:;
}
.
wq
EOF

# allow any peer name but rewrite it in the XRL to 127.0.0.1
ed bgp.tp <<\EOF
/peer @ {/
.,/^	}/s/peer_ip:txt=$(@)/peer_ip:txt=127.0.0.1/
1
/peer @ {/
.,/^	}/s/peer_ip:txt=$(peer\.@)/peer_ip:txt=127.0.0.1/
wq
EOF

# Local Variables:
# mode: shell-script
# sh-indentation: 4
# End:
