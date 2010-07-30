#!/bin/bash

#  Try to initialize the system before we start doing xorp buildbot
#  builds and tests.

# Kill all existing xorp processes
killall -9 -r xorp

# Remove tmp files
rm -fr /var/tmp/xrl.* || exit 1

# Remove build directory
rm -fr obj

exit 0
