ripng_common_get_target_name()
{
    if [ $# -ne 0 ] ; then
        echo "Usage: ripng_common_get_target_name"
        exit 1
    fi

    XRL="finder://ripng/common/0.1/get_target_name"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_common_get_version()
{
    if [ $# -ne 0 ] ; then
        echo "Usage: ripng_common_get_version"
        exit 1
    fi

    XRL="finder://ripng/common/0.1/get_version"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_common_get_status()
{
    if [ $# -ne 0 ] ; then
        echo "Usage: ripng_common_get_status"
        exit 1
    fi

    XRL="finder://ripng/common/0.1/get_status"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_common_shutdown()
{
    if [ $# -ne 0 ] ; then
        echo "Usage: ripng_common_shutdown"
        exit 1
    fi

    XRL="finder://ripng/common/0.1/shutdown"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_finder_event_observer_xrl_target_birth()
{
    if [ $# -ne 2 ] ; then
        echo "Usage: ripng_finder_event_observer_xrl_target_birth <target_class:txt> <target_instance:txt>"
        exit 1
    fi

    XRL="finder://ripng/finder_event_observer/0.1/xrl_target_birth?target_class:txt=$1&target_instance:txt=$2"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_finder_event_observer_xrl_target_death()
{
    if [ $# -ne 2 ] ; then
        echo "Usage: ripng_finder_event_observer_xrl_target_death <target_class:txt> <target_instance:txt>"
        exit 1
    fi

    XRL="finder://ripng/finder_event_observer/0.1/xrl_target_death?target_class:txt=$1&target_instance:txt=$2"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_redist6_add_route()
{
    if [ $# -ne 7 ] ; then
        echo "Usage: ripng_redist6_add_route <dst:ipv6net> <nexthop:ipv6> <ifname:txt> <vifname:txt> <metric:u32> <admin_distance:u32> <cookie:txt>"
        exit 1
    fi

    XRL="finder://ripng/redist6/0.1/add_route?dst:ipv6net=$1&nexthop:ipv6=$2&ifname:txt=$3&vifname:txt=$4&metric:u32=$5&admin_distance:u32=$6&cookie:txt=$7"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_redist6_delete_route()
{
    if [ $# -ne 2 ] ; then
        echo "Usage: ripng_redist6_delete_route <network:ipv6net> <cookie:txt>"
        exit 1
    fi

    XRL="finder://ripng/redist6/0.1/delete_route?network:ipv6net=$1&cookie:txt=$2"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_redist6_starting_route_dump()
{
    if [ $# -ne 1 ] ; then
        echo "Usage: ripng_redist6_starting_route_dump <cookie:txt>"
        exit 1
    fi

    XRL="finder://ripng/redist6/0.1/starting_route_dump?cookie:txt=$1"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_redist6_finishing_route_dump()
{
    if [ $# -ne 1 ] ; then
        echo "Usage: ripng_redist6_finishing_route_dump <cookie:txt>"
        exit 1
    fi

    XRL="finder://ripng/redist6/0.1/finishing_route_dump?cookie:txt=$1"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_ripng_add_rip_address()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: ripng_ripng_add_rip_address <ifname:txt> <vifname:txt> <addr:ipv6>"
        exit 1
    fi

    XRL="finder://ripng/ripng/0.1/add_rip_address?ifname:txt=$1&vifname:txt=$2&addr:ipv6=$3"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_ripng_remove_rip_address()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: ripng_ripng_remove_rip_address <ifname:txt> <vifname:txt> <addr:ipv6>"
        exit 1
    fi

    XRL="finder://ripng/ripng/0.1/remove_rip_address?ifname:txt=$1&vifname:txt=$2&addr:ipv6=$3"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_ripng_set_rip_address_enabled()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: ripng_ripng_set_rip_address_enabled <ifname:txt> <vifname:txt> <addr:ipv6> <enabled:bool>"
        exit 1
    fi

    XRL="finder://ripng/ripng/0.1/set_rip_address_enabled?ifname:txt=$1&vifname:txt=$2&addr:ipv6=$3&enabled:bool=$4"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_ripng_rip_address_enabled()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: ripng_ripng_rip_address_enabled <ifname:txt> <vifname:txt> <addr:ipv6>"
        exit 1
    fi

    XRL="finder://ripng/ripng/0.1/rip_address_enabled?ifname:txt=$1&vifname:txt=$2&addr:ipv6=$3"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_ripng_set_cost()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: ripng_ripng_set_cost <ifname:txt> <vifname:txt> <addr:ipv6> <cost:u32>"
        exit 1
    fi

    XRL="finder://ripng/ripng/0.1/set_cost?ifname:txt=$1&vifname:txt=$2&addr:ipv6=$3&cost:u32=$4"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_ripng_cost()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: ripng_ripng_cost <ifname:txt> <vifname:txt> <addr:ipv6>"
        exit 1
    fi

    XRL="finder://ripng/ripng/0.1/cost?ifname:txt=$1&vifname:txt=$2&addr:ipv6=$3"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_ripng_set_horizon()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: ripng_ripng_set_horizon <ifname:txt> <vifname:txt> <addr:ipv6> <horizon:txt>"
        exit 1
    fi

    XRL="finder://ripng/ripng/0.1/set_horizon?ifname:txt=$1&vifname:txt=$2&addr:ipv6=$3&horizon:txt=$4"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_ripng_horizon()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: ripng_ripng_horizon <ifname:txt> <vifname:txt> <addr:ipv6>"
        exit 1
    fi

    XRL="finder://ripng/ripng/0.1/horizon?ifname:txt=$1&vifname:txt=$2&addr:ipv6=$3"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_ripng_set_passive()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: ripng_ripng_set_passive <ifname:txt> <vifname:txt> <addr:ipv6> <passive:bool>"
        exit 1
    fi

    XRL="finder://ripng/ripng/0.1/set_passive?ifname:txt=$1&vifname:txt=$2&addr:ipv6=$3&passive:bool=$4"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_ripng_passive()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: ripng_ripng_passive <ifname:txt> <vifname:txt> <addr:ipv6>"
        exit 1
    fi

    XRL="finder://ripng/ripng/0.1/passive?ifname:txt=$1&vifname:txt=$2&addr:ipv6=$3"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_ripng_set_accept_non_rip_requests()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: ripng_ripng_set_accept_non_rip_requests <ifname:txt> <vifname:txt> <addr:ipv6> <accept:bool>"
        exit 1
    fi

    XRL="finder://ripng/ripng/0.1/set_accept_non_rip_requests?ifname:txt=$1&vifname:txt=$2&addr:ipv6=$3&accept:bool=$4"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_ripng_accept_non_rip_requests()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: ripng_ripng_accept_non_rip_requests <ifname:txt> <vifname:txt> <addr:ipv6>"
        exit 1
    fi

    XRL="finder://ripng/ripng/0.1/accept_non_rip_requests?ifname:txt=$1&vifname:txt=$2&addr:ipv6=$3"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_ripng_set_accept_default_route()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: ripng_ripng_set_accept_default_route <ifname:txt> <vifname:txt> <addr:ipv6> <accept:bool>"
        exit 1
    fi

    XRL="finder://ripng/ripng/0.1/set_accept_default_route?ifname:txt=$1&vifname:txt=$2&addr:ipv6=$3&accept:bool=$4"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_ripng_accept_default_route()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: ripng_ripng_accept_default_route <ifname:txt> <vifname:txt> <addr:ipv6>"
        exit 1
    fi

    XRL="finder://ripng/ripng/0.1/accept_default_route?ifname:txt=$1&vifname:txt=$2&addr:ipv6=$3"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_ripng_set_advertise_default_route()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: ripng_ripng_set_advertise_default_route <ifname:txt> <vifname:txt> <addr:ipv6> <advertise:bool>"
        exit 1
    fi

    XRL="finder://ripng/ripng/0.1/set_advertise_default_route?ifname:txt=$1&vifname:txt=$2&addr:ipv6=$3&advertise:bool=$4"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_ripng_advertise_default_route()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: ripng_ripng_advertise_default_route <ifname:txt> <vifname:txt> <addr:ipv6>"
        exit 1
    fi

    XRL="finder://ripng/ripng/0.1/advertise_default_route?ifname:txt=$1&vifname:txt=$2&addr:ipv6=$3"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_ripng_set_route_expiry_seconds()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: ripng_ripng_set_route_expiry_seconds <ifname:txt> <vifname:txt> <addr:ipv6> <t_secs:u32>"
        exit 1
    fi

    XRL="finder://ripng/ripng/0.1/set_route_expiry_seconds?ifname:txt=$1&vifname:txt=$2&addr:ipv6=$3&t_secs:u32=$4"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_ripng_route_expiry_seconds()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: ripng_ripng_route_expiry_seconds <ifname:txt> <vifname:txt> <addr:ipv6>"
        exit 1
    fi

    XRL="finder://ripng/ripng/0.1/route_expiry_seconds?ifname:txt=$1&vifname:txt=$2&addr:ipv6=$3"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_ripng_set_route_deletion_seconds()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: ripng_ripng_set_route_deletion_seconds <ifname:txt> <vifname:txt> <addr:ipv6> <t_secs:u32>"
        exit 1
    fi

    XRL="finder://ripng/ripng/0.1/set_route_deletion_seconds?ifname:txt=$1&vifname:txt=$2&addr:ipv6=$3&t_secs:u32=$4"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_ripng_route_deletion_seconds()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: ripng_ripng_route_deletion_seconds <ifname:txt> <vifname:txt> <addr:ipv6>"
        exit 1
    fi

    XRL="finder://ripng/ripng/0.1/route_deletion_seconds?ifname:txt=$1&vifname:txt=$2&addr:ipv6=$3"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_ripng_set_table_request_seconds()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: ripng_ripng_set_table_request_seconds <ifname:txt> <vifname:txt> <addr:ipv6> <t_secs:u32>"
        exit 1
    fi

    XRL="finder://ripng/ripng/0.1/set_table_request_seconds?ifname:txt=$1&vifname:txt=$2&addr:ipv6=$3&t_secs:u32=$4"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_ripng_table_request_seconds()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: ripng_ripng_table_request_seconds <ifname:txt> <vifname:txt> <addr:ipv6>"
        exit 1
    fi

    XRL="finder://ripng/ripng/0.1/table_request_seconds?ifname:txt=$1&vifname:txt=$2&addr:ipv6=$3"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_ripng_set_unsolicited_response_min_seconds()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: ripng_ripng_set_unsolicited_response_min_seconds <ifname:txt> <vifname:txt> <addr:ipv6> <t_secs:u32>"
        exit 1
    fi

    XRL="finder://ripng/ripng/0.1/set_unsolicited_response_min_seconds?ifname:txt=$1&vifname:txt=$2&addr:ipv6=$3&t_secs:u32=$4"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_ripng_unsolicited_response_min_seconds()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: ripng_ripng_unsolicited_response_min_seconds <ifname:txt> <vifname:txt> <addr:ipv6>"
        exit 1
    fi

    XRL="finder://ripng/ripng/0.1/unsolicited_response_min_seconds?ifname:txt=$1&vifname:txt=$2&addr:ipv6=$3"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_ripng_set_unsolicited_response_max_seconds()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: ripng_ripng_set_unsolicited_response_max_seconds <ifname:txt> <vifname:txt> <addr:ipv6> <t_secs:u32>"
        exit 1
    fi

    XRL="finder://ripng/ripng/0.1/set_unsolicited_response_max_seconds?ifname:txt=$1&vifname:txt=$2&addr:ipv6=$3&t_secs:u32=$4"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_ripng_unsolicited_response_max_seconds()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: ripng_ripng_unsolicited_response_max_seconds <ifname:txt> <vifname:txt> <addr:ipv6>"
        exit 1
    fi

    XRL="finder://ripng/ripng/0.1/unsolicited_response_max_seconds?ifname:txt=$1&vifname:txt=$2&addr:ipv6=$3"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_ripng_set_triggered_update_min_seconds()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: ripng_ripng_set_triggered_update_min_seconds <ifname:txt> <vifname:txt> <addr:ipv6> <t_secs:u32>"
        exit 1
    fi

    XRL="finder://ripng/ripng/0.1/set_triggered_update_min_seconds?ifname:txt=$1&vifname:txt=$2&addr:ipv6=$3&t_secs:u32=$4"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_ripng_triggered_update_min_seconds()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: ripng_ripng_triggered_update_min_seconds <ifname:txt> <vifname:txt> <addr:ipv6>"
        exit 1
    fi

    XRL="finder://ripng/ripng/0.1/triggered_update_min_seconds?ifname:txt=$1&vifname:txt=$2&addr:ipv6=$3"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_ripng_set_triggered_update_max_seconds()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: ripng_ripng_set_triggered_update_max_seconds <ifname:txt> <vifname:txt> <addr:ipv6> <t_secs:u32>"
        exit 1
    fi

    XRL="finder://ripng/ripng/0.1/set_triggered_update_max_seconds?ifname:txt=$1&vifname:txt=$2&addr:ipv6=$3&t_secs:u32=$4"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_ripng_triggered_update_max_seconds()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: ripng_ripng_triggered_update_max_seconds <ifname:txt> <vifname:txt> <addr:ipv6>"
        exit 1
    fi

    XRL="finder://ripng/ripng/0.1/triggered_update_max_seconds?ifname:txt=$1&vifname:txt=$2&addr:ipv6=$3"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_ripng_set_interpacket_delay_milliseconds()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: ripng_ripng_set_interpacket_delay_milliseconds <ifname:txt> <vifname:txt> <addr:ipv6> <t_msecs:u32>"
        exit 1
    fi

    XRL="finder://ripng/ripng/0.1/set_interpacket_delay_milliseconds?ifname:txt=$1&vifname:txt=$2&addr:ipv6=$3&t_msecs:u32=$4"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_ripng_interpacket_delay_milliseconds()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: ripng_ripng_interpacket_delay_milliseconds <ifname:txt> <vifname:txt> <addr:ipv6>"
        exit 1
    fi

    XRL="finder://ripng/ripng/0.1/interpacket_delay_milliseconds?ifname:txt=$1&vifname:txt=$2&addr:ipv6=$3"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_ripng_rip_address_status()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: ripng_ripng_rip_address_status <ifname:txt> <vifname:txt> <addr:ipv6>"
        exit 1
    fi

    XRL="finder://ripng/ripng/0.1/rip_address_status?ifname:txt=$1&vifname:txt=$2&addr:ipv6=$3"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_ripng_get_all_addresses()
{
    if [ $# -ne 0 ] ; then
        echo "Usage: ripng_ripng_get_all_addresses"
        exit 1
    fi

    XRL="finder://ripng/ripng/0.1/get_all_addresses"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_ripng_get_counters()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: ripng_ripng_get_counters <ifname:txt> <vifname:txt> <addr:ipv6>"
        exit 1
    fi

    XRL="finder://ripng/ripng/0.1/get_counters?ifname:txt=$1&vifname:txt=$2&addr:ipv6=$3"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_ripng_get_peers()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: ripng_ripng_get_peers <ifname:txt> <vifname:txt> <addr:ipv6>"
        exit 1
    fi

    XRL="finder://ripng/ripng/0.1/get_peers?ifname:txt=$1&vifname:txt=$2&addr:ipv6=$3"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_ripng_get_all_peers()
{
    if [ $# -ne 0 ] ; then
        echo "Usage: ripng_ripng_get_all_peers"
        exit 1
    fi

    XRL="finder://ripng/ripng/0.1/get_all_peers"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_ripng_get_peer_counters()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: ripng_ripng_get_peer_counters <ifname:txt> <vifname:txt> <addr:ipv6> <peer:ipv6>"
        exit 1
    fi

    XRL="finder://ripng/ripng/0.1/get_peer_counters?ifname:txt=$1&vifname:txt=$2&addr:ipv6=$3&peer:ipv6=$4"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_ripng_redist_protocol_routes()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: ripng_ripng_redist_protocol_routes <protocol_name:txt> <cost:u32> <tag:u32>"
        exit 1
    fi

    XRL="finder://ripng/ripng/0.1/redist_protocol_routes?protocol_name:txt=$1&cost:u32=$2&tag:u32=$3"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_ripng_no_redist_protocol_routes()
{
    if [ $# -ne 1 ] ; then
        echo "Usage: ripng_ripng_no_redist_protocol_routes <protocol_name:txt>"
        exit 1
    fi

    XRL="finder://ripng/ripng/0.1/no_redist_protocol_routes?protocol_name:txt=$1"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_socket6_user_recv_event()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: ripng_socket6_user_recv_event <sockid:txt> <src_host:ipv6> <src_port:u32> <data:binary>"
        exit 1
    fi

    XRL="finder://ripng/socket6_user/0.1/recv_event?sockid:txt=$1&src_host:ipv6=$2&src_port:u32=$3&data:binary=$4"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_socket6_user_connect_event()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: ripng_socket6_user_connect_event <sockid:txt> <src_host:ipv6> <src_port:u32> <new_sockid:txt>"
        exit 1
    fi

    XRL="finder://ripng/socket6_user/0.1/connect_event?sockid:txt=$1&src_host:ipv6=$2&src_port:u32=$3&new_sockid:txt=$4"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_socket6_user_error_event()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: ripng_socket6_user_error_event <sockid:txt> <error:txt> <fatal:bool>"
        exit 1
    fi

    XRL="finder://ripng/socket6_user/0.1/error_event?sockid:txt=$1&error:txt=$2&fatal:bool=$3"
    call_xrl_wrapper -p all "${XRL}"
}

ripng_socket6_user_close_event()
{
    if [ $# -ne 2 ] ; then
        echo "Usage: ripng_socket6_user_close_event <sockid:txt> <reason:txt>"
        exit 1
    fi

    XRL="finder://ripng/socket6_user/0.1/close_event?sockid:txt=$1&reason:txt=$2"
    call_xrl_wrapper -p all "${XRL}"
}

