#!/bin/sh

#
# $XORP: xorp/utils/xrl_shell_lib.sh,v 1.1 2003/06/03 18:52:19 pavlin Exp $
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
# Return 0 if the XRL contains the variable, otherwise return 1.
#
has_xrl_variable()
{
    local _xrl_result _xrl_variable_xrl_type

    if [ $# -lt 2 ] ; then
	echo "Usage: $0 <xrl_result> <xrl_variable:xrl_type>"
	exit 1
    fi
    _xrl_result="$1"
    _xrl_variable_xrl_type="$2"

    echo "${_xrl_result}" |						\
	awk -F "${XRL_VARIABLE_SEPARATOR}" -v xrl_variable_xrl_type="${_xrl_variable_xrl_type}" '
# AWK CODE STARTS
# XXX: do NOT put single quotas in the awk code below, otherwise
# it may not work!!
#
# The code assumes that in the command line there are the following options:
#	-F "${XRL_VARIABLE_SEPARATOR}"
#	to specify the XRL variable separator (e.g, "\&")
#
#	-v xrl_variable_xrl_type="${_xrl_variable_xrl_type}"
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
    local _xrl_result _xrl_variable_xrl_type

    if [ $# -lt 2 ] ; then
	echo "Usage: $0 <xrl_result> <xrl_variable:xrl_type>"
	exit 1
    fi
    _xrl_result="$1"
    _xrl_variable_xrl_type="$2"

    echo "${_xrl_result}" |						\
	awk -F "${XRL_VARIABLE_SEPARATOR}" -v xrl_variable_xrl_type="${_xrl_variable_xrl_type}" '
# AWK CODE STARTS
# XXX: do NOT put single quotas in the awk code below, otherwise
# it may not work!!
#
# Assume that there is in the command line
#	-F "${XRL_VARIABLE_SEPARATOR}"
#	to specify the XRL variable separator (e.g, "\&")
#
#	-v xrl_variable_xrl_type="${_xrl_variable_xrl_type}"
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
# Return 0 if no error, otherwise return 1.
#
test_xrl_result()
{
    local _xrl_result _xrl_variable_xrl_type _test_operator _test_value
    local _error _xrl_variable_value

    if [ $# -lt 4 ] ; then
	echo "Usage: $0 <xrl_result> <xrl_variable:xrl_type> <test-operator> <test-value>"
	exit 1
    fi
    _xrl_result="$1"
    _xrl_variable_xrl_type="$2"
    _test_operator="$3"
    _test_value="$4"

    if [ "X${_xrl_variable_xrl_type}" = "X" ] ; then
	# Nothing to test for
	return 0
    fi

    if [ "X${_test_operator}" = "X" ] ; then
	_error="missing <test-operator> argument"
	echo "ERROR: ${_error}"
	return 1
    fi
    if [ "X${_test_value}" = "X" ] ; then
	_error="missing <test-value> argument"
	echo "ERROR: ${_error}"
	return 1
    fi

    # Test if the variable was returned
    has_xrl_variable "${_xrl_result}" "${_xrl_variable_xrl_type}"
    if [ $? -ne 0 ] ; then
	_error="cannot find variable type '${_xrl_variable_xrl_type}' inside return string '${_xrl_result}'"
	echo "ERROR: ${_error}"
	return 1
    fi

    # Get the return value
    _xrl_variable_value=`get_xrl_variable_value "${_xrl_result}" "${_xrl_variable_xrl_type}"`
    
    # Test the return value
    if [ "X${_xrl_variable_value}" = "X" ] ; then
	_error="cannot find variable value-type '${_xrl_variable_xrl_type}' inside return string '${_xrl_result}'"
	echo "ERROR: ${_error}"
	return 1
    fi
    
    if [ "${_xrl_variable_value}" "${_test_operator}" "${_test_value}" ] ; then
	return 0
    else
	_error="return variable value of '${_xrl_variable_xrl_type}' is '${_xrl_variable_value}', but expected is '${_test_operator} ${_test_value}' inside return string '${_xrl_result}'"
	echo "ERROR: ${_error}"
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
# Return 0 if no error, otherwise return 1.
#
print_xrl_result()
{
    local _xrl_result _print_xrl_variable_xrl_type _print_xrl_variable_value
    local _error

    if [ $# -lt 1 ] ; then
	return 0	# Nothing to print
    fi

    _xrl_result="$1"
    shift

    while [ $# -gt 0 ] ; do
	_print_xrl_variable_xrl_type="$1"
	if [ "X${_print_xrl_variable_xrl_type}" != "X" ] ; then
	    if [ "X${_print_xrl_variable_xrl_type}" = "Xall" ] ; then
		# Print all result
		echo "${_xrl_result}"
	    else
		_print_xrl_variable_value=`get_xrl_variable_value "${_xrl_result}" "${_print_xrl_variable_xrl_type}"`
		if [ "X${_print_xrl_variable_value}" != "X" ] ; then
		    echo ${_print_xrl_variable_xrl_type}=${_print_xrl_variable_value}
		else
		    _error="${_print_xrl_variable_xrl_type}: NOT FOUND"
		    echo "ERROR: ${_error}"
		    return 1
		fi
	    fi
	fi
	shift
    done

    return 0
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
# Return 0 if no error, otherwise return 1.
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
    local _max_repeat_number _print_list_xrl_variable_xrl_type _xrl
    local _test_operator _test_value _error _ret_value

    if [ $# -lt 1 ] ; then
	echo "Usage: $0 [-r <max-repeat-number>] [-p <<xrl_variable:xrl_type> | all>] <XRL> [<xrl_variable:xrl_type> <test-operator> <test-value>]"
	exit 1
    fi

    _max_repeat_number=1		# Default value: try only once
    _print_list_xrl_variable_xrl_type=""
    while [ $# -gt 0 ] ; do
	case "$1" in
	    -r )		# [-r <max-repeat-number>]
		if [ $# -lt 2 ] ; then
		    echo "Usage: call_xrl [-r <max-repeat-number>] <<xrl_variable:xrl_type> | all>] <XRL> [<xrl_variable:xrl_type> <test-operator> <test-value>]"
		    exit 1
		fi
		shift
		_max_repeat_number=$1
		;;
	    -p )		# [-p <<xrl_variable:xrl_type> | all>]
		if [ $# -lt 2 ] ; then
		    echo "Usage: call_xrl [-r <max-repeat-number>] <<xrl_variable:xrl_type> | all>] <XRL> [<xrl_variable:xrl_type> <test-operator> <test-value>]"
		    exit 1
		fi
		shift
		_print_list_xrl_variable_xrl_type="${_print_list_xrl_variable_xrl_type} $1"
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

    _xrl="$1"
    # Note: the values below may be empty
    _xrl_variable_xrl_type="$2"
    _test_operator="$3"
    _test_value="$4"
    if [ "X${_xrl_variable_xrl_type}" != "X" ] ; then
	if [ "X${_test_operator}" = "X" ] ; then
	    echo "ERROR: missing <test-operator> argument"
	    exit 1
	fi
	if [ "X${_test_value}" = "X" ] ; then
	    echo "ERROR: missing <test-value> argument"
	    exit 1
	fi
    fi

    # Initialize the iterator
    if [ "${_max_repeat_number}" -eq 0 ] ; then
	_iter=-1
    else
	_iter=0
    fi
    while [ true ] ; do
	_xrl_result=`"${CALLXRL}" "${_xrl}"`
	_ret_value=$?
	if [ ${_ret_value} -ne 0 ] ; then
	    _error="failure calling '${CALLXRL} ${_xrl}'"
	    echo "ERROR: ${_error}"
	fi
	if [ ${_ret_value} -eq 0 -a "X${_test_operator}" != "X" -a "X${_test_value}" != "X" ] ; then
	    test_xrl_result "${_xrl_result}" "${_xrl_variable_xrl_type}" "${_test_operator}" "${_test_value}"
	    _ret_value=$?
	fi
	if [ ${_ret_value} -eq 0 ] ; then
	    print_xrl_result ${_xrl_result} ${_print_list_xrl_variable_xrl_type}
	    _ret_value=$?
	    if [ ${_ret_value} -ne 0 ] ; then
		_error="failure printing '${_print_list_xrl_variable_xrl_type}'"
		echo "ERROR: ${_error}"
	    else
		# OK
		break;
	    fi
	fi

	if [ "${_max_repeat_number}" -ne 0 ] ; then
	    _iter=$((${_iter}+1))
	fi
	if [ ${_iter} -ge ${_max_repeat_number} ] ; then
	    echo "ERROR: reached maximum number of trials. Giving-up..."
	    return 1
	fi

	echo "Trying again..."
	sleep 1
    done

    return 0
}

# Local Variables:
# mode: shell-script
# sh-indentation: 4
# End:
