#!/bin/bash

#  Try to initialize the system before we start doing xorp tests.

# Kill all existing xorp processes
killall -9 -r xorp

# Remove tmp files
rm -fr /var/tmp/xrl.* || exit 1

exit 0
