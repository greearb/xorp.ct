#!/bin/sh

#
# $XORP: xorp/utils/xrl_wrappers_generator.sh,v 1.2 2003/10/16 19:37:07 pavlin Exp $
#

#
# A script to auto-generate XRL shell wrappers that can be used in shell
# scripts to call XRLs.
# It takes as an argument a filename with a list of XRLs
# (e.g., one of the $XORP/xrl/targets/*.xrls files), and outputs on the
# standard output a list of shell wrappers (one wrapper per XRL).
#
# Example:
#    If the XRL to wrap is specified as:
#
#        finder://fea/ifmgr/0.1/get_configured_mtu?ifname:txt->mtu:u32
#
#    then the auto-generated wrapper will look like:
#
#        fea_ifmgr_get_configured_mtu()
#        {
#            if [ $# -ne 1 ] ; then
#                echo "Usage: fea_ifmgr_get_configured_mtu <ifname:txt>"
#                exit 1
#            fi
#
#            XRL="finder://fea/ifmgr/0.1/get_configured_mtu?ifname:txt=$1"
#            call_xrl_wrapper -p all "${XRL}"
#        }
#
#        where call_xrl_wrapper is a shell function defined in
#        $XORP/utils/xrl_shell_lib.sh
#

if [ $# -ne 1 ] ; then
    echo "Usage: $0 <file.xrls>"
    exit 1
fi

filter_xrls()
{
    if [ $# -ne 1 ] ; then
	echo "Usage: filter_xrls <file.xrls>"
	exit 1
    fi
    awk -F "://" '{if ($1 == "finder") {print $0}}' $1
}

get_xrl_target_name()
{
    if [ $# -ne 1 ] ; then
	echo "Usage: get_xrl_target_name <XRL>"
	exit 1
    fi

    echo $1 | awk -F "://" '{print $2}' | awk -F "/" '{print $1}'
}

get_xrl_interface_name()
{
    if [ $# -ne 1 ] ; then
	echo "Usage: get_xrl_interface_name <XRL>"
	exit 1
    fi

    echo $1 | awk -F "://" '{print $2}' | awk -F "/" '{print $2}'
}

get_xrl_interface_version()
{
    if [ $# -ne 1 ] ; then
	echo "Usage: get_xrl_interface_version <XRL>"
	exit 1
    fi

    echo $1 | awk -F "://" '{print $2}' | awk -F "/" '{print $3}'
}

get_xrl_method_name()
{
    if [ $# -ne 1 ] ; then
	echo "Usage: get_xrl_method_name <XRL>"
	exit 1
    fi

    echo $1 | awk -F "://" '{print $2}' | awk -F "/" '{print $4}' | awk -F "?" '{print $1}' | awk -F "->" '{print $1}'
}

get_xrl_arguments()
{
    if [ $# -ne 1 ] ; then
	echo "Usage: get_xrl_arguments <XRL>"
	exit 1
    fi

    echo $1 | awk -F "://" '{print $2}' | awk -F "/" '{print $4}' | awk -F "?" '{print $2}' | awk -F "->" '{print $1}'
}

get_xrl_arguments_number()
{
    if [ $# -ne 1 ] ; then
	echo "Usage: get_xrl_arguments_number <XRL arguments>"
	exit 1
    fi

    echo $1 | awk -F "&" '{print NF}'
}

get_xrl_split_arguments_list()
{
    if [ $# -ne 1 ] ; then
	echo "Usage: get_xrl_split_arguments_list <XRL arguments>"
	exit 1
    fi

    echo $1 | awk -F "&" '{for (i = 1; i <= NF; i++) {printf("%s", $i); if (i < NF) printf(" "); }}'
}

get_xrl_usage_wrap_arguments_list()
{
    if [ $# -ne 1 ] ; then
	echo "Usage: get_xrl_usage_wrap_arguments_list <split XRL arguments>"
	exit 1
    fi

    echo $1 | awk '{for (i = 1; i <= NF; i++) {printf("<%s>", $i); if (i < NF) printf(" "); }}'
}

get_xrl_call_wrap_arguments_list()
{
    if [ $# -ne 1 ] ; then
	echo "Usage: get_xrl_call_wrap_arguments_list <split XRL arguments>"
	exit 1
    fi

    echo $1 | awk '{for (i = 1; i <= NF; i++) {printf("%s=$%d", $i, i); if (i < NF) printf("&"); }}'
}

get_xrl_return_values()
{
    if [ $# -ne 1 ] ; then
	echo "Usage: get_xrl_return_values <XRL>"
	exit 1
    fi

    echo $1 | awk -F "://" '{print $2}' | awk -F "/" '{print $4}' | awk -F "?" '{print $2}' | awk -F "->" '{print $2}'
}

generate_wrapper()
{
    if [ $# -ne 1 ] ; then
	echo "Usage: generate_wrapper <XRL>"
	exit 1
    fi

    local _target_name _interface_name _interface_version
    local _method_name _arguments _arguments_number _split_arguments_list
    local _usage_wrap_arguments_list _call_wrap_arguments_list _return_values
    local _shell_function_name _usage_string _constructred_xrl

    _target_name=`get_xrl_target_name $1`
    _interface_name=`get_xrl_interface_name $1`
    _interface_version=`get_xrl_interface_version $1`
    _method_name=`get_xrl_method_name $1`
    _arguments=`get_xrl_arguments $1`
    _arguments_number=`get_xrl_arguments_number "${_arguments}"`
    _split_arguments_list=`get_xrl_split_arguments_list "${_arguments}"`
    _usage_wrap_arguments_list=`get_xrl_usage_wrap_arguments_list "${_split_arguments_list}"`
    #_return_values=`get_xrl_return_values $1`

    #
    # The shell function name
    #
    _shell_function_name="${_target_name}_${_interface_name}_${_method_name}"

    #
    # The usage string
    #
    if [ ${_arguments_number} -gt 0 ] ; then
	_usage_string="${_shell_function_name} ${_usage_wrap_arguments_list}"
    else
	_usage_string="${_shell_function_name}"
    fi

    #
    # Construct the XRL
    #
    _constructed_xrl="finder://${_target_name}/${_interface_name}/${_interface_version}/${_method_name}"
    if [ ${_arguments_number} -gt 0 ] ; then
	# Add the arguments
	_call_wrap_arguments_list=`get_xrl_call_wrap_arguments_list "${_split_arguments_list}"`
	_constructed_xrl="${_constructed_xrl}?${_call_wrap_arguments_list}"
    fi

    echo "${_shell_function_name}()"
    echo "{"
    echo "    if [ \$# -ne ${_arguments_number} ] ; then"
    echo "        echo \"Usage: ${_usage_string}\""
    echo "        exit 1"
    echo "    fi"
    echo ""
    echo "    XRL=\"${_constructed_xrl}\""
    echo "    call_xrl_wrapper -p all \"\${XRL}\""
    echo "}"
    echo ""
}

#
# Generate the XRL wrappers
#
file=$1
xrl_list=`filter_xrls ${file}`
for xrl in ${xrl_list} ; do
    generate_wrapper "${xrl}"
done

# Local Variables:
# mode: shell-script
# sh-indentation: 4
# End:
