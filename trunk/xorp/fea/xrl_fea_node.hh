// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007-2008 International Computer Science Institute
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

// $XORP: xorp/fea/xrl_fea_node.hh,v 1.12 2008/01/04 03:15:51 pavlin Exp $


#ifndef __FEA_XRL_FEA_NODE_HH__
#define __FEA_XRL_FEA_NODE_HH__


//
// FEA (Forwarding Engine Abstraction) with XRL front-end node implementation.
//

#include "libxorp/xorp.h"

#include "libxipc/xrl_std_router.hh"

#include "cli/xrl_cli_node.hh"

#include "fea_node.hh"
#include "libfeaclient_bridge.hh"
#include "xrl_fea_io.hh"
#include "xrl_fea_target.hh"
#include "xrl_io_link_manager.hh"
#include "xrl_io_ip_manager.hh"
#include "xrl_io_tcpudp_manager.hh"
#include "xrl_mfea_node.hh"

class EventLoop;

/**
 * @short FEA (Forwarding Engine Abstraction) node class with XRL front-end.
 * 
 * There should be one node per FEA instance.
 */
class XrlFeaNode {
public:
    /**
     * Constructor.
     *
     * @param eventloop the event loop to use.
     * @param xrl_fea_targetname the XRL targetname of the FEA.
     * @param xrl_finder_targetname the XRL targetname of the Finder.
     * @param finder_hostname the XRL Finder hostname.
     * @param finder_port the XRL Finder port.
     * @param is_dummy if true, then run the FEA in dummy mode.
     */
    XrlFeaNode(EventLoop& eventloop, const string& xrl_fea_targetname,
	       const string& xrl_finder_targetname,
	       const string& finder_hostname, uint16_t finder_port,
	       bool is_dummy);

    /**
     * Destructor
     */
    virtual	~XrlFeaNode();

    /**
     * Startup the service operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		startup();

    /**
     * Shutdown the service operation.
     *
     * Gracefully shutdown the FEA.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		shutdown();

    /**
     * Test whether the service is running.
     *
     * @return true if the service is still running, otherwise false.
     */
    bool	is_running() const;

    /**
     * Test whether a shutdown XRL request has been received.
     *
     * @return true if shutdown XRL request has been received, otherwise false.
     */
    bool	is_shutdown_received() const;

    /**
     * Get the event loop this service is added to.
     * 
     * @return the event loop this service is added to.
     */
    EventLoop&	eventloop() { return (_eventloop); }

    /**
     * Get the XRL transmission and reception point.
     *
     * @return reference to the XRL transmission and reception point.
     */
    XrlStdRouter& xrl_router() { return (_xrl_router); }

    /**
     * Get the FEA I/O XRL instance.
     *
     * @return reference to the FEA I/O XRL instance.
     */
    XrlFeaIo&	xrl_fea_io() { return (_xrl_fea_io); }

    /**
     * Get the FEA node instance.
     *
     * @return reference to the FEA node instance.
     */
    FeaNode&	fea_node() { return (_fea_node); }

    /**
     * Get the FEA XRL target.
     *
     * @return reference to the FEA XRL target.
     */
    XrlFeaTarget& xrl_fea_target() { return (_xrl_fea_target); }

    /**
     * Get the Finder's XRL target name.
     *
     * @return the Finder's XRL target name.
     */
    const string& xrl_finder_targetname() const { return (_xrl_finder_targetname); }

private:
    EventLoop&		_eventloop;	// The event loop to use
    XrlStdRouter	_xrl_router;	// The standard XRL send/recv point
    XrlFeaIo		_xrl_fea_io;	// The FEA I/O XRL instance
    FeaNode		_fea_node;	// The FEA node
    LibFeaClientBridge	_lib_fea_client_bridge;

    XrlFibClientManager	_xrl_fib_client_manager; // The FIB client manager
    XrlIoLinkManager	_xrl_io_link_manager;	// Link raw I/O manager
    XrlIoIpManager	_xrl_io_ip_manager;	// IP raw I/O manager
    XrlIoTcpUdpManager	_xrl_io_tcpudp_manager;	// TCP/UDP I/O manager

    // MFEA-related stuff
    // TODO: XXX: This should be refactored and better integrated with the FEA.
    // TODO: XXX: For now we don't have a dummy MFEA

    //
    // XXX: TODO: The CLI stuff is temporary needed (and used) by the
    // multicast modules.
    //
    CliNode		_cli_node4;
    XrlCliNode		_xrl_cli_node;
    XrlMfeaNode		_xrl_mfea_node4;	// The IPv4 MFEA
#ifdef HAVE_IPV6_MULTICAST
    XrlMfeaNode		_xrl_mfea_node6;	// The IPv6 MFEA
#endif

    XrlFeaTarget	_xrl_fea_target;	// The FEA XRL target

    const string	_xrl_finder_targetname;	// The Finder target name
};

#endif // __FEA_XRL_FEA_NODE_HH__
