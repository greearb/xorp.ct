// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

// $XORP: xorp/bgp/bgp.hh,v 1.23 2004/03/24 19:34:30 atanu Exp $

#ifndef __BGP_MAIN_HH__
#define __BGP_MAIN_HH__

#include "libxorp/eventloop.hh"
#include "libxorp/status_codes.h"
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
     *
     * @return true on success
     */
    bool originate_route(const IPv4Net& nlri,
			 const IPv4& next_hop,
			 const bool& unicast,
			 const bool& multicast);

    /**
     * Originate an IPv6 route
     *
     * @param nlri subnet to announce
     * @param next_hop to forward to
     * @param unicast if true install in unicast routing table
     * @param multicast if true install in multicast routing table
     *
     * @return true on success
     */
    bool originate_route(const IPv6Net& nlri,
			 const IPv6& next_hop,
			 const bool& unicast,
			 const bool& multicast);

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

    bool get_route_list_start4(uint32_t& token);
    bool get_route_list_start6(uint32_t& token);
    
    bool get_route_list_next4(
			      // Input values, 
			      const uint32_t&	token, 
			      // Output values, 
			      IPv4&	peer_id, 
			      IPv4Net& net, 
			      uint32_t& origin, 
			      vector<uint8_t>& aspath, 
			      IPv4& nexthop, 
			      int32_t& med, 
			      int32_t& localpref, 
			      int32_t& atomic_agg, 
			      vector<uint8_t>& aggregator, 
			      int32_t& calc_localpref, 
			      vector<uint8_t>& attr_unknown,
			      bool& best);
    bool get_route_list_next6(
			      // Input values, 
			      const uint32_t&	token, 
			      // Output values, 
			      IPv4&	peer_id, 
			      IPv6Net& net, 
			      uint32_t& origin, 
			      vector<uint8_t>& aspath, 
			      IPv6& nexthop, 
			      int32_t& med, 
			      int32_t& localpref, 
			      int32_t& atomic_agg, 
			      vector<uint8_t>& aggregator, 
			      int32_t& calc_localpref, 
			      vector<uint8_t>& attr_unknown,
			      bool& best);

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

    template <class A>
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

    XrlBgpTarget *_xrl_target;
    RibIpcHandler *_rib_ipc_handler;
    LocalData _local_data;
    XrlStdRouter *_xrl_router;
    static EventLoop _eventloop;
    ProcessWatch *_process_watch;
};

// template <typename A>
// struct NameOf {
//     static const char* get() { return "Unknown"; }
// };
// template <> const char* NameOf<IPv4>::get() { return "IPv4"; }
// template <> const char* NameOf<IPv6>::get() { return "IPv6"; }

#endif // __BGP_MAIN_HH__
