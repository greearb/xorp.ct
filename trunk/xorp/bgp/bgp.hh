// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2004 International Computer Science Institute
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software")
// to deal in the Software without restriction, subject to the conditions
// listed in the XORP LICENSE file. These conditions include: you must
// preserve this copyright notice, and you cannot mention the copyright
// holders in advertising related to the Software without their permission.
// The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
// notice is a summary of the XORP LICENSE file; the license in that file is
// legally binding.

// $XORP: xorp/bgp/bgp.hh,v 1.30 2004/09/17 13:50:53 abittau Exp $

#ifndef __BGP_MAIN_HH__
#define __BGP_MAIN_HH__

#include "libxorp/eventloop.hh"
#include "libxorp/status_codes.h"
#include "libxorp/profile.hh"
#include "libxipc/xrl_std_router.hh"

#include "socket.hh"
#include "packet.hh"

#include "peer.hh"
#include "peer_list.hh"
#include "plumbing.hh"
#include "iptuple.hh"
#include "path_attribute.hh"
#include "peer_handler.hh"
#include "process_watch.hh"

#include "policy/backend/policy_filters.hh"

class XrlBgpTarget;

class BGPMain {
public:
    BGPMain();
    ~BGPMain();

    /**
     * Get the process status
     */
    ProcessStatus status(string& reason);

    /**
     * Set the local configuration.
     *
     * @param as as number.
     *
     * @param id router id.
     */
    void local_config(const uint32_t& as, const IPv4& id);

    /**
     * attach peer to peerlist
     *
     * @param p BGP peer.
     */
    void attach_peer(BGPPeer *p);

    /**
     * detach peer from the peerlist.
     *
     * @param p BGP peer.
     */
    void detach_peer(BGPPeer *p);

    /**
     * Find peer with this iptuple
     *
     * @param search iptuple.
     *
     * @return A pointer to a peer if one is found NULL otherwise.
     */
    BGPPeer *find_peer(const Iptuple& search);

    /**
     * create a new peer and attach it to the peerlist.
     *
     * @param pd BGP peer data.
     *
     * @return true on success
     */
    bool create_peer(BGPPeerData *pd);

    /**
     * delete peer tear down connection and remove for peerlist.

	XrlBgpTarget xbt(bgp.get_router(), bgp);
     *
     * @param iptuple iptuple.
     *
     * @return true on success
     */
    bool delete_peer(const Iptuple& iptuple);

    /**
     * enable peer
     *
     * @param iptuple iptuple.
     *
     * @return true on success
     */
    bool enable_peer(const Iptuple& iptuple);

    /**
     * disable peer
     *
     * @param iptuple iptuple.
     *
     * @return true on success
     */
    bool disable_peer(const Iptuple& iptuple);

    /**
     * Set peer state.
     *
     * @param iptuple iptuple.
     * @param state should the peering be enable or disabled.
     *
     * @ return true on success.
     */
    bool set_peer_state(const Iptuple& iptuple, bool state);

    /**
     * Set peer TCP-MD5 password.
     *
     * @param iptuple iptuple.
     * @param password The password to use for TCP-MD5 authentication;
     * if this is the empty string, then authentication will be disabled.
     *
     * @return true on success.
     */
    bool set_peer_md5_password(const Iptuple& iptuple, const string& password);

    /**
     * Activate peer.
     *
     * Enable the peering based on the peer state.
     *
     * @param iptuple iptuple.
     *
     * @ return true on success.
     */
    bool activate(const Iptuple& iptuple);

    /*
     * Set next hop rewrite filter.
     *
     * @param iptuple iptuple.
     * @param next_hop next hop value zero to clear filter.
     *
     * @return true on success
     */
    bool next_hop_rewrite_filter(const Iptuple& iptuple, const IPv4& next_hop);

    bool get_peer_list_start(uint32_t& token);

