#!/bin/sh

finder_host=127.0.0.1
finder_port=17777

#
# Start finder
#
../libxipc/xorp_finder &
finder_pid=$!

#
# Wait a couple of seconds and check it is still running
#
sleep 2
ps -p ${finder_pid} > /dev/null
if [ $? -ne 0 ] ; then
    echo "Finder did not start correctly."
    exit 1
fi

./test_xrl_sockets4_udp -F ${finder_host}:${finder_port}
exit_code=$?

ps -p ${finder_pid} > /dev/null
if [ $? -ne 0 ] ; then
    echo "Finder died unexpectedly."
    exit 1
fi

kill -9 ${finder_pid}
exit ${exit_code}