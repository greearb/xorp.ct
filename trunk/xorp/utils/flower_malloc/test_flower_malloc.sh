#! /bin/sh
(exec ./FlowerCheck ./test_flower_malloc)
TEST_RESULT=$?
rm flower_report*  > /dev/null  2>&1 
exit $TEST_RESULT
