!# /bin/sh
(exec ./FlowerCheck ./test_flower_malloc)
TEST_RESULT=$?
rm flower_report*
exit $TEST_RESULT
