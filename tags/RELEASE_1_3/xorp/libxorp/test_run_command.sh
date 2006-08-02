#!/bin/sh
#
# $XORP$
#
# The Windows version of the test_run_command regression test needs to be
# run from an external (i.e. non-MSYS) shell, or else it will fail.
#

if [ x"$OSTYPE" = xmsys ]; then
   exec_cmd="cmd //c test_run_command.exe"
else
   exec_cmd="./test_run_command"
fi

exec $exec_cmd $*