    bool get_peer_list_next(const uint32_t& token, 
			    IPv4& local_ip, 
			    uint32_t& local_port, 
			    IPv4& peer_ip, 
			    uint32_t& peer_port);

    bool get_peer_id(const Iptuple& iptuple, IPv4& peer_id);
    bool get_peer_status(const Iptuple& iptuple,  uint32_t& peer_state, 
			 uint32_t& admin_status);
    bool get_peer_negotiated_version(const Iptuple& iptuple, 
				     int32_t& neg_version);
    bool get_peer_as(const Iptuple& iptuple,   uint32_t& peer_as);
    bool get_peer_msg_stats(const Iptuple& iptuple, 
			    uint32_t& in_updates, 
			    uint32_t& out_updates, 
			    uint32_t& in_msgs, 
			    uint32_t& out_msgs, 
			    uint16_t& last_error, 
			    uint32_t& in_update_elapsed);
    bool get_peer_established_stats(const Iptuple& iptuple,  
				    uint32_t& transitions, 
				    uint32_t& established_time);
    bool get_peer_timer_config(const Iptuple& iptuple,
			       uint32_t& retry_interval, 
			       uint32_t& hold_time, 
			       uint32_t& keep_alive, 
			       uint32_t& hold_time_configured, 
			       uint32_t& keep_alive_configured, 
			       uint32_t& min_as_origination_interval,
			       uint32_t& min_route_adv_interval);

    bool register_ribname(const string& name);

    void main_loop();

    /**
     * shutdown BGP cleanly
     */
    void terminate() { _exit_loop = true; }
    bool run() {return !_exit_loop;}

    int create_listener(const Iptuple& iptuple);
    LocalData *get_local_data();
    void start_server(const Iptuple& iptuple);
    void stop_server(const Iptuple& iptuple);
    /**
     * Stop listening for incoming connections.
     */
    void stop_all_servers();

    /**
     * Originate an IPv4 route
     *
     * @param nlri subnet to announce
     * @param next_hop to forward to
     * @param unicast if true install in unicast routing table
     * @param multicast if true install in multicast routing table
     * @param policytags policy-tags associated with route.
     *
     * @return true on success
     */
    bool originate_route(const IPv4Net& nlri,
			 const IPv4& next_hop,
			 const bool& unicast,
			 const bool& multicast,
			 const PolicyTags& policytags);

    /**
     * Originate an IPv6 route
     *
     * @param nlri subnet to announce
     * @param next_hop to forward to
     * @param unicast if true install in unicast routing table
     * @param multicast if true install in multicast routing table
     * @param policytags policy-tags associated with route.
     *
     * @return true on success
     */
    bool originate_route(const IPv6Net& nlri,
			 const IPv6& next_hop,
			 const bool& unicast,
			 const bool& multicast,
			 const PolicyTags& policytags);

    /**
     * Withdraw an IPv4 route
     *
     * @param nlri subnet to withdraw
     * @param unicast if true withdraw from unicast routing table
     * @param multicast if true withdraw from multicast routing table
     *
     * @return true on success
     */
    bool withdraw_route(const IPv4Net&	nlri,
			const bool& unicast,
			const bool& multicast) const;

    /**
     * Withdraw an IPv6 route
     *
     * @return true on success
     */
    bool withdraw_route(const IPv6Net&	nlri,
			const bool& unicast,
			const bool& multicast) const;

    template <typename A>
    bool get_route_list_start(uint32_t& token,
			       const bool& unicast,
			       const bool& multicast);

    template <typename A>
    bool get_route_list_next(
			      // Input values, 
			      const uint32_t&	token, 
			      // Output values, 
			      IPv4&  peer_id, 
			      IPNet<A>& net, 
			      uint32_t& origin, 
			      vector<uint8_t>& aspath, 
			      A& nexthop, 
			      int32_t& med, 
			      int32_t& localpref, 
			      int32_t& atomic_agg, 
			      vector<uint8_t>& aggregator, 
			      int32_t& calc_localpref, 
			      vector<uint8_t>& attr_unknown,
			      bool& best,
			      bool& unicast,
			      bool& multicast);

