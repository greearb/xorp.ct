#!/bin/sh

finder_host=127.0.0.1
finder_port=17777

#
# Start finder
#
./finder -p ${finder_port} &
finder_pid=$!

#
# Wait a couple of seconds and check it is still running
#
sleep 2
ps -p ${finder_pid} > /dev/null
if [ $? -ne 0 ] ; then
    echo "Finder did not start correctly."
    echo "Port ${finder_port} maybe in use by another application."
    exit 1
fi

for w in 1 2 3 4 5 6 8 9 10 11 12 13 ; do
echo "Trying wait period of $w seconds"

#
# Start test application
#
    ./test_finder_events -F ${finder_host}:${finder_port} &
    test_app_pid=$!

#
# Wait a couple of seconds and check it is still running
#
    sleep 2
    ps -p ${test_app_pid} > /dev/null
    if [ $? -ne 0 ] ; then
	echo "Test application did not start correctly."
	kill -9 ${finder_pid}
	exit 1
    fi

    sleep ${w}

#
# Kill test application, wait a couple of seconds, see if Finder is still up
#
    kill -9 ${test_app_pid}
    sleep 2

    ps -p ${finder_pid} > /dev/null
    if [ $? -ne 0 ] ; then
	echo "Finder died unexpectedly."
	exit 1
    fi
done

kill -9 ${finder_pid}

exit 0