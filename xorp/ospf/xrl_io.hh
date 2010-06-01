// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2010 XORP, Inc and Others
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

// $XORP: xorp/ospf/xrl_io.hh,v 1.35 2008/11/14 12:44:19 bms Exp $

#ifndef __OSPF_XRL_IO_HH__
#define __OSPF_XRL_IO_HH__

#include "libxipc/xrl_router.hh"

#include "libfeaclient/ifmgr_xrl_mirror.hh"
#include "policy/backend/policytags.hh"

#include "io.hh"

class EventLoop;
template <typename A> class XrlIO;

/**
 * XXX - This should be moved to its own file.
 *
 * Queue route adds and deletes to the RIB.
 */
template <class A>
class XrlQueue {
public:
    XrlQueue(EventLoop& eventloop, XrlRouter& xrl_router);

    void set_io(XrlIO<A> *io) {
	_io = io;
    }

    void queue_add_route(string ribname, const IPNet<A>& net,
			 const A& nexthop, uint32_t nexthop_id, 
			 uint32_t metric, const PolicyTags& policytags);

    void queue_delete_route(string ribname, const IPNet<A>& net);

    bool busy();
private:
    static const size_t WINDOW = 100;	// Maximum number of XRLs
					// allowed in flight.

    XrlIO<A>    *_io;
    EventLoop& _eventloop;
    XrlRouter& _xrl_router;

    struct Queued {
	bool add;
	string ribname;
	IPNet<A> net;
	A nexthop;
	uint32_t nexthop_id;
	uint32_t metric;
	string comment;
	PolicyTags policytags;
    };

    deque <Queued> _xrl_queue;
    size_t _flying; //XRLs currently in flight

    /**
     * Maximum number in flight
     */
    bool maximum_number_inflight() const { return _flying >= WINDOW; }

    /**
     * Start the transmission of XRLs to tbe RIB.
     */
    void start();

    /**
     * The specialised method called by sendit to deal with IPv4/IPv6.
     *
     * @param q the queued command.
     * @param protocol "ospf"
     * @return True if the add/delete was queued.
     */
    bool sendit_spec(Queued& q, const char *protocol);

    EventLoop& eventloop() const;

    void route_command_done(const XrlError& error, const string comment);
};

/**
 * Concrete implementation of IO using XRLs.
 */