    bool rib_client_route_info_changed4(
					// Input values,
					const IPv4&	addr,
					const uint32_t&	prefix_len,
					const IPv4&	nexthop,
					const uint32_t&	metric);

    bool rib_client_route_info_changed6(
					// Input values,
					const IPv6&	addr,
					const uint32_t&	prefix_len,
					const IPv6&	nexthop,
					const uint32_t&	metric);

    bool rib_client_route_info_invalid4(
					// Input values,
					const IPv4&	addr,
					const uint32_t&	prefix_len);

    bool rib_client_route_info_invalid6(
					// Input values,
					const IPv6&	addr,
					const uint32_t&	prefix_len);

    /**
     * set parameter
     *
     * Typically called via XRL's to set which parameters we support
     * per peer.
     *
     * @param iptuple iptuple
     * @param parameter we are setting for this peer.
     */
    bool set_parameter(
				// Input values,
		       const Iptuple& iptuple,
		       const string& parameter);

    /**
     * Originally inserted for testing. However, now used by all the
     * "rib_client_route_info_*" methods.
     */
    BGPPlumbing *plumbing_unicast() const { return _plumbing_unicast; }
    BGPPlumbing *plumbing_multicast() const { return _plumbing_multicast; }

    XrlStdRouter *get_router() { return _xrl_router; }
    EventLoop& eventloop() { return _eventloop; }
    XrlBgpTarget *get_xrl_target() { return _xrl_target; }

    /**
     * Call via XrlBgpTarget when the finder reports that a process
     * has started.
     * 
     * @param target_class Class of process that has started.
     * @param target_instance Instance name of process that has started.
     */
    void notify_birth(const string& target_class,
		      const string& target_instance) {
	_process_watch->birth(target_class, target_instance);
    }

    /**
     * Call via XrlBgpTarget when the finder reports that a process
     * has terminated.
     * 
     * @param target_class Class of process that has terminated.
     * @param target_instance Instance name of process that has terminated.
     */
    void notify_death(const string& target_class,
		      const string& target_instance) {
	_process_watch->death(target_class, target_instance);
    }

    /**
     * @return Return true when all the processes that BGP is
     * dependent on are ready.
     */
    bool processes_ready() {
	return _process_watch->ready();
    }

    /**
     * @return Return the bgp mib name.
     */
    string bgp_mib_name() const {
	return "bgp4_mib";
    }

    /**
     * Check to see if the bgp snmp entity is running.
     */
    bool do_snmp_trap() const {
	return _process_watch->target_exists(bgp_mib_name());
    }

    /**
     * To be called when the finder dies.
     */
    void finder_death(const char *file, const int lineno) {
	_process_watch->finder_death(file, lineno);
    }

    /**
     * Configure a policy filter
     *
     * @param filter Id of filter to configure.
     * @param conf Configuration of filter.
     */
    void configure_filter(const uint32_t& filter, const string& conf);

    /**
     * Reset a policy filter.
     *
     * @param filter Id of filter to reset.
     */
    void reset_filter(const uint32_t& filter);

    /**
     * Push routes through policy filters for re-filtering.
     */
    void push_routes();
    
    /**
     * @return a pointer to the profiler.
     */
    Profile& profile() {return _profile;}

protected:
private:
    /**
     * Store the socket descriptor and iptuple together.
     */
    struct Server {
	Server(int fd, Iptuple iptuple) : _serverfd(fd) {
	    _tuples.push_back(iptuple);
	}
	Server(const Server& rhs) {
	    _serverfd = rhs._serverfd;
	    _tuples = rhs._tuples;
	}
	Server operator=(const Server& rhs) {
	    if (&rhs == this)
		return *this;

	    _serverfd = rhs._serverfd;
	    _tuples = rhs._tuples;

	    return *this;
	}
	int _serverfd;
	list <Iptuple> _tuples;
    };

