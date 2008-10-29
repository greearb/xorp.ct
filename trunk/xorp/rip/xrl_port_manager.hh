// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

// $XORP: xorp/rip/xrl_port_manager.hh,v 1.20 2008/10/02 21:58:19 bms Exp $

#ifndef __RIP_XRL_PORT_MANAGER_HH__
#define __RIP_XRL_PORT_MANAGER_HH__

#include "libxorp/service.hh"
#include "libfeaclient/ifmgr_xrl_mirror.hh"
#include "port_manager.hh"

#include "port.hh"
#include "trace.hh"

class XrlRouter;

/**
 * @short Class for managing RIP Ports and their transport systems.
 *
 * The XrlPortManager class instantiates RIP Port instances and their
 * transport system.  The methods add_rip_address and
 * remove_rip_address cause @ref Port objects and @ref XrlPortIO
 * objects to be instantiated and destructed.
 *
 * The deliver_packet method forwards an arriving packet to the
 * appropriate @ref XrlPortIO object.
 */
template <typename A>
class XrlPortManager
    : public PortManagerBase<A>,
      public IfMgrHintObserver,
      public ServiceBase,
      public ServiceChangeObserverBase
{
public:
    XrlPortManager(System<A>& 		system,
		   XrlRouter& 		xr,
		   IfMgrXrlMirror& 	ifm)
	: PortManagerBase<A>(system, ifm.iftree()),
	  ServiceBase("RIP Port Manager"),
	  _xr(xr), _ifm(ifm)
    {
	_ifm.attach_hint_observer(this);
    }

    ~XrlPortManager();

    /**
     * Request start up of instance.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int startup();

    /**
     * Request shutdown of instance.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int shutdown();

    /**
     * Request the addition of an address to run RIP on.  If the
     * address is known to exist on the specified interface and vif, a
     * request is sent to the FEA to create an appropriately
     * bound RIP socket.
     *
     * @param interface to run RIP on.
     * @param vif virtual interface to run RIP on.
     * @param addr address to run RIP on.
     *
     * @return true on success or if address is already running RIP,
     * false on failure.
     */
    bool add_rip_address(const string&	ifname,
			 const string&	vifname,
			 const A&	addr);

    /**
     * Remove an address running RIP.
     *
     * @param interface to run RIP on.
     * @param vif virtual interface to run RIP on.
     * @param addr address to run RIP on.
     *
     * @return true on success or if address is not running RIP, false on
     * otherwise.
     */
    bool remove_rip_address(const string&	ifname,
			    const string&	vifname,
			    const A&		addr);

    /**
     * Deliver packet to RIP port associated with socket id that
     * received the packet.
     *
     * @param sockid unique socket identifier.
     * @param ifname the interface name the packet arrived on, if known.
     * If unknown, then it is an empty string.
     * @param vifname the vif name the packet arrived on, if known.
     * If unknown, then it is an empty string.
     * @param src_addr source address of packet.
     * @param src_port source port of packet.
     * @param pdata packet data.
     *
     * @return true if packet delivered, false if the owner of the
     * sockid can not be found.
     */
    bool deliver_packet(const string& 		sockid,
			const string&		ifname,
			const string&		vifname,
			const A& 		src_addr,
			uint16_t 		src_port,
			const vector<uint8_t>& 	pdata);

    /**
     * Find RIP port associated with interface, vif, address tuple.
     *
     * @return pointer to port on success, 0 if port could not be found.
     */
    Port<A>* find_port(const string&	ifname,
		       const string&	vifname,
		       const A&		addr);
    /**
     * Find RIP port associated with interface, vif, address tuple.
     *
     * @return pointer to port on success, 0 if port could not be found.
     */
    const Port<A>* find_port(const string&	ifname,
			     const string&	vifname,
			     const A&		addr) const;

    /**
     * Determine if rip address is up.  The result is based on the current
     * state of information from the FEA.
     *
     * @return true if the address is up, false if the address is not up
     * or does not exist.
     */
    bool underlying_rip_address_up(const string&	ifname,
				   const string&	vifname,
				   const A&		addr) const;

    /**
     * Determine if rip address exists.  The result is based on the
     * current state of information from the FEA.
     *
     * @return true if the address is up, false if the address is not up
     * or does not exist.
     */
    bool underlying_rip_address_exists(const string&	ifname,
				       const string&	vifname,
				       const A&		addr) const;

    Trace& packet_trace() { return _trace; }
protected:
    //
    // IfMgrHintObserver methods
    //
    void tree_complete();
    void updates_made();

    //
    // ServiceChangeObserverBase methods
    // - used for observing status changes of XrlPortIO objects instantiated
    //   by XrlPortManager instance.
    //
    void status_change(ServiceBase*	service,
		       ServiceStatus	old_status,
		       ServiceStatus 	new_status);

    //
    // Attempt to start next io handler.  Walk list of ports, check
    // none are in state SERVICE_STARTING and call start on first found to be
    // in state SERVICE_READY.
    //
    // We start 1 at a time to avoid races trying to create
    // sockets with the fea.
    //
    void try_start_next_io_handler();

private:
    XrlPortManager(const XrlPortManager&);		// not implemented
    XrlPortManager& operator=(const XrlPortManager&);	// not implemented

protected:
    XrlRouter& 				_xr;	// XrlRouter
    IfMgrXrlMirror& 			_ifm;	// Interface Mirror
    map<ServiceBase*, Port<A>*>	_dead_ports; // Ports awaiting io shutdown

private:
	Trace _trace;
};

#endif // __RIP_XRL_PORT_MANAGER_HH__