template <typename A>
class XrlIO : public IO<A>,
	      public IfMgrHintObserver,
	      public ServiceChangeObserverBase {
 public:
    XrlIO(EventLoop& eventloop, XrlRouter& xrl_router, const string& feaname,
	  const string& ribname)
	: _eventloop(eventloop),
	  _xrl_router(xrl_router),
	  _feaname(feaname),
	  _ribname(ribname),
	  _component_count(0),
	  _ifmgr(eventloop, feaname.c_str(), _xrl_router.finder_address(),
		 _xrl_router.finder_port()),
	  _rib_queue(eventloop, xrl_router)

    {
	_ifmgr.set_observer(this);
	_ifmgr.attach_hint_observer(this);
	_rib_queue.set_io(this);
	//
	// TODO: for now startup inside the constructor. Ideally, we want
	// to startup after the FEA birth event.
	//
// 	startup();
    }

    ~XrlIO() {
	//
	// TODO: for now shutdown inside the destructor. Ideally, we want
	// to shutdown gracefully before we call the destructor.
	//
// 	shutdown();

	_ifmgr.detach_hint_observer(this);
	_ifmgr.unset_observer(this);
    }

    /**
     * Startup operation.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int startup() {
	//
	// XXX: when the startup is completed,
	// IfMgrHintObserver::tree_complete() will be called.
	//
	if (_ifmgr.startup() != XORP_OK) {
	    ServiceBase::set_status(SERVICE_FAILED);
	    return (XORP_ERROR);
	}

 	register_rib();
	component_up("startup");

	return (XORP_OK);
    }


    /**
     * Shutdown operation.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int shutdown() {
	//
	// XXX: when the shutdown is completed, XrlIO::status_change()
	// will be called.
	//

	unregister_rib();
	component_down("shutdown");

	return (_ifmgr.shutdown());
    }

    /**
     * Called when internal subsystem comes up.
     */
    void component_up(string /*name*/) {
//  	fprintf(stderr, "Component: %s count %d\n", name.c_str(),
// 		_component_count + 1);
	_component_count++;
	// XXX - Should really get every component to register at
	// initialisation time and track the individual
	// status. Simpler to uncomment the printfs and track the count.
#ifdef HAVE_IPV6
	if (4 == _component_count)
	    ServiceBase::set_status(SERVICE_RUNNING);
#else
	if (3 == _component_count)
	    ServiceBase::set_status(SERVICE_RUNNING);
#endif
    }

    /**
     * Called when internal subsystem goes down.
     */
    void component_down(string /*name*/) {
//  	fprintf(stderr, "Component: %s count %d\n", name.c_str(),
// 		_component_count - 1);
	_component_count--;
	if (0 == _component_count)
	    ServiceBase::set_status(SERVICE_SHUTDOWN);
	else
	    ServiceBase::set_status(SERVICE_SHUTTING_DOWN);
    }

    /**
     * Receiver Raw frames.
     */
    void recv(const string& interface,
	      const string& vif,
	      A src,
	      A dst,
	      uint8_t ip_protocol,
	      int32_t ip_ttl,
	      int32_t ip_tos,
	      bool ip_router_alert,
	      bool ip_internet_control,
	      const vector<uint8_t>& payload);

    /**
     * Send Raw frames.
     */
    bool send(const string& interface, const string& vif, 
	      A dst, A src,
	      int ttl, uint8_t* data, uint32_t len);

    /**
     * Enable the interface/vif to receive frames.
     */
    bool enable_interface_vif(const string& interface, const string& vif);

    /**
     * Disable this interface/vif from receiving frames.
     */
    bool disable_interface_vif(const string& interface, const string& vif);

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
     * Test whether an interface/vif/address is enabled.
     *
     * @param interface the name of the interface to test.
     * @param vif the name of the vif to test.
     * @param address the address to test.
     * @return true if it exists and is enabled, otherwise false.
     */
    bool is_address_enabled(const string& interface, const string& vif,
			    const A& address) const;

    /**
     * Get all addresses associated with this interface/vif.
     *
     * @param interface the name of the interface
     * @param vif the name of the vif
     * @param addresses (out argument) list of associated addresses
     *
     * @return true if there are no errors.
     */
    bool get_addresses(const string& interface, const string& vif,
		       list<A>& addresses) const;

    /**
     * Get a link local address for this interface/vif if available.
     *
     * @param interface the name of the interface
     * @param vif the name of the vif
     * @param address (out argument) set if address is found.
     *
     * @return true if a link local address is available.
     */
    bool get_link_local_address(const string& interface, const string& vif,
				A& address);

    /**
     * Get the interface ID.
     *
     * @param interface the name of the interface.
     * @param interface_id the value if found..
     * @return true if the interface ID has been found..
     */
    bool get_interface_id(const string& interface, uint32_t& interface_id);

    /**
     * Obtain the subnet prefix length for an interface/vif/address.
     *
     * @param interface the name of the interface.
     * @param vif the name of the vif.
     * @param address the address.
     * @return the subnet prefix length for the address.
     */
    uint32_t get_prefix_length(const string& interface, const string& vif,
			       A address);

    /**
     * Obtain the MTU for an interface.
     *
     * @param the name of the interface.
     * @return the mtu for the interface.
     */
    uint32_t get_mtu(const string& interface);

    /**
     * On the interface/vif join this multicast group.
     */
    bool join_multicast_group(const string& interface, const string& vif,
			      A mcast);

    /**
     * On the interface/vif leave this multicast group.
     */
    bool leave_multicast_group(const string& interface, const string& vif,
			       A mcast);

    /**
     * Register with the RIB.
     */
    void register_rib();

    /**
     * Remove registration from the RIB.
     */
    void unregister_rib();

    void rib_command_done(const XrlError& error, bool up, const char *comment);

    /**
     * Add route to RIB.
     *
     * @param net network
     * @param nexthop
     * @param nexthop_id interface ID towards the nexthop
     * @param metric to network
     * @param equal true if this in another route to the same destination.
     * @param discard true if this is a discard route.
     * @param policytags policy info to the RIB.
     */
    bool add_route(IPNet<A> net,
		   A nexthop,
		   uint32_t nexthop_id,
		   uint32_t metric,
		   bool equal,
		   bool discard,
		   const PolicyTags& policytags);

    /**
     * Replace route in RIB.
     *
     * @param net network
     * @param nexthop
     * @param nexthop_id interface ID towards the nexthop
     * @param metric to network
     * @param equal true if this in another route to the same destination.
     * @param discard true if this is a discard route.
     * @param policytags policy info to the RIB.
     */
    bool replace_route(IPNet<A> net,
		       A nexthop,
		       uint32_t nexthop_id,
		       uint32_t metric,
		       bool equal,
		       bool discard,
		       const PolicyTags& policytags);

    /**
     * Delete route from RIB.
     */
    bool delete_route(IPNet<A> net);

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
		       ServiceStatus	new_status) {
	if (old_status == new_status)
	    return;
	if (SERVICE_RUNNING == new_status)
	    component_up(service->service_name());
	if (SERVICE_SHUTDOWN == new_status)
	    component_down(service->service_name());

    }

    /**
     * Obtain a pointer to the interface manager service base.
     *
     * @return a pointer to the interface manager service base.
     */
    const ServiceBase* ifmgr_mirror_service_base() const {
	return dynamic_cast<const ServiceBase*>(&_ifmgr);
    }

    /**
     * Obtain a reference to the interface manager's interface tree.
     *
     * @return a reference to the interface manager's interface tree.
     */
    const IfMgrIfTree& ifmgr_iftree() const { return _ifmgr.iftree(); }

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

    //
    // XRL callbacks
    //
    void send_cb(const XrlError& xrl_error, string interface, string vif);
    void enable_interface_vif_cb(const XrlError& xrl_error, string interface,
				 string vif);
    void disable_interface_vif_cb(const XrlError& xrl_error, string interface,
				  string vif);
    void join_multicast_group_cb(const XrlError& xrl_error, string interface,
				 string vif);
    void leave_multicast_group_cb(const XrlError& xrl_error, string interface,
				  string vif);

    EventLoop&		_eventloop;
    XrlRouter&		_xrl_router;
    string		_feaname;
    string		_ribname;
    uint32_t		_component_count;

    IfMgrXrlMirror	_ifmgr;
    XrlQueue<A>		_rib_queue;

    //
    // A local copy with the interface state information
    //
    IfMgrIfTree		_iftree;
};
#endif // __OSPF_XRL_IO_HH__
