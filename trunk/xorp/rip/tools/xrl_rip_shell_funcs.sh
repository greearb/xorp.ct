rip_common_get_target_name()
{
    if [ $# -ne 0 ] ; then
        echo "Usage: rip_common_get_target_name"
        exit 1
    fi

    XRL="finder://rip/common/0.1/get_target_name"
    call_xrl_wrapper -p all "${XRL}"
}

rip_common_get_version()
{
    if [ $# -ne 0 ] ; then
        echo "Usage: rip_common_get_version"
        exit 1
    fi

    XRL="finder://rip/common/0.1/get_version"
    call_xrl_wrapper -p all "${XRL}"
}

rip_common_get_status()
{
    if [ $# -ne 0 ] ; then
        echo "Usage: rip_common_get_status"
        exit 1
    fi

    XRL="finder://rip/common/0.1/get_status"
    call_xrl_wrapper -p all "${XRL}"
}

rip_common_shutdown()
{
    if [ $# -ne 0 ] ; then
        echo "Usage: rip_common_shutdown"
        exit 1
    fi

    XRL="finder://rip/common/0.1/shutdown"
    call_xrl_wrapper -p all "${XRL}"
}

rip_finder_event_observer_xrl_target_birth()
{
    if [ $# -ne 2 ] ; then
        echo "Usage: rip_finder_event_observer_xrl_target_birth <target_class:txt> <target_instance:txt>"
        exit 1
    fi

    XRL="finder://rip/finder_event_observer/0.1/xrl_target_birth?target_class:txt=$1&target_instance:txt=$2"
    call_xrl_wrapper -p all "${XRL}"
}

rip_finder_event_observer_xrl_target_death()
{
    if [ $# -ne 2 ] ; then
        echo "Usage: rip_finder_event_observer_xrl_target_death <target_class:txt> <target_instance:txt>"
        exit 1
    fi

    XRL="finder://rip/finder_event_observer/0.1/xrl_target_death?target_class:txt=$1&target_instance:txt=$2"
    call_xrl_wrapper -p all "${XRL}"
}

rip_redist4_add_route()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: rip_redist4_add_route <network:ipv4net> <nexthop:ipv4> <global_metric:u32> <cookie:txt>"
        exit 1
    fi

    XRL="finder://rip/redist4/0.1/add_route?network:ipv4net=$1&nexthop:ipv4=$2&global_metric:u32=$3&cookie:txt=$4"
    call_xrl_wrapper -p all "${XRL}"
}

rip_redist4_delete_route()
{
    if [ $# -ne 2 ] ; then
        echo "Usage: rip_redist4_delete_route <network:ipv4net> <cookie:txt>"
        exit 1
    fi

    XRL="finder://rip/redist4/0.1/delete_route?network:ipv4net=$1&cookie:txt=$2"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_add_rip_address()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: rip_rip_add_rip_address <ifname:txt> <vifname:txt> <addr:ipv4>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/add_rip_address?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_remove_rip_address()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: rip_rip_remove_rip_address <ifname:txt> <vifname:txt> <addr:ipv4>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/remove_rip_address?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_set_rip_address_enabled()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: rip_rip_set_rip_address_enabled <ifname:txt> <vifname:txt> <addr:ipv4> <enabled:bool>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/set_rip_address_enabled?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3&enabled:bool=$4"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_rip_address_enabled()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: rip_rip_rip_address_enabled <ifname:txt> <vifname:txt> <addr:ipv4>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/rip_address_enabled?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_set_cost()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: rip_rip_set_cost <ifname:txt> <vifname:txt> <addr:ipv4> <cost:u32>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/set_cost?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3&cost:u32=$4"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_cost()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: rip_rip_cost <ifname:txt> <vifname:txt> <addr:ipv4>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/cost?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_set_horizon()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: rip_rip_set_horizon <ifname:txt> <vifname:txt> <addr:ipv4> <horizon:txt>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/set_horizon?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3&horizon:txt=$4"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_horizon()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: rip_rip_horizon <ifname:txt> <vifname:txt> <addr:ipv4>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/horizon?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_set_passive()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: rip_rip_set_passive <ifname:txt> <vifname:txt> <addr:ipv4> <passive:bool>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/set_passive?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3&passive:bool=$4"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_passive()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: rip_rip_passive <ifname:txt> <vifname:txt> <addr:ipv4>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/passive?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_set_accept_non_rip_requests()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: rip_rip_set_accept_non_rip_requests <ifname:txt> <vifname:txt> <addr:ipv4> <accept:bool>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/set_accept_non_rip_requests?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3&accept:bool=$4"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_accept_non_rip_requests()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: rip_rip_accept_non_rip_requests <ifname:txt> <vifname:txt> <addr:ipv4>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/accept_non_rip_requests?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_set_accept_default_route()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: rip_rip_set_accept_default_route <ifname:txt> <vifname:txt> <addr:ipv4> <accept:bool>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/set_accept_default_route?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3&accept:bool=$4"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_accept_default_route()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: rip_rip_accept_default_route <ifname:txt> <vifname:txt> <addr:ipv4>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/accept_default_route?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_set_advertise_default_route()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: rip_rip_set_advertise_default_route <ifname:txt> <vifname:txt> <addr:ipv4> <advertise:bool>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/set_advertise_default_route?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3&advertise:bool=$4"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_advertise_default_route()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: rip_rip_advertise_default_route <ifname:txt> <vifname:txt> <addr:ipv4>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/advertise_default_route?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_set_route_expiry_seconds()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: rip_rip_set_route_expiry_seconds <ifname:txt> <vifname:txt> <addr:ipv4> <t_secs:u32>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/set_route_expiry_seconds?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3&t_secs:u32=$4"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_route_expiry_seconds()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: rip_rip_route_expiry_seconds <ifname:txt> <vifname:txt> <addr:ipv4>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/route_expiry_seconds?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_set_route_deletion_seconds()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: rip_rip_set_route_deletion_seconds <ifname:txt> <vifname:txt> <addr:ipv4> <t_secs:u32>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/set_route_deletion_seconds?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3&t_secs:u32=$4"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_route_deletion_seconds()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: rip_rip_route_deletion_seconds <ifname:txt> <vifname:txt> <addr:ipv4>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/route_deletion_seconds?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_set_table_request_seconds()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: rip_rip_set_table_request_seconds <ifname:txt> <vifname:txt> <addr:ipv4> <t_secs:u32>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/set_table_request_seconds?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3&t_secs:u32=$4"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_table_request_seconds()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: rip_rip_table_request_seconds <ifname:txt> <vifname:txt> <addr:ipv4>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/table_request_seconds?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_set_unsolicited_response_min_seconds()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: rip_rip_set_unsolicited_response_min_seconds <ifname:txt> <vifname:txt> <addr:ipv4> <t_secs:u32>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/set_unsolicited_response_min_seconds?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3&t_secs:u32=$4"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_unsolicited_response_min_seconds()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: rip_rip_unsolicited_response_min_seconds <ifname:txt> <vifname:txt> <addr:ipv4>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/unsolicited_response_min_seconds?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_set_unsolicited_response_max_seconds()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: rip_rip_set_unsolicited_response_max_seconds <ifname:txt> <vifname:txt> <addr:ipv4> <t_secs:u32>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/set_unsolicited_response_max_seconds?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3&t_secs:u32=$4"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_unsolicited_response_max_seconds()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: rip_rip_unsolicited_response_max_seconds <ifname:txt> <vifname:txt> <addr:ipv4>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/unsolicited_response_max_seconds?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_set_triggered_update_min_seconds()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: rip_rip_set_triggered_update_min_seconds <ifname:txt> <vifname:txt> <addr:ipv4> <t_secs:u32>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/set_triggered_update_min_seconds?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3&t_secs:u32=$4"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_triggered_update_min_seconds()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: rip_rip_triggered_update_min_seconds <ifname:txt> <vifname:txt> <addr:ipv4>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/triggered_update_min_seconds?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_set_triggered_update_max_seconds()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: rip_rip_set_triggered_update_max_seconds <ifname:txt> <vifname:txt> <addr:ipv4> <t_secs:u32>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/set_triggered_update_max_seconds?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3&t_secs:u32=$4"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_triggered_update_max_seconds()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: rip_rip_triggered_update_max_seconds <ifname:txt> <vifname:txt> <addr:ipv4>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/triggered_update_max_seconds?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_set_interpacket_delay_milliseconds()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: rip_rip_set_interpacket_delay_milliseconds <ifname:txt> <vifname:txt> <addr:ipv4> <t_msecs:u32>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/set_interpacket_delay_milliseconds?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3&t_msecs:u32=$4"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_interpacket_delay_milliseconds()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: rip_rip_interpacket_delay_milliseconds <ifname:txt> <vifname:txt> <addr:ipv4>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/interpacket_delay_milliseconds?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_set_authentication()
{
    if [ $# -ne 5 ] ; then
        echo "Usage: rip_rip_set_authentication <ifname:txt> <vifname:txt> <addr:ipv4> <type:txt> <password:txt>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/set_authentication?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3&type:txt=$4&password:txt=$5"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_authentication()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: rip_rip_authentication <ifname:txt> <vifname:txt> <addr:ipv4>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/authentication?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_rip_address_status()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: rip_rip_rip_address_status <ifname:txt> <vifname:txt> <addr:ipv4>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/rip_address_status?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_get_counters()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: rip_rip_get_counters <ifname:txt> <vifname:txt> <addr:ipv4>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/get_counters?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_get_peers()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: rip_rip_get_peers <ifname:txt> <vifname:txt> <addr:ipv4>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/get_peers?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_get_all_peers()
{
    if [ $# -ne 0 ] ; then
        echo "Usage: rip_rip_get_all_peers"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/get_all_peers"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_get_peer_counters()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: rip_rip_get_peer_counters <ifname:txt> <vifname:txt> <addr:ipv4> <peer:ipv4>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/get_peer_counters?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3&peer:ipv4=$4"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_import_protocol_routes()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: rip_rip_import_protocol_routes <protocol_name:txt> <cost:u32> <tag:u32>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/import_protocol_routes?protocol_name:txt=$1&cost:u32=$2&tag:u32=$3"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_no_import_protocol_routes()
{
    if [ $# -ne 1 ] ; then
        echo "Usage: rip_rip_no_import_protocol_routes <protocol_name:txt>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/no_import_protocol_routes?protocol_name:txt=$1"
    call_xrl_wrapper -p all "${XRL}"
}

rip_socket4_user_recv_event()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: rip_socket4_user_recv_event <sockid:txt> <src_host:ipv4> <src_port:u32> <data:binary>"
        exit 1
    fi

    XRL="finder://rip/socket4_user/0.1/recv_event?sockid:txt=$1&src_host:ipv4=$2&src_port:u32=$3&data:binary=$4"
    call_xrl_wrapper -p all "${XRL}"
}

rip_socket4_user_connect_event()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: rip_socket4_user_connect_event <sockid:txt> <src_host:ipv4> <src_port:u32> <new_sockid:txt>"
        exit 1
    fi

    XRL="finder://rip/socket4_user/0.1/connect_event?sockid:txt=$1&src_host:ipv4=$2&src_port:u32=$3&new_sockid:txt=$4"
    call_xrl_wrapper -p all "${XRL}"
}

rip_socket4_user_error_event()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: rip_socket4_user_error_event <sockid:txt> <error:txt> <fatal:bool>"
        exit 1
    fi

    XRL="finder://rip/socket4_user/0.1/error_event?sockid:txt=$1&error:txt=$2&fatal:bool=$3"
    call_xrl_wrapper -p all "${XRL}"
}

rip_socket4_user_close_event()
{
    if [ $# -ne 2 ] ; then
        echo "Usage: rip_socket4_user_close_event <sockid:txt> <reason:txt>"
        exit 1
    fi

    XRL="finder://rip/socket4_user/0.1/close_event?sockid:txt=$1&reason:txt=$2"
    call_xrl_wrapper -p all "${XRL}"
}

