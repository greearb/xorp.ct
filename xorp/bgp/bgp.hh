// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net


#ifndef __BGP_MAIN_HH__
#define __BGP_MAIN_HH__

#include "libxorp/status_codes.h"
#include "libxipc/xrl_std_router.hh"
#include "libxorp/profile.hh"

#include "socket.hh"
#include "packet.hh"

#include "peer.hh"
#include "peer_list.hh"
#include "plumbing.hh"
#include "iptuple.hh"
#include "path_attribute.hh"
#include "peer_handler.hh"
#include "process_watch.hh"

#include "libfeaclient/ifmgr_xrl_mirror.hh"
#include "policy/backend/version_filters.hh"

class EventLoop;
class XrlBgpTarget;

class BGPMain : public ServiceBase,
		public IfMgrHintObserver,
		public ServiceChangeObserverBase {
public:
    BGPMain(EventLoop& eventloop);
    ~BGPMain();

    /**
     * Get the process status
     */
    ProcessStatus status(string& reason);

    /**
     * Startup operation.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int startup();

    /**
     * Shutdown operation.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int shutdown();

    /**
     * A method that should be called when an internal subsystem comes up.
     *
     * @param component_name the name of the component.
     */
    void component_up(const string& component_name);

    /**
     * A method that should be called when an internal subsystem goes down.
     *
     * @param component_name the name of the component.
     */
    void component_down(const string& component_name);

    /**
     * Test whether an interface is enabled.
     *
     * @param interface the name of the interface to test.
     * @return true if it exists and is enabled, otherwise false.
     */
    bool is_interface_enabled(const string& interface) const;

    /**
     * Test whether an interface/vif is enabled.
     *
     * @param interface the name of the interface to test.
     * @param vif the name of the vif to test.
     * @return true if it exists and is enabled, otherwise false.
     */
    bool is_vif_enabled(const string& interface, const string& vif) const;

    /**
     * Test whether an IPv4 interface/vif/address is enabled.
     *
     * @param interface the name of the interface to test.
     * @param vif the name of the vif to test.
     * @param address the address to test.
     * @return true if it exists and is enabled, otherwise false.
     */
    bool is_address_enabled(const string& interface, const string& vif,
			    const IPv4& address) const;

    typedef XorpCallback2<void, const string&,
			  bool>::RefPtr InterfaceStatusCb;
    typedef XorpCallback3<void, const string&, const string&,
			  bool>::RefPtr VifStatusCb;
    typedef XorpCallback5<void, const string&, const string&, const IPv4&,
			  uint32_t, bool>::RefPtr AddressStatus4Cb;

    /**
     * Add a callback for tracking the interface status.
     *
     * The callback will be invoked whenever the status of the interface
     * is changed from disabled to enabled or vice-versa.
     *
     * @param cb the callback to register.
     */
    void register_interface_status(InterfaceStatusCb cb) {
	_interface_status_cb = cb;
    }

    /**
     * Add a callback for tracking the interface/vif status.
     *
     * The callback will be invoked whenever the status of the interface/vif
     * is changed from disabled to enabled or vice-versa.
     *
     * @param cb the callback to register.
     */
    void register_vif_status(VifStatusCb cb) {
	_vif_status_cb = cb;
    }

    /**
     * Add a callback for tracking the IPv4 interface/vif/address status.
     *
     * The callback will be invoked whenever the status of the tuple
     * (interface, vif, address) is changed from disabled to enabled
     * or vice-versa.
     *
     * @param cb the callback to register.
     */
    void register_address_status(AddressStatus4Cb cb) {
	_address_status4_cb = cb;
    }

    /**
     * Obtain the subnet prefix length for an IPv4 interface/vif/address.
     *
     * @param interface the name of the interface.
     * @param vif the name of the vif.
     * @param address the address.
     * @return the subnet prefix length for the address.
     */
    uint32_t get_prefix_length(const string& interface, const string& vif,
			       const IPv4& address);

    /**
     * Obtain the MTU for an interface.
     *
     * @param the name of the interface.
     * @return the mtu for the interface.
     */
    uint32_t get_mtu(const string& interface);

    /**
     * Is the address one of this routers interface addresses?
     */
    bool interface_address4(const IPv4& address) const;

    /**
     * Obtain the prefix length for a particular IPv4 address.
     *
     * @param address the address to search for.
     * @param prefix_len the return-by-reference prefix length
     * for @ref address.
     * @return true if the address belongs to this router, otherwise false.
     */
    bool interface_address_prefix_len4(const IPv4& address,
				       uint32_t& prefix_len) const;

    /**
     * Set the local configuration.
     *
     * @param as as number.
     *
     * @param id router id.
     *
     * @param use_4byte_asnums indicates we should send 4 byte AS
     * numbers to all peers that are 4byte capable.
     */
    void local_config(const uint32_t& as, const IPv4& id, 
		      bool use_4byte_asnums);

    /**
     * Set or disable the confederation identifier.
     */
    void set_confederation_identifier(const uint32_t& as, bool disable);

    /**
     * Set the cluster ID and enable or disable route reflection.
     */
    void set_cluster_id(const IPv4& cluster_id, bool disable);

    /**
     * Set the route flap damping parameters.
     */
    void set_damping(uint32_t half_life,uint32_t max_suppress,uint32_t reuse,
		     uint32_t suppress, bool disable);

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
     * attach peer to deleted peerlist
     *
     * @param p BGP peer.
     */
    void attach_deleted_peer(BGPPeer *p);

    /**
     * detach peer from the deleted peerlist.
     *
     * @param p BGP peer.
     */
    void detach_deleted_peer(BGPPeer *p);

    /**
     * Find peer with this iptuple from the list provided
     *
     * @param search iptuple.
     * @param peers list to search.
     *
     * @return A pointer to a peer if one is found NULL otherwise.
     */
    BGPPeer *find_peer(const Iptuple& search, list<BGPPeer *>& peers);

    /**
     * Find peer with this iptuple
     *
     * @param search iptuple.
     *
     * @return A pointer to a peer if one is found NULL otherwise.
     */
    BGPPeer *find_peer(const Iptuple& search);

    /**
     * Find peer with this iptuple on the deleted peer list.
     *
     * @param search iptuple.
     *
     * @return A pointer to a peer if one is found NULL otherwise.
     */
    BGPPeer *find_deleted_peer(const Iptuple& search);

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
     * Drop this peering and if it was configured up allow it attempt
     * a new peering.
     *
     * @param iptuple iptuple.
     *
     * @return true on success
     */
    bool bounce_peer(const Iptuple& iptuple);

    /**
     * One of the local IP addresses of this router has changed. Where
     * a change can be an addition or removal or a change in the link
     * status. If the provided address matches any of the local ip
     * addresses of any of the peerings unconditionally bounce the
     * peering. Unconditional bouncing of the peering is all that is
     * required if a link has gone down the old session will be
     * dropped and the new one will fail. If the link has just come up
     * then a session will be made.
     */
    void local_ip_changed(string local_address);

    /**
     * Change one of the tuple settings of this peering.
     *
     * @param iptuple original tuple.
     * @param iptuple new tuple.
     *
     * @return true on success
     */
    bool change_tuple(const Iptuple& iptuple, const Iptuple& nptuple);

    /**
     * Find the tuple that has this peer address and both ports are
     * 179. This is a hack as at the moment of writing the rtrmgr
     * can't send both the old and new state of a variable.
     *
     * @param peer_addr of tuple.
     * @param otuple the tuple if one is found.
     *
     * @return true if a tuple is matched.
     */
    bool find_tuple_179(string peer_addr, Iptuple& otuple);

    /**
     * Change the local IP address of this peering.
     *
     * @param iptuple iptuple.
     * @param local_ip new IP value
     * @param local_dev new Interface value.
     *
     * @return true on success
     */
    bool change_local_ip(const Iptuple& iptuple, const string& local_ip, const string& local_dev);

    /**
     * Change the local IP port of this peering.
     *
     * @param iptuple iptuple.
     * @param local_port new value.
     *
     * @return true on success
     */
    bool change_local_port(const Iptuple& iptuple, uint32_t local_port);

    /**
     * Change the peer IP port of this peering.
     *
     * @param iptuple iptuple.
     * @param peer_port new value.
     *
     * @return true on success
     */
    bool change_peer_port(const Iptuple& iptuple, uint32_t peer_port);

    /**
     * set peer as
     *
     * @param iptuple iptuple.
     * @param peer_as new value.
     *
     * @return true on success
     */
    bool set_peer_as(const Iptuple& iptuple, uint32_t peer_as);

    /**
     * set holdtime
     *
     * @param iptuple iptuple.
     * @param holdtime new value.
     *
     * @return true on success
     */
    bool set_holdtime(const Iptuple& iptuple, uint32_t holdtime);

    /**
     * set delay open time
     *
     * @param iptuple iptuple.
     * @param delay_open_time new value.
     *
     * @return true on success
     */
    bool set_delay_open_time(const Iptuple& iptuple, uint32_t delay_open_time);

    /**
     * set route reflector client
     *
     * @param iptuple iptuple.
     * @param rr true if this peer is a route reflector client.
     *
     * @return true on success
     */
    bool set_route_reflector_client(const Iptuple& iptuple, bool rr);

    /**
     * set route confederation member
     *
     * @param iptuple iptuple.
     * @param conf true if this peer is a confederation member.
     *
     * @return true on success
     */
    bool set_confederation_member(const Iptuple& iptuple, bool conf);

    /**
     * set prefix limit
     *
     * @param maximum number of prefixes
     * @param state true if the prefix limit is being enforced
     *
     * @return true on success
     */
    bool set_prefix_limit(const Iptuple& iptuple, 
			  uint32_t maximum, bool state);

    /**
     * set IPv4 next-hop.
     *
     * @param iptuple iptuple.
     * @param next-hop
     *
     * @return true on success
     */
    bool set_nexthop4(const Iptuple& iptuple, const IPv4& next_hop);

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

    /**
     * Activate all peers.
     *
     * @return true on success
     */
    bool activate_all_peers();

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
			    string& local_ip, 
			    uint32_t& local_port, 
			    string& peer_ip, 
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

    XorpFd create_listener(const Iptuple& iptuple);
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

    template <typename A>
    bool get_route_list_start(uint32_t& token,
			      const IPNet<A>& prefix,
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

    bool rib_client_route_info_invalid4(
					// Input values,
					const IPv4&	addr,
					const uint32_t&	prefix_len);

    /**
     * set parameter
     *
     * Typically called via XRL's to set which parameters we support
     * per peer.
     *
     * @param iptuple iptuple
     * @param parameter
     * @param toggle enable or disable parameter
     */
    bool set_parameter(
				// Input values,
		       const Iptuple& iptuple,
		       const string& parameter,
		       const bool toggle);

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

/** IPv6 stuff */
#ifdef HAVE_IPV6

    /**
     * Test whether an IPv6 interface/vif/address is enabled.
     *
     * @param interface the name of the interface to test.
     * @param vif the name of the vif to test.
     * @param address the address to test.
     * @return true if it exists and is enabled, otherwise false.
     */
    bool is_address_enabled(const string& interface, const string& vif,
			    const IPv6& address) const;

    typedef XorpCallback5<void, const string&, const string&, const IPv6&,
			  uint32_t, bool>::RefPtr AddressStatus6Cb;

    /**
     * Add a callback for tracking the IPv6 interface/vif/address status.
     *
     * The callback will be invoked whenever the status of the tuple
     * (interface, vif, address) is changed from disabled to enabled
     * or vice-versa.
     *
     * @param cb the callback to register.
     */
    void register_address_status(AddressStatus6Cb cb) {
	_address_status6_cb = cb;
    }

    /**
     * Obtain the subnet prefix length for an IPv6 interface/vif/address.
     *
     * @param interface the name of the interface.
     * @param vif the name of the vif.
     * @param address the address.
     * @return the subnet prefix length for the address.
     */
    uint32_t get_prefix_length(const string& interface, const string& vif,
			       const IPv6& address);

    /**
     * Is the address one of this routers interface addresses?
     */
    bool interface_address6(const IPv6& address) const;

    /**
     * Obtain the prefix length for a particular IPv6 address.
     *
     * @param address the address to search for.
     * @param prefix_len the return-by-reference prefix length
     * for @ref address.
     * @return true if the address belongs to this router, otherwise false.
     */
    bool interface_address_prefix_len6(const IPv6& address,
				       uint32_t& prefix_len) const;

    /**
     * set IPv6 next-hop.
     *
     * @param iptuple iptuple.
     * @param next-hop
     *
     * @return true on success
     */
    bool set_nexthop6(const Iptuple& iptuple, const IPv6& next_hop);

    /**
     * get IPv6 next-hop.
     *
     * @param iptuple iptuple.
     * @param next-hop
     *
     * @return true on success
     */
    bool get_nexthop6(const Iptuple& iptuple, IPv6& next_hop);

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
     * Withdraw an IPv6 route
     *
     * @return true on success
     */
    bool withdraw_route(const IPv6Net&	nlri,
			const bool& unicast,
			const bool& multicast) const;

    bool rib_client_route_info_changed6(
					// Input values,
					const IPv6&	addr,
					const uint32_t&	prefix_len,
					const IPv6&	nexthop,
					const uint32_t&	metric);

    bool rib_client_route_info_invalid6(
					// Input values,
					const IPv6&	addr,
					const uint32_t&	prefix_len);

#endif //ipv6
    
#ifndef XORP_DISABLE_PROFILE
    /**
     * @return a reference to the profiler.
     */
    Profile& profile() {return _profile;}
#endif

protected:
private:
    /**
     * A method invoked when the status of a service changes.
     *
     * @param service the service whose status has changed.
     * @param old_status the old status.
     * @param new_status the new status.
     */
    void status_change(ServiceBase*	service,
		       ServiceStatus	old_status,
		       ServiceStatus	new_status);

    /**
     * Obtain a pointer to the interface manager service base.
     *
     * @return a pointer to the interface manager service base.
     */
    const ServiceBase* ifmgr_mirror_service_base() const {
	return dynamic_cast<const ServiceBase*>(_ifmgr);
    }

    /**
     * Obtain a reference to the interface manager's interface tree.
     *
     * @return a reference to the interface manager's interface tree.
     */
    const IfMgrIfTree& ifmgr_iftree() const { return _ifmgr->iftree(); }

    /**
     * An IfMgrHintObserver method invoked when the initial interface tree
     * information has been received.
     */
    void tree_complete();

    /**
     * An IfMgrHintObserver method invoked whenever the interface tree
     * information has been changed.
     */
    void updates_made();

    /**
     * Callback method that is invoked when the status of an address changes.
     */
    void address_status_change4(const string& interface, const string& vif,
				const IPv4& source, uint32_t prefix_len,
				bool state);

    /**
     * Store the socket descriptor and iptuple together.
     */
    struct Server {
	Server(XorpFd fd, const Iptuple& iptuple) : _serverfd(fd) {
	    _tuples.push_back(iptuple);
	}
	Server(const Server& rhs) {
	    _serverfd = rhs._serverfd;
	    _tuples = rhs._tuples;
	}
	Server& operator=(const Server& rhs) {
	    if (&rhs == this)
		return *this;

	    _serverfd = rhs._serverfd;
	    _tuples = rhs._tuples;

	    return *this;
	}
	string str() {
	    return c_format("fd(%s)", _serverfd.str().c_str());

	}
	XorpFd _serverfd;
	list <Iptuple> _tuples;
    };

    list<Server> _serverfds;

    /**
     * Callback method called when a connection attempt is made.
     */
    void connect_attempt(XorpFd fd, IoEventType type, string laddr, uint16_t lport);

    template <typename A>
    void extract_attributes(// Input values, 
			    PAListRef<A> attributes, 
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


    EventLoop& _eventloop;
    bool _exit_loop;
    BGPPeerList *_peerlist;		// List of current BGP peers.
    BGPPeerList *_deleted_peerlist;	// List of deleted BGP peers.

    /**
    * Unicast Routing Table. SAFI = 1.
    */
    BGPPlumbing *_plumbing_unicast;
    NextHopResolver<IPv4> *_next_hop_resolver_ipv4;

    /**
    * Multicast Routing Table. SAFI = 2.
    */
    BGPPlumbing *_plumbing_multicast;

    /**
     * Token generator to map between unicast and multicast.
     */
    template <typename A>
    class RoutingTableToken {
    public:
	RoutingTableToken() : _last(0)
	{}

	uint32_t create(uint32_t& internal_token, const IPNet<A>& prefix,
			const bool& unicast, const bool& multicast) {

	    while(_tokens.find(_last) != _tokens.end())
		_last++;
		
	    _tokens.insert(make_pair(_last, WhichTable(internal_token, prefix,
						       unicast, multicast)));

	    return _last;
	}
	
	bool lookup(uint32_t& token, IPNet<A>& prefix,
		    bool& unicast, bool& multicast) {
	    typename map <uint32_t, WhichTable>::iterator i;

	    i = _tokens.find(token);
	    if (i == _tokens.end())
		return false;

	    WhichTable t = i->second;

	    token = t._token;
	    prefix = t._prefix;
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
	    WhichTable(uint32_t token, IPNet<A> prefix,
		       bool unicast, bool multicast)
		: _token(token), _prefix(prefix),
		  _unicast(unicast), _multicast(multicast)
							       {}
	    uint32_t _token;
	    IPNet<A> _prefix;
	    bool _unicast;
	    bool _multicast;
	};

	map <uint32_t, WhichTable> _tokens;
	uint32_t _last;
    };

    template <typename A> RoutingTableToken<A>& get_token_table();

    RoutingTableToken<IPv4> _table_ipv4;

    XrlBgpTarget *_xrl_target;
    RibIpcHandler *_rib_ipc_handler;
    AggregationHandler *_aggregation_handler;
    LocalData *_local_data;
    XrlStdRouter *_xrl_router;
    ProcessWatch *_process_watch;
    VersionFilters _policy_filters;
#ifndef XORP_DISABLE_PROFILE
    Profile _profile;
#endif

    size_t		_component_count;
    IfMgrXrlMirror*	_ifmgr;
    bool		_is_ifmgr_ready;
    IfMgrIfTree		_iftree;	// The interface state information

    InterfaceStatusCb	_interface_status_cb;
    VifStatusCb		_vif_status_cb;
    AddressStatus4Cb	_address_status4_cb;

    map<IPv4, uint32_t> _interfaces_ipv4;	// IPv4 interface addresses

    bool _first_policy_push;	     	// Don't form peerings until the
					// first policy push is seen.

#ifdef HAVE_IPV6

    /**
     * Callback method that is invoked when the status of an address changes.
     */
    void address_status_change6(const string& interface, const string& vif,
				const IPv6& source, uint32_t prefix_len,
				bool state);
    NextHopResolver<IPv6> *_next_hop_resolver_ipv6;
    RoutingTableToken<IPv6> _table_ipv6;
    AddressStatus6Cb	_address_status6_cb;
    map<IPv6, uint32_t> _interfaces_ipv6;	// IPv6 interface addresses

#endif //ipv6
};

template <typename A>
bool
BGPMain::get_route_list_start(uint32_t& token,
			      const IPNet<A>& prefix,
			      const bool& unicast,
			      const bool& multicast)
{
    if (unicast) {
	token = _plumbing_unicast->create_route_table_reader(prefix);
    } else if (multicast) {
	token = _plumbing_multicast->create_route_table_reader(prefix);
    } else {
	XLOG_ERROR("Must specify at least one of unicast or multicast");
	return false;
    }

    token = get_token_table<A>().create(token, prefix, unicast, multicast);

    return true;
}

template <typename A>
void
BGPMain::extract_attributes(// Input values,
			    PAListRef<A> attributes,
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
    FastPathAttributeList<A> fpa_list(attributes);
    origin = fpa_list.origin();
    fpa_list.aspath().encode_for_mib(aspath);
    nexthop = fpa_list.nexthop();

    const MEDAttribute* med_att = fpa_list.med_att();
    if (med_att) {
	med = (int32_t)med_att->med();
	if (med < 0) {
	    med = 0x7ffffff;
	    XLOG_WARNING("MED truncated in MIB from %u to %u\n",
			 XORP_UINT_CAST(med_att->med()),
			 XORP_UINT_CAST(med));
	}
    } else {
	med = -1;
    }

    const LocalPrefAttribute* local_pref_att
	= fpa_list.local_pref_att();
    if (local_pref_att) {
	localpref = (int32_t)local_pref_att->localpref();
	if (localpref < 0) {
	    localpref = 0x7ffffff;
	    XLOG_WARNING("LOCAL_PREF truncated in MIB from %u to %u\n",
			 XORP_UINT_CAST(local_pref_att->localpref()),
			 XORP_UINT_CAST(localpref));
	}
    } else {
	localpref = -1;
    }

    if (fpa_list.atomic_aggregate_att())
	atomic_agg = 2;
    else
	atomic_agg = 1;

    const AggregatorAttribute* agg_att
	= fpa_list.aggregator_att();
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
    IPNet<A> prefix;
    bool unicast = false, multicast = false;
    uint32_t internal_token, global_token;
    internal_token = global_token = token;

    if (!get_token_table<A>().lookup(internal_token, prefix,
				     unicast, multicast))
	return false;

    const SubnetRoute<A>* route;
    if (unicast) {
	if (_plumbing_unicast->read_next_route(internal_token, route,
					       peer_id)) {
	    net = route->net();
	    extract_attributes(route->attributes(),
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
		_plumbing_multicast->create_route_table_reader(prefix);
	    global_token = get_token_table<A>().
		create(internal_token, prefix, false, true);
	}
    }
    if (multicast) {
	if (_plumbing_multicast->read_next_route(internal_token, route,
						 peer_id)) {
	    net = route->net();
	    extract_attributes(route->attributes(),
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
