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

rip_policy_backend_configure()
{
    if [ $# -ne 2 ] ; then
        echo "Usage: rip_policy_backend_configure <filter:u32> <conf:txt>"
        exit 1
    fi

    XRL="finder://rip/policy_backend/0.1/configure?filter:u32=$1&conf:txt=$2"
    call_xrl_wrapper -p all "${XRL}"
}

rip_policy_backend_reset()
{
    if [ $# -ne 1 ] ; then
        echo "Usage: rip_policy_backend_reset <filter:u32>"
        exit 1
    fi

    XRL="finder://rip/policy_backend/0.1/reset?filter:u32=$1"
    call_xrl_wrapper -p all "${XRL}"
}

rip_policy_backend_push_routes()
{
    if [ $# -ne 0 ] ; then
        echo "Usage: rip_policy_backend_push_routes"
        exit 1
    fi

    XRL="finder://rip/policy_backend/0.1/push_routes"
    call_xrl_wrapper -p all "${XRL}"
}

rip_policy_redist4_add_route4()
{
    if [ $# -ne 6 ] ; then
        echo "Usage: rip_policy_redist4_add_route4 <network:ipv4net> <unicast:bool> <multicast:bool> <nexthop:ipv4> <metric:u32> <policytags:list>"
        exit 1
    fi

    XRL="finder://rip/policy_redist4/0.1/add_route4?network:ipv4net=$1&unicast:bool=$2&multicast:bool=$3&nexthop:ipv4=$4&metric:u32=$5&policytags:list=$6"
    call_xrl_wrapper -p all "${XRL}"
}

rip_policy_redist4_delete_route4()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: rip_policy_redist4_delete_route4 <network:ipv4net> <unicast:bool> <multicast:bool>"
        exit 1
    fi

    XRL="finder://rip/policy_redist4/0.1/delete_route4?network:ipv4net=$1&unicast:bool=$2&multicast:bool=$3"
    call_xrl_wrapper -p all "${XRL}"
}

rip_redist4_add_route()
{
    if [ $# -ne 8 ] ; then
        echo "Usage: rip_redist4_add_route <dst:ipv4net> <nexthop:ipv4> <ifname:txt> <vifname:txt> <metric:u32> <admin_distance:u32> <cookie:txt> <protocol_origin:txt>"
        exit 1
    fi

    XRL="finder://rip/redist4/0.1/add_route?dst:ipv4net=$1&nexthop:ipv4=$2&ifname:txt=$3&vifname:txt=$4&metric:u32=$5&admin_distance:u32=$6&cookie:txt=$7&protocol_origin:txt=$8"
    call_xrl_wrapper -p all "${XRL}"
}

rip_redist4_delete_route()
{
    if [ $# -ne 8 ] ; then
        echo "Usage: rip_redist4_delete_route <dst:ipv4net> <nexthop:ipv4> <ifname:txt> <vifname:txt> <metric:u32> <admin_distance:u32> <cookie:txt> <protocol_origin:txt>"
        exit 1
    fi

    XRL="finder://rip/redist4/0.1/delete_route?dst:ipv4net=$1&nexthop:ipv4=$2&ifname:txt=$3&vifname:txt=$4&metric:u32=$5&admin_distance:u32=$6&cookie:txt=$7&protocol_origin:txt=$8"
    call_xrl_wrapper -p all "${XRL}"
}

rip_redist4_starting_route_dump()
{
    if [ $# -ne 1 ] ; then
        echo "Usage: rip_redist4_starting_route_dump <cookie:txt>"
        exit 1
    fi

    XRL="finder://rip/redist4/0.1/starting_route_dump?cookie:txt=$1"
    call_xrl_wrapper -p all "${XRL}"
}

rip_redist4_finishing_route_dump()
{
    if [ $# -ne 1 ] ; then
        echo "Usage: rip_redist4_finishing_route_dump <cookie:txt>"
        exit 1
    fi

    XRL="finder://rip/redist4/0.1/finishing_route_dump?cookie:txt=$1"
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

rip_rip_set_route_timeout()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: rip_rip_set_route_timeout <ifname:txt> <vifname:txt> <addr:ipv4> <t_secs:u32>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/set_route_timeout?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3&t_secs:u32=$4"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_route_timeout()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: rip_rip_route_timeout <ifname:txt> <vifname:txt> <addr:ipv4>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/route_timeout?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_set_deletion_delay()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: rip_rip_set_deletion_delay <ifname:txt> <vifname:txt> <addr:ipv4> <t_secs:u32>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/set_deletion_delay?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3&t_secs:u32=$4"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_deletion_delay()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: rip_rip_deletion_delay <ifname:txt> <vifname:txt> <addr:ipv4>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/deletion_delay?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_set_request_interval()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: rip_rip_set_request_interval <ifname:txt> <vifname:txt> <addr:ipv4> <t_secs:u32>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/set_request_interval?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3&t_secs:u32=$4"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_request_interval()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: rip_rip_request_interval <ifname:txt> <vifname:txt> <addr:ipv4>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/request_interval?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_set_update_interval()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: rip_rip_set_update_interval <ifname:txt> <vifname:txt> <addr:ipv4> <t_secs:u32>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/set_update_interval?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3&t_secs:u32=$4"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_update_interval()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: rip_rip_update_interval <ifname:txt> <vifname:txt> <addr:ipv4>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/update_interval?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_set_update_jitter()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: rip_rip_set_update_jitter <ifname:txt> <vifname:txt> <addr:ipv4> <t_jitter:u32>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/set_update_jitter?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3&t_jitter:u32=$4"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_update_jitter()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: rip_rip_update_jitter <ifname:txt> <vifname:txt> <addr:ipv4>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/update_jitter?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_set_triggered_update_delay()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: rip_rip_set_triggered_update_delay <ifname:txt> <vifname:txt> <addr:ipv4> <t_secs:u32>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/set_triggered_update_delay?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3&t_secs:u32=$4"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_triggered_update_delay()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: rip_rip_triggered_update_delay <ifname:txt> <vifname:txt> <addr:ipv4>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/triggered_update_delay?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_set_triggered_update_jitter()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: rip_rip_set_triggered_update_jitter <ifname:txt> <vifname:txt> <addr:ipv4> <t_jitter:u32>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/set_triggered_update_jitter?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3&t_jitter:u32=$4"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_triggered_update_jitter()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: rip_rip_triggered_update_jitter <ifname:txt> <vifname:txt> <addr:ipv4>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/triggered_update_jitter?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_set_interpacket_delay()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: rip_rip_set_interpacket_delay <ifname:txt> <vifname:txt> <addr:ipv4> <t_msecs:u32>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/set_interpacket_delay?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3&t_msecs:u32=$4"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_interpacket_delay()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: rip_rip_interpacket_delay <ifname:txt> <vifname:txt> <addr:ipv4>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/interpacket_delay?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_set_simple_authentication_key()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: rip_rip_set_simple_authentication_key <ifname:txt> <vifname:txt> <addr:ipv4> <password:txt>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/set_simple_authentication_key?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3&password:txt=$4"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_delete_simple_authentication_key()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: rip_rip_delete_simple_authentication_key <ifname:txt> <vifname:txt> <addr:ipv4>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/delete_simple_authentication_key?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_set_md5_authentication_key()
{
    if [ $# -ne 7 ] ; then
        echo "Usage: rip_rip_set_md5_authentication_key <ifname:txt> <vifname:txt> <addr:ipv4> <key_id:u32> <password:txt> <start_time:txt> <end_time:txt>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/set_md5_authentication_key?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3&key_id:u32=$4&password:txt=$5&start_time:txt=$6&end_time:txt=$7"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_delete_md5_authentication_key()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: rip_rip_delete_md5_authentication_key <ifname:txt> <vifname:txt> <addr:ipv4> <key_id:u32>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/delete_md5_authentication_key?ifname:txt=$1&vifname:txt=$2&addr:ipv4=$3&key_id:u32=$4"
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

rip_rip_get_all_addresses()
{
    if [ $# -ne 0 ] ; then
        echo "Usage: rip_rip_get_all_addresses"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/get_all_addresses"
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

rip_rip_redist_protocol_routes()
{
    if [ $# -ne 3 ] ; then
        echo "Usage: rip_rip_redist_protocol_routes <protocol_name:txt> <cost:u32> <tag:u32>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/redist_protocol_routes?protocol_name:txt=$1&cost:u32=$2&tag:u32=$3"
    call_xrl_wrapper -p all "${XRL}"
}

rip_rip_no_redist_protocol_routes()
{
    if [ $# -ne 1 ] ; then
        echo "Usage: rip_rip_no_redist_protocol_routes <protocol_name:txt>"
        exit 1
    fi

    XRL="finder://rip/rip/0.1/no_redist_protocol_routes?protocol_name:txt=$1"
    call_xrl_wrapper -p all "${XRL}"
}

rip_socket4_user_recv_event()
{
    if [ $# -ne 6 ] ; then
        echo "Usage: rip_socket4_user_recv_event <sockid:txt> <if_name:txt> <vif_name:txt> <src_host:ipv4> <src_port:u32> <data:binary>"
        exit 1
    fi

    XRL="finder://rip/socket4_user/0.1/recv_event?sockid:txt=$1&if_name:txt=$2&vif_name:txt=$3&src_host:ipv4=$4&src_port:u32=$5&data:binary=$6"
    call_xrl_wrapper -p all "${XRL}"
}

rip_socket4_user_inbound_connect_event()
{
    if [ $# -ne 4 ] ; then
        echo "Usage: rip_socket4_user_inbound_connect_event <sockid:txt> <src_host:ipv4> <src_port:u32> <new_sockid:txt>"
        exit 1
    fi

    XRL="finder://rip/socket4_user/0.1/inbound_connect_event?sockid:txt=$1&src_host:ipv4=$2&src_port:u32=$3&new_sockid:txt=$4"
    call_xrl_wrapper -p all "${XRL}"
}

rip_socket4_user_outgoing_connect_event()
{
    if [ $# -ne 1 ] ; then
        echo "Usage: rip_socket4_user_outgoing_connect_event <sockid:txt>"
        exit 1
    fi

    XRL="finder://rip/socket4_user/0.1/outgoing_connect_event?sockid:txt=$1"
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

rip_socket4_user_disconnect_event()
{
    if [ $# -ne 1 ] ; then
        echo "Usage: rip_socket4_user_disconnect_event <sockid:txt>"
        exit 1
    fi

    XRL="finder://rip/socket4_user/0.1/disconnect_event?sockid:txt=$1"
    call_xrl_wrapper -p all "${XRL}"
}

