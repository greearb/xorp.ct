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

// $XORP: xorp/ospf/xrl_io.hh,v 1.17 2005/11/05 20:43:49 atanu Exp $

#ifndef __OSPF_XRL_IO_HH__
#define __OSPF_XRL_IO_HH__

#include "libxipc/xrl_router.hh"

#include "libfeaclient/ifmgr_xrl_mirror.hh"
#include "policy/backend/policytags.hh"

#include "io.hh"

class EventLoop;

/**
 * XXX - This should be moved to its own file.
 *
 * Queue route adds and deletes to the RIB.
 */
template <class A>
class XrlQueue {
public:
    XrlQueue(EventLoop& eventloop, XrlRouter& xrl_router);

    void queue_add_route(string ribname, const IPNet<A>& net,
			 const A& nexthop, uint32_t metric,
			 const PolicyTags& policytags);

    void queue_delete_route(string ribname, const IPNet<A>& net);

    bool busy();
private:
    static const size_t WINDOW = 100;	// Maximum number of XRLs
					// allowed in flight.

    EventLoop& _eventloop;
    XrlRouter& _xrl_router;

    struct Queued {
	bool add;
	string ribname;
	IPNet<A> net;
	A nexthop;
	uint32_t metric;
	string comment;
	PolicyTags policytags;
    };

    deque <Queued> _xrl_queue;
    size_t _flying; //XRLs currently in flight

    /**
     * Maximum number in flight
     */
    inline bool maximum_number_inflight() const {
	return _flying >= WINDOW;
    }

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

    inline EventLoop& eventloop() const;

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
	  _class_name(xrl_router.class_name()),
	  _instance_name(xrl_router.instance_name()),
	  _feaname(feaname),
	  _ribname(ribname),
	  _component_count(0),
	  _ifmgr(eventloop, feaname.c_str(), _xrl_router.finder_address(),
		 _xrl_router.finder_port()),
	  _rib_queue(eventloop, xrl_router)

    {
	_ifmgr.set_observer(this);
	_ifmgr.attach_hint_observer(this);

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
     * @return true on success, false on failure.
     */
    bool startup() {
	//
	// XXX: when the startup is completed,
	// IfMgrHintObserver::tree_complete() will be called.
	//
	if (_ifmgr.startup() != true) {
	    ServiceBase::set_status(SERVICE_FAILED);
	    return (false);
	}

 	register_rib();
	component_up("startup");

	return (true);
    }


    /**
     * Shutdown operation.
     *
     * @return true on success, false on failure.
     */
    bool shutdown() {
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
// 	printf("Component: %s count %d\n", name.c_str(), _component_count + 1);
	_component_count++;
	// XXX - Should really get every component to register at
	// initialisation time and track the individual
	// status. Simpler to uncomment the printfs and track the count.
	if (4 == _component_count)
	    ServiceBase::set_status(SERVICE_RUNNING);
    }

    /**
     * Called when internal subsystem goes down.
     */
    void component_down(string /*name*/) {
// 	printf("Component: %s count %d\n", name.c_str(), _component_count - 1);
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
	      uint32_t ip_protocol,
	      int32_t ip_ttl,
	      int32_t ip_tos,
	      bool ip_router_alert,
	      const vector<uint8_t>& payload);

    /**
     * Send Raw frames.
     */
    bool send(const string& interface, const string& vif, 
	      A dst, A src,
	      uint8_t* data, uint32_t len);

    /**
     * Enable the interface/vif to receive frames.
     */
    bool enable_interface_vif(const string& interface, const string& vif);

    /**
     * Disable this interface/vif from receiving frames.
     */
    bool disable_interface_vif(const string& interface, const string& vif);

    /**
     * Test whether this interface is enabled.
     *
     * @return true if it exists and is enabled, otherwise false.
     */
    bool is_interface_enabled(const string& interface) const;

    /**
     * Test whether this interface/vif is enabled.
     *
     * @return true if it exists and is enabled, otherwise false.
     */
    bool is_vif_enabled(const string& interface, const string& vif) const;

    /**
     * Test whether this interface/vif/address is enabled.
     *
     * @return true if it exists and is enabled, otherwise false.
     */
    bool is_address_enabled(const string& interface, const string& vif,
			    const A& address) const;

    /**
     * @return prefix length for this address.
     */
    uint32_t get_prefix_length(const string& interface, const string& vif,
			       A address);

    /**
     * @return the mtu for this interface.
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
     * @param metric to network
     * @param equal true if this in another route to the same destination.
     * @param discard true if this is a discard route.
     * @param policytags policy info to the RIB.
     */
    bool add_route(IPNet<A> net,
		   A nexthop,
		   uint32_t metric,
		   bool equal,
		   bool discard,
		   const PolicyTags& policytags);

    /**
     * Replace route in RIB.
     *
     * @param net network
     * @param nexthop
     * @param metric to network
     * @param equal true if this in another route to the same destination.
     * @param discard true if this is a discard route.
     * @param policytags policy info to the RIB.
     */
    bool replace_route(IPNet<A> net,
		       A nexthop,
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
     * @param old_status the old status
     * @param new_status the new status.
     */
    void status_change(ServiceBase*  service,
		       ServiceStatus old_status,
		       ServiceStatus new_status) {
	if (old_status == new_status)
	    return;
	if (SERVICE_RUNNING == new_status)
	    component_up(service->service_name());
	if (SERVICE_SHUTDOWN == new_status)
	    component_down(service->service_name());

    }
    const ServiceBase* ifmgr_mirror_service_base() const {
	return dynamic_cast<const ServiceBase*>(&_ifmgr);
    }
    const IfMgrIfTree& ifmgr_iftree() const { return _ifmgr.iftree(); }

    //
    // IfMgrHintObserver methods
    //
    void tree_complete();
    void updates_made();

    //
    // XRL allbacks
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
    string		_class_name;
    string		_instance_name;
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
