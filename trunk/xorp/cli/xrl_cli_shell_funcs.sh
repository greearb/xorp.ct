#!/bin/sh

#
# $XORP: xorp/cli/xrl_cli_shell_funcs.sh,v 1.4 2003/03/25 01:21:45 pavlin Exp $
#

#
# Library of functions to sent XRLs to a running CLI process.
#

. ../utils/xrl_shell_lib.sh

# Conditionally set the target name
CLI_TARGET=${CLI_TARGET:="CLI"}

cli_enable_cli()
{
    echo "cli_enable_cli" $*
    XRL="finder://$CLI_TARGET/cli_manager/0.1/enable_cli"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS
}

cli_disable_cli()
{
    echo "cli_disable_cli" $*
    XRL="finder://$CLI_TARGET/cli_manager/0.1/disable_cli"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS
}

cli_start_cli()
{
    echo "cli_start_cli" $*
    XRL="finder://$CLI_TARGET/cli_manager/0.1/start_cli"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS
}

cli_stop_cli()
{
    echo "cli_stop_cli" $*
    XRL="finder://$CLI_TARGET/cli_manager/0.1/stop_cli"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS
}

cli_add_enable_cli_access_from_subnet4()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: cli_add_enable_cli_access_from_subnet4 <subnet_addr:ipv4net>"
	exit 1
    fi
    subnet_addr=$1
    
    echo "cli_add_enable_cli_access_from_subnet4" $*
    XRL="finder://$CLI_TARGET/cli_manager/0.1/add_enable_cli_access_from_subnet4"
    XRL_ARGS="?subnet_addr:ipv4net=$subnet_addr"
    call_xrl -r 0 $XRL$XRL_ARGS
}

cli_add_enable_cli_access_from_subnet6()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: cli_add_enable_cli_access_from_subnet6 <subnet_addr:ipv6net>"
	exit 1
    fi
    subnet_addr=$1
    
    echo "cli_add_enable_cli_access_from_subnet6" $*
    XRL="finder://$CLI_TARGET/cli_manager/0.1/add_enable_cli_access_from_subnet6"
    XRL_ARGS="?subnet_addr:ipv6net=$subnet_addr"
    call_xrl -r 0 $XRL$XRL_ARGS
}

cli_delete_enable_cli_access_from_subnet4()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: cli_delete_enable_cli_access_from_subnet4 <subnet_addr:ipv4net>"
	exit 1
    fi
    subnet_addr=$1
    
    echo "cli_delete_enable_cli_access_from_subnet4" $*
    XRL="finder://$CLI_TARGET/cli_manager/0.1/delete_enable_cli_access_from_subnet4"
    XRL_ARGS="?subnet_addr:ipv4net=$subnet_addr"
    call_xrl -r 0 $XRL$XRL_ARGS
}

cli_delete_enable_cli_access_from_subnet6()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: cli_delete_enable_cli_access_from_subnet6 <subnet_addr:ipv6net>"
	exit 1
    fi
    subnet_addr=$1
    
    echo "cli_delete_enable_cli_access_from_subnet6" $*
    XRL="finder://$CLI_TARGET/cli_manager/0.1/delete_enable_cli_access_from_subnet6"
    XRL_ARGS="?subnet_addr:ipv6net=$subnet_addr"
    call_xrl -r 0 $XRL$XRL_ARGS
}

cli_add_disable_cli_access_from_subnet4()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: cli_add_disable_cli_access_from_subnet4 <subnet_addr:ipv4net>"
	exit 1
    fi
    subnet_addr=$1
    
    echo "cli_add_disable_cli_access_from_subnet4" $*
    XRL="finder://$CLI_TARGET/cli_manager/0.1/add_disable_cli_access_from_subnet4"
    XRL_ARGS="?subnet_addr:ipv4net=$subnet_addr"
    call_xrl -r 0 $XRL$XRL_ARGS
}

cli_add_disable_cli_access_from_subnet6()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: cli_add_disable_cli_access_from_subnet6 <subnet_addr:ipv6net>"
	exit 1
    fi
    subnet_addr=$1
    
    echo "cli_add_disable_cli_access_from_subnet6" $*
    XRL="finder://$CLI_TARGET/cli_manager/0.1/add_disable_cli_access_from_subnet6"
    XRL_ARGS="?subnet_addr:ipv6net=$subnet_addr"
    call_xrl -r 0 $XRL$XRL_ARGS
}

cli_delete_disable_cli_access_from_subnet4()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: cli_delete_disable_cli_access_from_subnet4 <subnet_addr:ipv4net>"
	exit 1
    fi
    subnet_addr=$1
    
    echo "cli_delete_disable_cli_access_from_subnet4" $*
    XRL="finder://$CLI_TARGET/cli_manager/0.1/delete_disable_cli_access_from_subnet4"
    XRL_ARGS="?subnet_addr:ipv4net=$subnet_addr"
    call_xrl -r 0 $XRL$XRL_ARGS
}

cli_delete_disable_cli_access_from_subnet6()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: cli_delete_disable_cli_access_from_subnet6 <subnet_addr:ipv6net>"
	exit 1
    fi
    subnet_addr=$1
    
    echo "cli_delete_disable_cli_access_from_subnet6" $*
    XRL="finder://$CLI_TARGET/cli_manager/0.1/delete_disable_cli_access_from_subnet6"
    XRL_ARGS="?subnet_addr:ipv6net=$subnet_addr"
    call_xrl -r 0 $XRL$XRL_ARGS
}
