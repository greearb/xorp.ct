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

// $XORP: xorp/rib/rib_manager.hh,v 1.7 2003/03/19 09:05:19 pavlin Exp $

#ifndef __RIB_RIB_MANAGER_HH__
#define __RIB_RIB_MANAGER_HH__

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/exceptions.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/xlog.h"

#include "libxipc/xrl_std_router.hh"

#include "rib.hh"
#include "rib_client.hh"
#include "register_server.hh"
#include "vifmanager.hh"
#include "xrl_target.hh"

/**
 * @short Main top-level class containing RIBs and main eventloop.
 *
 * The single RibManager class instance is the top-level class in the
 * RIB process from which everything else is built and run.  It
 * contains the four RIBs for IPv4 unicast routes, IPv4 multicast
 * routes, IPv6 unicast routes and IPv6 multicast routes.  It also
 * contains the RIB's main eventloop.  
 */
class RibManager {
public:
    /**
     * RibManager constructor
     * 
     * @param event_loop the event loop to user.
     * @param xrl_std_router the XRL router to use.
     */
    RibManager(EventLoop& event_loop, XrlStdRouter& xrl_std_router);

    /**
     * Run the RIB process's main event loop.  This should only return
     * when the RIB process is terminating.  
     */
    void run_event_loop();

    /**
     * new_vif is called to inform all the RIBs that a new virtual
     * interface has been created.
     *
     * @param vifname the name of the new VIF.
     * @param vif the Vif class instance holding information about the
     * new VIF.
     * @param err reference to string in which to store the
     * human-readable error message in case anything goes wrong.  Used
     * for debugging purposes.
     */
    int new_vif(const string& vifname, const Vif& vif, string& err);

    /**
     * delete_vif is called to inform all the RIBs that a virtual
     * interface that they previously knew about has been deleted.
     *
     * @param vifname the name of the VIF that was deleted.
     * @param err reference to string in which to store the
     * human-readable error message in case anything goes wrong.  Used
     * for debugging purposes.  
     */
    int delete_vif(const string& vifname, string& err);

    /**
     * add_vif_addr is called to inform all the RIBs that a new IPv4
     * address has been added to a virtual interface.
     *
     * @param vifname the name of the VIF that the address was added to.
     * @param addr the new address.
     * @param net the subnet (masked address) that the new address
     * resides on.
     * @param err reference to string in which to store the
     * human-readable error message in case anything goes wrong.  Used
     * for debugging purposes.  
     */
    int add_vif_addr(const string& vifname, 
		     const IPv4& addr,
		     const IPNet<IPv4>& net,
		     string& err);

    /**
     * delete_vif_addr is called to inform all the RIBs that an IPv4
     * address that they previously know about has been deleted from a
     * specific VIF.
     *
     * @param vifname the name of the VIF that the address was deleted from.
     * @param addr the address that was deleted.
     * @param err reference to string in which to store the
     * human-readable error message in case anything goes wrong.  Used
     * for debugging purposes.  
     */
    int delete_vif_addr(const string& vifname, 
			const IPv4& addr,
			string& err);

    /**
     * add_vif_addr is called to inform all the RIBs that a new IPv6
     * address has been added to a virtual interface.
     *
     * @param vifname the name of the VIF that the address was added to.
     * @param addr the new address.
     * @param net the subnet (masked address) that the new address
     * resides on.
     * @param err reference to string in which to store the
     * human-readable error message in case anything goes wrong.  Used
     * for debugging purposes.
     */
    int add_vif_addr(const string& vifname,
		     const IPv6& addr,
		     const IPNet<IPv6>& net,
		     string& err);

    /**
     * delete_vif_addr is called to inform all the RIBs that an IPv6
     * address that they previously know about has been deleted from a
     * specific VIF.
     *
     * @param vifname the name of the VIF that the address was deleted from.
     * @param addr the address that was deleted.
     * @param err reference to string in which to store the
     * human-readable error message in case anything goes wrong.  Used
     * for debugging purposes.  
     */
    int delete_vif_addr(const string& vifname, 
			const IPv6& addr,
			string& err);

    void set_rib_clients_enabled(bool en);
    bool rib_clients_enabled() const;
    
private:
    EventLoop&		_event_loop;	// The event loop to use
    XrlStdRouter&	_xrl_router;	// The XRL router to use
    RibClient		_rib_client;	// To send URIB routes to clients
    RibClient		_mrib4_client;	// To send IPv4 MRIB routes to clients 
    RibClient		_mrib6_client;	// To send IPv6 MRIB routes to clients
    RegisterServer	_rserv;		// To notify clients about route change
    
    RIB<IPv4>		_urib4;		// The IPv4 unicast RIB
    RIB<IPv4>		_mrib4;		// The IPv4 multicast RIB
    RIB<IPv6>		_urib6;		// The IPv6 unicast RIB
    RIB<IPv6>		_mrib6;		// The IPv6 multicast RIB
    
    VifManager		_vifmanager;	// The VIF manager
    XrlRibTarget	_xrt;
};

#endif // __RIB_RIB_MANAGER_HH__
