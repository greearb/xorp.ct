#!/bin/sh

#
# $XORP: xorp/mfea/xrl_shell_lib.sh,v 1.3 2003/03/25 00:45:38 pavlin Exp $
#

#
# Library of functions to sent XRL and process the result.
#

#set -x

CALLXRL="../libxipc/call_xrl"
XRL_VARIABLE_SEPARATOR="\&"

#
# Test if the return result of an XRL contains a particular variable
#
# Usage: has_xrl_variable <xrl_result> <xrl_variable:xrl_type>
#
# Return 0 if OK, otherwise return 1.
#
has_xrl_variable()
{
    if [ $# -lt 2 ] ; then
	echo "Usage: $0 <xrl_result> <xrl_variable:xrl_type>"
	exit 1
    fi
    XRL_RESULT="$1"
    XRL_VARIABLE_XRL_TYPE="$2"
    
    echo "${XRL_RESULT}" |						\
	awk -F "${XRL_VARIABLE_SEPARATOR}" -v xrl_variable_xrl_type="${XRL_VARIABLE_XRL_TYPE}" '
# AWK CODE STARTS
# XXX: do NOT put single quotas in the awk code below, otherwise
# it may not work!!
#
# Assume that there is in the command line
#	-F "${XRL_VARIABLE_SEPARATOR}"
#	to specify the XRL variable separator (e.g, "\&")
#
#	-v xrl_variable_xrl_type="${XRL_VARIABLE_XRL_TYPE}"
#	to specify the variable name and type

BEGIN {
	found = 0;
}
{
	for (i = 1; i <= NF; i++) {
		j = split($i, a, "=");
		if (j < 1) {
			continue;
		}
		if (a[1] == xrl_variable_xrl_type) {
			found = 1;		# FOUND
			break;
		}
	}
}
END {
    if (found) {
	exit 0;
    } else {
	exit 1;
    }
}
# AWK CODE END
'
    
    return $?
}

#
# Get the value of an variable from an XRL.
#
# Usage: get_xrl_variable_value <xrl_result> <xrl_variable:xrl_type>
#
# Return the XRL variable value.
#
get_xrl_variable_value()
{
    if [ $# -lt 2 ] ; then
	echo "Usage: $0 <xrl_result> <xrl_variable:xrl_type>"
	exit 1
    fi
    XRL_RESULT="$1"
    XRL_VARIABLE_XRL_TYPE="$2"

    echo "${XRL_RESULT}" |						\
	awk -F "${XRL_VARIABLE_SEPARATOR}" -v xrl_variable_xrl_type="${XRL_VARIABLE_XRL_TYPE}" '
# AWK CODE STARTS
# XXX: do NOT put single quotas in the awk code below, otherwise
# it may not work!!
#
# Assume that there is in the command line
#	-F "${XRL_VARIABLE_SEPARATOR}"
#	to specify the XRL variable separator (e.g, "\&")
#
#	-v xrl_variable_xrl_type="${XRL_VARIABLE_XRL_TYPE}"
#	to specify the variable name and type

BEGIN {
    found = 0;
    value = "";
}
{
	for (i = 1; i <= NF; i++) {
		j = split($i, a, "=");
		if (j < 1) {
			continue;
		}
		if (a[1] == xrl_variable_xrl_type) {
			if (j >= 0) {
				# Non-empty value
				found = 1;
				value = a[2];
			} else {
				# Empty value
				found = 1;
			}
			break;		# FOUND
		}
	}
}
END {
    print value;
    if (found) {
	exit 0;
    } else {
	exit 1;				# NOT FOUND: shound NOT happen!
    }
}
# AWK CODE END
'
    
}

#
# Test the return result of an XRL whether the XRL has succeeded.
#
# Usage: test_xrl_result <xrl_result> <xrl_variable:xrl_type> <test-operator> <test-value>
#
# Return 0 if OK, otherwise return 1.
#
test_xrl_result()
{
    if [ $# -lt 4 ] ; then
	echo "Usage: $0 <xrl_result> <xrl_variable:xrl_type> <test-operator> <test-value>"
	exit 1
    fi
    XRL_RESULT="$1"
    XRL_VARIABLE_XRL_TYPE="$2"
    TEST_OPERATOR="$3"
    TEST_VALUE="$4"

    if [ "X${XRL_VARIABLE_XRL_TYPE}" = "X" ] ; then
	# Nothing to test for
	return 0
    fi

    if [ "X${TEST_OPERATOR}" = "X" ] ; then
	ERROR="missing <test-operator> argument"
	return 1
    fi
    if [ "X${TEST_VALUE}" = "X" ] ; then
	ERROR="missing <test-value> argument"
	return 1
    fi
    
    # Test if the variable was returned
    has_xrl_variable "${XRL_RESULT}" "${XRL_VARIABLE_XRL_TYPE}"
    if [ $? -ne 0 ] ; then
	ERROR="cannot find variable type '${XRL_VARIABLE_XRL_TYPE}' inside return string '${XRL_RESULT}'"
	return 1
    fi

    # Get the return value
    XRL_VARIABLE_VALUE=`get_xrl_variable_value "${XRL_RESULT}" "${XRL_VARIABLE_XRL_TYPE}"`
    
    # Test the return value
    if [ "X${XRL_VARIABLE_VALUE}" = "X" ] ; then
	ERROR="cannot find variable value-type '${XRL_VARIABLE_XRL_TYPE}' inside return string '${XRL_RESULT}'"
	return 1
    fi
    
    if [ "${XRL_VARIABLE_VALUE}" "${TEST_OPERATOR}" "${TEST_VALUE}" ] ; then
	return 0
    else
	ERROR="return variable value of '${XRL_VARIABLE_XRL_TYPE}' is '${XRL_VARIABLE_VALUE}', but expected is '${TEST_OPERATOR} ${TEST_VALUE}' inside return string '${XRL_RESULT}'"
	return 1
    fi
}

#
# Print the XRL return result
#
# Usage: print_xrl_result [<xrl_result> [<<xrl_variable:xrl_type> | all> ...]]
#
# Options: <xrl_result> The result returned from the XRL.
#
#          <<xrl_variable:xrl_type> | all> A list of XRL return variable names
#           whose return value to print. If the keyword 'all' is used, the
#	    return value of all return variables will be print.
#           Note: this option can be repeated.
#
#
print_xrl_result()
{
    if [ $# -lt 1 ] ; then
	return		# Nothing to print
    fi
    
    XRL_RESULT="$1"
    shift
    
    while [ $# -gt 0 ] ; do
	PRINT_XRL_VARIABLE_XRL_TYPE="$1"
	if [ "X${PRINT_XRL_VARIABLE_XRL_TYPE}" != "X" ] ; then
	    if [ "X${PRINT_XRL_VARIABLE_XRL_TYPE}" = "Xall" ] ; then
		echo "${XRL_RESULT}"
	    else
		PRINT_XRL_VARIABLE_VALUE=`get_xrl_variable_value "${XRL_RESULT}" "${PRINT_XRL_VARIABLE_XRL_TYPE}"`
		if [ "X${PRINT_XRL_VARIABLE_VALUE}" != "X" ] ; then
		    echo ${PRINT_XRL_VARIABLE_XRL_TYPE}=${PRINT_XRL_VARIABLE_VALUE}
		else
		    echo "${PRINT_XRL_VARIABLE_XRL_TYPE}: NOT FOUND"
		fi
	    fi
	fi
	shift
    done
}

#
# Call an XRL.
#
# If necessary to test the return value, keep calling the XRL until success.
#
# Usage: call_xrl [-r <max-repeat-number>] [-p <<xrl_variable:xrl_type> | all>] <XRL> [<xrl_variable:xrl_type> <test-operator> <test-value>]
#
# Options: -r <max-repeat-number> The maximum number of tries to call the
#             XRL before giving up. If the value is 0, then the XRL will be
#             called until success. If this option is omitted, the default
#             is to try only once.
#
#          -p <<xrl_variable:xrl_type> | all> The XRL return variable name
#             whose return value to print. If '-p all' is used, the return
#             value of all return variables will be print.
#             Note: this option can be repeated more than once.
#
# Example:
# MFEA_TARGET="MFEA_4"
# mfea_enable_vif()
# {
#     echo "mfea_enable_vif" $*
#     XRL="finder://${MFEA_TARGET}/mfea/0.1/enable_vif?vif_name:txt=$1"
#     call_xrl -r 0 ${XRL} fail:bool = false
# }
#
#
# XXX: there is latency of 1 second between repeated XRL calls.
#
call_xrl()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: $0 [-r <max-repeat-number>] [-p <<xrl_variable:xrl_type> | all>] <XRL> [<xrl_variable:xrl_type> <test-operator> <test-value>]"
	exit 1
    fi

    MAX_REPEAT_NUMBER=1		# Default value: try only once
    PRINT_LIST_XRL_VARIABLE_XRL_TYPE=""
    while [ $# -gt 0 ] ; do
	case "$1" in
	    -r )		# -r <max-repeat-number>
		if [ $# -lt 2 ] ; then
		    echo "Usage: call_xrl [-r <max-repeat-number>] <<xrl_variable:xrl_type> | all>] <XRL> [<xrl_variable:xrl_type> <test-operator> <test-value>]"
		    exit 1
		fi
		shift
		MAX_REPEAT_NUMBER=$1
		;;
	    -p )
		if [ $# -lt 2 ] ; then
		    echo "Usage: call_xrl [-r <max-repeat-number>] <<xrl_variable:xrl_type> | all>] <XRL> [<xrl_variable:xrl_type> <test-operator> <test-value>]"
		    exit 1
		fi
		shift
		PRINT_LIST_XRL_VARIABLE_XRL_TYPE="${PRINT_LIST_XRL_VARIABLE_XRL_TYPE} $1"
		;;
	    * )			# Default case
		break
		;;
	esac
	shift
    done
    
    if [ $# -lt 1 ] ; then
	echo "Usage: call_xrl [-r <max-repeat-number>] <<xrl_variable:xrl_type> | all>] <XRL> [<xrl_variable:xrl_type> <test-operator> <test-value>]"
	exit 1
    fi
    
    XRL="$1"
    # Note: the values below may be empty
    XRL_VARIABLE_XRL_TYPE="$2"
    TEST_OPERATOR="$3"
    TEST_VALUE="$4"
    if [ "X${XRL_VARIABLE_XRL_TYPE}" != "X" ] ; then
	if [ "X${TEST_OPERATOR}" = "X" ] ; then
	    echo "ERROR: missing <test-operator> argument"
	    exit 1
	fi
	if [ "X${TEST_VALUE}" = "X" ] ; then
	    echo "ERROR: missing <test-value> argument"
	    exit 1
	fi
    fi
    
    # Initialize the iterator
    if [ "${MAX_REPEAT_NUMBER}" -eq 0 ] ; then
	_iter=-1
    else
	_iter=0
    fi
    while [ true ] ; do
	XRL_RESULT=`"${CALLXRL}" "${XRL}"`
	ret_value=$?
	if [ ${ret_value} -ne 0 ] ; then
	    ERROR="failure calling '${CALLXRL} ${XRL}'"
	fi
	if [ ${ret_value} -eq 0 -a "X${TEST_OPERATOR}" != "X" -a "X${TEST_VALUE}" != "X" ] ; then
	    test_xrl_result "${XRL_RESULT}" "${XRL_VARIABLE_XRL_TYPE}" "${TEST_OPERATOR}" "${TEST_VALUE}"
	    ret_value=$?
	fi
	if [ ${ret_value} -eq 0 ] ; then
	    print_xrl_result ${XRL_RESULT} ${PRINT_LIST_XRL_VARIABLE_XRL_TYPE}
	    echo "OK"
	    break;
	fi
	
	echo "ERROR: ${ERROR}"
	
	if [ "${MAX_REPEAT_NUMBER}" -ne 0 ] ; then
	    _iter=$((${_iter}+1))
	fi
	if [ ${_iter} -ge ${MAX_REPEAT_NUMBER} ] ; then
	    echo "ERROR: reached maximum number of trials. Giving-up..."
	    break;
	fi
	
	echo "Trying again..."
	sleep 1
    done
}