    list<Server> _serverfds;

    /**
     * Callback method called when a connection attempt is made.
     */
    void connect_attempt(int fd, SelectorMask m,
			 struct in_addr laddr, uint16_t lport);

    template <typename A>
    void extract_attributes(// Input values, 
			    const PathAttributeList<A>& attributes, 
			    // Output values, 
			    uint32_t& origin, 
			    vector<uint8_t>& aspath, 
			    A& nexthop,
			    int32_t& med, 
			    int32_t& localpref, 
			    int32_t& atomic_agg, 
			    vector<uint8_t>& aggregator, 
			    int32_t& calc_localpref, 
			    vector<uint8_t>& attr_unknown);


    bool _exit_loop;
    BGPPeerList *_peerlist;

    /**
    * Unicast Routing Table. SAFI = 1.
    */
    BGPPlumbing *_plumbing_unicast;
    NextHopResolver<IPv4> *_next_hop_resolver_ipv4;

    /**
    * Multicast Routing Table. SAFI = 2.
    */
    BGPPlumbing *_plumbing_multicast;
    NextHopResolver<IPv6> *_next_hop_resolver_ipv6;

    /**
     * Token generator to map between unicast and multicast.
     *
     */
    struct RoutingTableToken {
	RoutingTableToken() : _last(0)
	{}

	uint32_t create(uint32_t& internal_token, const bool& unicast,
			const bool& multicast) {

	    while(_tokens.find(_last) != _tokens.end())
		_last++;
		
	    _tokens[_last] = WhichTable(internal_token, unicast, multicast);

	    return _last;
	}
	
	bool lookup(uint32_t& token, bool& unicast, bool& multicast) {
	    map <uint32_t, WhichTable>::iterator i;

	    i = _tokens.find(token);
	    if (i == _tokens.end())
		return false;

	    WhichTable t = i->second;

	    token = t._token;
	    unicast = t._unicast;
	    multicast = t._multicast;

	    return true;
	}

	void erase(uint32_t& token) {
	    _tokens.erase(token);
	}

    private:
	struct WhichTable {
	    WhichTable() {}
	    WhichTable(uint32_t token, bool unicast, bool multicast)
		: _token(token), _unicast(unicast), _multicast(multicast)
							       {}
	    uint32_t _token;
	    bool _unicast;
	    bool _multicast;
	};

	map <uint32_t, WhichTable> _tokens;
	uint32_t _last;
    };

    template <typename A> RoutingTableToken& get_token_table();

    RoutingTableToken _table_ipv4;
    RoutingTableToken _table_ipv6;

    XrlBgpTarget *_xrl_target;
    RibIpcHandler *_rib_ipc_handler;
    LocalData _local_data;
    XrlStdRouter *_xrl_router;
    static EventLoop _eventloop;
    ProcessWatch *_process_watch;
    PolicyFilters _policy_filters;
    Profile _profile;
};

template <typename A>
bool
BGPMain::get_route_list_start(uint32_t& token,
				 const bool& unicast,
				 const bool& multicast)
{
    A dummy;

    if (unicast) {
	token = _plumbing_unicast->create_route_table_reader(dummy);
    } else if (multicast) {
	token = _plumbing_multicast->create_route_table_reader(dummy);
    } else {
	XLOG_ERROR("Must specify at least one of unicast or multicast");
	return false;
    }

    token = get_token_table<A>().create(token, unicast, multicast);

    return true;
}

