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

// $XORP: xorp/rip/xrl_port_manager.hh,v 1.3 2004/01/13 20:38:07 hodson Exp $

#ifndef __RIP_XRL_PORT_MANAGER_HH__
#define __RIP_XRL_PORT_MANAGER_HH__

#include "libxorp/service.hh"
#include "libfeaclient/ifmgr_xrl_mirror.hh"
#include "port_manager.hh"

#include "port.hh"

class XrlRouter;

template <typename A>
class XrlPortManager
    : public PortManagerBase<A>,
      public IfMgrHintObserver,
      public ServiceBase,
      public ServiceChangeObserverBase
{
public:
    inline XrlPortManager(System<A>& 		system,
			  XrlRouter& 		xr,
			  IfMgrXrlMirror& 	ifm)
	: PortManagerBase<A>(system), ServiceBase("RIP Port Manager"),
	  _xr(xr), _ifm(ifm)
    {
	_ifm.attach_hint_observer(this);
    }

    ~XrlPortManager();

    /**
     * Request start up of instance.
     */
    void startup();

    /**
     * Request shutdown of instance..
     */
    void shutdown();

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
     * @param src_addr source address of packet.
     * @param src_port source port of packet.
     * @param pdata packet data.
     *
     * @return true if packet delivered, false if the owner of the
     * sockid can not be found.
     */
    bool deliver_packet(const string& 		sockid,
			const A& 		src_addr,
			uint16_t 		src_port,
			const vector<uint8_t>& 	pdata);

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

private:
    XrlPortManager(const XrlPortManager&);		// not implemented
    XrlPortManager& operator=(const XrlPortManager&);	// not implemented

protected:
    XrlRouter& 				_xr;	// XrlRouter
    IfMgrXrlMirror& 			_ifm;	// Interface Mirror
    map<ServiceBase*, Port<A>*>	_dead_ports; // Ports awaiting io shutdown
};

#endif // __RIP_XRL_PORT_MANAGER_HH__
