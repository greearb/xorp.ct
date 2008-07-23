#!/bin/sh

# Copyright (c) 2001-2008 XORP, Inc.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software")
# to deal in the Software without restriction, subject to the conditions
# listed in the XORP LICENSE file. These conditions include: you must
# preserve this copyright notice, and you cannot mention the copyright
# holders in advertising related to the Software without their permission.
# The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
# notice is a summary of the XORP LICENSE file; the license in that file is
# legally binding.

# $XORP: xorp/tests/install_templates.sh,v 1.3 2008/01/04 03:17:49 pavlin Exp $

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
