// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/contrib/olsr/xrl_io.hh,v 1.2 2008/07/23 05:09:54 pavlin Exp $

#ifndef __OLSR_XRL_IO_HH__
#define __OLSR_XRL_IO_HH__

#include "libxipc/xrl_router.hh"

#include "libfeaclient/ifmgr_xrl_mirror.hh"
#include "policy/backend/policytags.hh"

#include "io.hh"
#include "xrl_queue.hh"

class EventLoop;
class XrlPort;

typedef list<XrlPort*>			XrlPortList;
typedef map<ServiceBase*, XrlPort*>	XrlDeadPortMap;

/**
 * @short Concrete implementation of IO using XRLs
 */
class XrlIO : public IO,
	      public IfMgrHintObserver,
	      public ServiceChangeObserverBase {
 public:

    /**
     * Construct an XrlIO instance.
     *
     * @param eventloop the event loop for the OLSR process.
     * @param xrl_router the name of the XRL router instance.
     * @param feaname the name of the FEA XRL target.
     * @param ribname the name of the RIB XRL target.
     */
    XrlIO(EventLoop& eventloop, XrlRouter& xrl_router,
	  const string& feaname, const string& ribname);

    /**
     * Destroy an XrlIO instance.
     */
    ~XrlIO();

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
     * Called when internal subsystem comes up.
     *
     * @param name the name of the affected subsystem
     */
    void component_up(string name);

    /**
     * Called when internal subsystem goes down.
     *
     * @param name the name of the affected subsystem
     */
    void component_down(string name);

    /**
     * Receive UDP datagrams. Specific to XrlIO.
     *
     * @param sockid the id of the socket.
     * @param interface the interface where received.
     * @param vif the vif where received.
     * @param src the source port of the UDP datagram.
     * @param sport the source address of the UDP datagram.
     * @param payload the datagram payload.
     */
    void receive(const string& sockid,
	const string& interface, const string& vif,
	const IPv4& src, const uint16_t& sport,
	const vector<uint8_t>& payload);

    /**
     * Send a UDP datagram on a specific link.
     *
     * @param interface the interface to send from.
     * @param vif the vif to send from.
     * @param src the IPv4 address to send from.
     * @param sport the UDP port to send from.
     * @param dst the IPv4 address to send to.
     * @param dport the UDP port to send to.
     * @param data the datagram payload.
     * @param len the length of the buffer @param data
     * @return true if the datagram was queued successfully,
     *         otherwise false.
     */
    bool send(const string& interface, const string& vif,
	const IPv4& src, const uint16_t& sport,
	const IPv4& dst, const uint16_t& dport,
	uint8_t* data, const uint32_t& len);

    /**
     * Enable the interface/vif to receive frames.
     * XXX
     *
     * @param interface the name of the interface to enable.
     * @param vif the name of the vif to enable.
     * @return true if the interface/vif was enabled successfully,
     *         otherwise false.
     */
    bool enable_interface_vif(const string& interface, const string& vif);

    /**
     * Disable this interface/vif from receiving frames.
     * XXX
     *
     * @param interface the name of the interface to disable.
     * @param vif the name of the vif to disable.
     * @return true if the interface/vif was disabled successfully,
     *         otherwise false.
     */
    bool disable_interface_vif(const string& interface, const string& vif);

    /**
     * Enable an IPv4 address and port for OLSR datagram reception and
     * transmission.
     *
     * @param interface the interface to enable.
     * @param vif the vif to enable.
     * @param address the address to enable.
     * @param port the port to enable.
     * @param all_nodes_address the address to transmit to.
     * @return true if the address was enabled, otherwise false.
     */
    bool enable_address(const string& interface, const string& vif,
	const IPv4& address, const uint16_t& port,
	const IPv4& all_nodes_address);

    /**
     * Disable an IPv4 address and port for OLSR datagram reception.
     *
     * @param interface the interface to disable.
     * @param vif the vif to disable.
     * @param address the address to disable.
     * @param port the port to disable.
     * @return true if the address was disabled, otherwise false.
     */
    bool disable_address(const string& interface, const string& vif,
	const IPv4& address, const uint16_t& port);

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
     * Test whether this interface/vif is broadcast capable.
     *
     * @param interface the interface to test.
     * @param vif the vif to test.
     * @return true if it is broadcast capable, otherwise false.
     */
    bool is_vif_broadcast_capable(const string& interface,
	const string& vif);

    /**
     * Test whether this interface/vif is multicast capable.
     *
     * @param interface the interface to test.
     * @param vif the vif to test.
     * @return true if it is multicast capable, otherwise false.
     */
    bool is_vif_multicast_capable(const string& interface,
	const string& vif);

    /**
     * @short Return true if the given vif is a loopback vif.
     */
    bool is_vif_loopback(const string& interface, const string& vif);

    /**
     * Test whether an interface/vif/address is enabled.
     *
     * @param interface the name of the interface to test.
     * @param vif the name of the vif to test.
     * @param address the address to test.
     * @return true if it exists and is enabled, otherwise false.
     */
    bool is_address_enabled(const string& interface, const string& vif,
			    const IPv4& address) const;

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
		       list<IPv4>& addresses) const;

    /**
     * Get the broadcast address associated with this vif.
     *
     * @param interface the name of the interface
     * @param vif the name of the vif
     * @param address (out argument) "all nodes" address
     *
     * @return true if there are no errors.
     */
    bool get_broadcast_address(const string& interface,
	const string& vif, IPv4& address) const;

    /**
     * Get the broadcast address associated with this IPv4 address.
     *
     * @param interface the name of the interface
     * @param vif the name of the vif
     * @param address IPv4 binding address
     * @param bcast_address (out argument) primary broadcast address
     * @return true if there are no errors.
     */
    bool get_broadcast_address(const string& interface,
			       const string& vif,
			       const IPv4& address,
			       IPv4& bcast_address) const;

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
			        IPv4 address);

    /**
     * Obtain the MTU for an interface.
     *
     * @param interface the name of the interface.
     * @return the mtu for the interface.
     */
    uint32_t get_mtu(const string& interface);

    /**
     * Register with the RIB.
     */
    void register_rib();

    /**
     * Remove registration from the RIB.
     */
    void unregister_rib();

    /**
     * Callback method to: signal that an XRL command which has been
     * sent to the RIB has returned.
     *
     * @param error the XRL command return code.
     * @param up indicates if the RIB component has been brought up or down.
     * @param comment text description of operation being performed.
     */
    void rib_command_done(const XrlError& error, bool up,
			  const char *comment);

    /**
     * Add route to RIB.
     *
     * @param net network
     * @param nexthop
     * @param nexthop_id interface ID towards the nexthop
     * @param metric to network
     * @param policytags policy info to the RIB.
     * @return true if the route add was queued successfully,
     *         otherwise false.
     */
    bool add_route(IPv4Net net,
		   IPv4 nexthop,
		   uint32_t nexthop_id,
		   uint32_t metric,
		   const PolicyTags& policytags);

    /**
     * Replace route in RIB.
     *
     * @param net network
     * @param nexthop
     * @param nexthop_id interface ID towards the nexthop
     * @param metric to network
     * @param policytags policy info to the RIB.
     * @return true if the route replacement was queued successfully,
     *         otherwise false.
     */
    bool replace_route(IPv4Net net,
		       IPv4 nexthop,
		       uint32_t nexthop_id,
		       uint32_t metric,
		       const PolicyTags& policytags);

    /**
     * Delete route from RIB.
     *
     * @param net the destination to delete route for.
     * @return true if the route delete was queued successfully,
     *         otherwise false.
     */
    bool delete_route(IPv4Net net);

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
     * @return a pointer to the interface manager service base.
     */
    const ServiceBase* ifmgr_mirror_service_base() const {
	return dynamic_cast<const ServiceBase*>(&_ifmgr);
    }

    /**
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

    /**
     * Callback method to: signal that the XRL command to send
     * a UDP datagram has returned.
     *
     * @param xrl_error the XRL command return code.
     */
    void send_cb(const XrlError& xrl_error);

    /**
     * Find OLSR port associated with interface, vif, address tuple.
     *
     * @return pointer to port on success, 0 if port could not be found.
     */
    XrlPort* find_port(const string&	ifname,
		       const string&	vifname,
		       const IPv4&	addr);
    /**
     * Find OLSR port associated with interface, vif, address tuple.
     *
     * @return pointer to port on success, 0 if port could not be found.
     */
    const XrlPort* find_port(const string&	ifname,
			     const string&	vifname,
			     const IPv4&	addr) const;

    /**
     * @return pointer to list of active XrlPorts.
     */
    inline XrlPortList& ports() { return _ports; }

    /**
     * @return pointer to list of active XrlPorts.
     */
    inline const XrlPortList& ports() const { return _ports; }

    /**
     * Gradually start each XrlPort to avoid races with the FEA.
     */
    void try_start_next_port();

private:
    EventLoop&		_eventloop;
    XrlRouter&		_xrl_router;
    string		_feaname;
    string		_ribname;
    uint32_t		_component_count;

    /**
     * @short libfeaclient wrapper.
     */
    IfMgrXrlMirror	_ifmgr;

    /**
     * @short local copy of interface state obtained from libfeaclient.
     */
    IfMgrIfTree		_iftree;

    /**
     * @short Queue of RIB add/delete XRL commands.
     */
    XrlQueue		_rib_queue;

    /**
     * @short List of active XrlPorts.
     */
    XrlPortList		_ports;

    /**
     * @short XrlPorts awaiting I/O shutdown.
     */
    XrlDeadPortMap	_dead_ports;
};

#endif // __OLSR_XRL_IO_HH__
