#!/bin/sh

test_data=./test_xrl_parser.data
test_program=./test_xrl_parser

if [ ! -f ${test_data} ] ; then
    echo "${test_data} does not exist or is not a regular file." >&2
    exit 1
fi

if [ ! -x ${test_program} ] ; then
    echo "${test_program} does not exist or is not an executable file" >&2
    exit 1
fi

# Count instances of the word FAIL in test data file.  Sed is used to
# delete the initial comment in the file that documents FAIL should 
# prepend each entry in the file which is expected to fail parsing.
expected_errors=`cat ${test_data} | sed -e '1,/*\// d' | grep -c FAIL`

# XXX This test is flawed since it only counts bad and not good results, ie
# if parser re-sync fails after bad.  Simply noting this for now, but
# this should be corrected in future.

${test_program} ${test_data} >/dev/null
errors=$?

if [ ${expected_errors} -ne ${errors} ] ; then
echo "${test_program} on ${test_data} generated ${errors} when ${expected_errors} were expected." >&2
    exit 1
fi