template <typename A>
void
BGPMain::extract_attributes(// Input values,
			    const PathAttributeList<A>& attributes,
			    // Output values,
			    uint32_t& origin,
			    vector<uint8_t>& aspath,
			    A& nexthop,
			    int32_t& med,
			    int32_t& localpref,
			    int32_t& atomic_agg,
			    vector<uint8_t>& aggregator,
			    int32_t& calc_localpref,
			    vector<uint8_t>& attr_unknown)
{
    origin = attributes.origin();
    attributes.aspath().encode_for_mib(aspath);
    nexthop = attributes.nexthop();

    const MEDAttribute* med_att = attributes.med_att();
    if (med_att) {
	med = (int32_t)med_att->med();
	if (med < 0) {
	    med = 0x7ffffff;
	    XLOG_WARNING("MED truncated in MIB from %u to %u\n",
			 med_att->med(), med);
	}
    } else {
	med = -1;
    }

    const LocalPrefAttribute* local_pref_att
	= attributes.local_pref_att();
    if (local_pref_att) {
	localpref = (int32_t)local_pref_att->localpref();
	if (localpref < 0) {
	    localpref = 0x7ffffff;
	    XLOG_WARNING("LOCAL_PREF truncated in MIB from %u to %u\n",
			 local_pref_att->localpref(), localpref);
	}
    } else {
	localpref = -1;
    }

    if (attributes.atomic_aggregate_att())
	atomic_agg = 2;
    else
	atomic_agg = 1;

    const AggregatorAttribute* agg_att
	= attributes.aggregator_att();
    if (agg_att) {
	aggregator.resize(6);
	agg_att->route_aggregator().copy_out(&aggregator[0]);
	agg_att->aggregator_as().copy_out(&aggregator[4]);
    } else {
	assert(aggregator.size()==0);
    }

    calc_localpref = 0;
    attr_unknown.resize(0);
}

template <typename A>
bool
BGPMain::get_route_list_next(
			      // Input values,
			      const uint32_t& token,
			      // Output values,
			      IPv4& peer_id,
			      IPNet<A>& net,
			      uint32_t& origin,
			      vector<uint8_t>& aspath,
			      A& nexthop,
			      int32_t& med,
			      int32_t& localpref,
			      int32_t& atomic_agg,
			      vector<uint8_t>& aggregator,
			      int32_t& calc_localpref,
			      vector<uint8_t>& attr_unknown,
			      bool& best,
			      bool& unicast_global,
			      bool& multicast_global)
{
    bool unicast, multicast;
    uint32_t internal_token, global_token;
    internal_token = global_token = token;

    A dummy;

    if (!get_token_table<A>().lookup(internal_token, unicast, multicast))
	return false;

    const SubnetRoute<A>* route;
    if (unicast) {
	if (_plumbing_unicast->read_next_route(internal_token, route,
					       peer_id)) {
	    net = route->net();
	    extract_attributes(*route->attributes(),
			       origin, aspath, nexthop, med, localpref,
			       atomic_agg, aggregator, calc_localpref,
			       attr_unknown);
	    best = route->is_winner();
	    unicast_global = true;
	    multicast_global = false;
	    return true;
	}

	// We may have been asked for the unicast and multicast
	// routing tables. In which case once we have completed the
	// unicast routing table move onto providing the multicast
	// table.
	get_token_table<A>().erase(global_token);
	if (multicast) {
	    internal_token =
		_plumbing_multicast->create_route_table_reader(dummy);
	    global_token = get_token_table<A>().
		create(internal_token, false, true);
	}
    }
    if (multicast) {
	if (_plumbing_multicast->read_next_route(internal_token, route,
						 peer_id)) {
	    net = route->net();
	    extract_attributes(*route->attributes(),
			       origin, aspath, nexthop, med, localpref,
			       atomic_agg, aggregator, calc_localpref,
			       attr_unknown);
	    best = route->is_winner();
	    unicast_global = false;
	    multicast_global = true;
	    return true;
	}
	get_token_table<A>().erase(global_token);
    }
    return false;
}

// template <typename A>
// struct NameOf {
//     static const char* get() { return "Unknown"; }
// };
// template <> const char* NameOf<IPv4>::get() { return "IPv4"; }
// template <> const char* NameOf<IPv6>::get() { return "IPv6"; }

#endif // __BGP_MAIN_HH__
