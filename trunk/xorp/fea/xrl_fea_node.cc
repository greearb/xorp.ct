// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007 International Computer Science Institute
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

#ident "$XORP: xorp/fea/xrl_fea_node.cc,v 1.6 2007/05/03 09:32:36 pavlin Exp $"


//
// FEA (Forwarding Engine Abstraction) with XRL front-end node implementation.
//


#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "xrl_fea_node.hh"



#include "libxipc/xrl_std_router.hh"

#include "cli/xrl_cli_node.hh"

#include "fibconfig.hh"
#include "ifconfig.hh"
#include "ifconfig_addr_table.hh"
#include "libfeaclient_bridge.hh"
#include "xrl_mfea_node.hh"
#include "xrl_packet_acl.hh"
#include "xrl_rawsock4.hh"
#include "xrl_rawsock6.hh"
#include "xrl_socket_server.hh"


XrlFeaNode::XrlFeaNode(EventLoop& eventloop, const string& xrl_fea_targetname,
		       const string& xrl_finder_targetname,
		       const string& finder_hostname, uint16_t finder_port)
    : _eventloop(eventloop),
      _fea_node(eventloop),
      _xrl_router(eventloop, xrl_fea_targetname.c_str(),
		  finder_hostname.c_str(), finder_port),
      _lib_fea_client_bridge(_xrl_router, _fea_node.ifconfig().ifconfig_update_replicator()),
      _xrl_fea_io(eventloop),
      _xrl_fib_client_manager(_fea_node.fibconfig(), _xrl_router),
      _xrsm4(_eventloop, _fea_node.ifconfig().local_config(), _xrl_router),
      _xrsm6(_eventloop, _fea_node.ifconfig().local_config(), _xrl_router),
      _xrl_socket_server(_eventloop,
			 _fea_node.ifconfig().ifconfig_address_table(),
			 _xrl_router.finder_address(),
			 _xrl_router.finder_port()),
      _xrl_packet_acl_target(_eventloop,
			     "packet_acl",	// TODO: XXX: hardcoded name
			     finder_hostname, finder_port,
			     _fea_node.pa_transaction_manager()),
      _cli_node4(AF_INET, XORP_MODULE_CLI, _eventloop),
      _xrl_cli_node(_eventloop, _cli_node4.module_name(), finder_hostname,
		    finder_port,
		    xrl_finder_targetname,
		    _cli_node4),
      _xrl_mfea_node4(_fea_node, AF_INET, XORP_MODULE_MFEA, _eventloop,
		      xorp_module_name(AF_INET, XORP_MODULE_MFEA),
		      finder_hostname, finder_port,
		      xrl_finder_targetname),
#ifdef HAVE_IPV6_MULTICAST
      _xrl_mfea_node6(_fea_node, AF_INET6, XORP_MODULE_MFEA, _eventloop,
		      xorp_module_name(AF_INET6, XORP_MODULE_MFEA),
		      finder_hostname, finder_port,
		      xrl_finder_targetname),
#endif
      _xrl_fea_target(_eventloop, _fea_node, _xrl_router, _fea_node.profile(),
		      _xrl_fib_client_manager, _xrsm4, _xrsm6,
		      _lib_fea_client_bridge, _xrl_socket_server),
      _is_dummy(false)
{
    _cli_node4.set_cli_port(0);		// XXX: disable CLI telnet access
}

XrlFeaNode::~XrlFeaNode()
{
    shutdown();
}

int
XrlFeaNode::startup()
{
    wait_until_xrl_router_is_ready(eventloop(), xrl_router());
    wait_until_xrl_router_is_ready(eventloop(),
				   _xrl_packet_acl_target.xrl_router());

    if (! is_dummy()) {
	// XXX: The multicast-related code doesn't have dummy mode (yet)
	wait_until_xrl_router_is_ready(eventloop(),
				       _xrl_cli_node.xrl_router());
	wait_until_xrl_router_is_ready(eventloop(),
				       _xrl_mfea_node4.xrl_router());
#ifdef HAVE_IPV6_MULTICAST
	wait_until_xrl_router_is_ready(eventloop(),
				       _xrl_mfea_node6.xrl_router());
#endif
    }

    fea_node().startup();
    xrl_fea_io().startup();
    xrl_fea_target().startup();

    _xrl_socket_server.startup();
    _xrl_packet_acl_target.startup();

    if (! is_dummy()) {
	// XXX: temporary enable the built-in CLI access
	_xrl_cli_node.enable_cli();
	_xrl_cli_node.start_cli();
	_xrl_mfea_node4.enable_mfea();
	// _xrl_mfea_node4.startup();
	_xrl_mfea_node4.enable_cli();
	_xrl_mfea_node4.start_cli();
#ifdef HAVE_IPV6_MULTICAST
	_xrl_mfea_node6.enable_mfea();
	// _xrl_mfea_node6.startup();
	_xrl_mfea_node6.enable_cli();
	_xrl_mfea_node6.start_cli();
#endif
    }

    return (XORP_OK);
}

int
XrlFeaNode::shutdown()
{
    fea_node().shutdown();
    xrl_fea_io().shutdown();
    xrl_fea_target().shutdown();

    if (! is_dummy()) {
	_xrl_mfea_node4.shutdown();
#ifdef HAVE_IPV6_MULTICAST
	_xrl_mfea_node6.shutdown();
#endif
    }

    _xrl_packet_acl_target.shutdown();
    _xrl_socket_server.shutdown();

    return (XORP_OK);
}

bool
XrlFeaNode::is_running() const
{
    if (_fea_node.is_running())
	return (true);
    if (_xrl_fea_io.is_running())
	return (true);
    if (_xrl_fea_target.is_running())
	return (true);

    if (! is_dummy()) {
	if (! _xrl_mfea_node4.MfeaNode::is_down())
	    return (true);
#ifdef HAVE_IPV6_MULTICAST
	if (! _xrl_mfea_node6.MfeaNode::is_down())
	    return (true);
#endif
    }

    if (_xrl_packet_acl_target.is_running())
	return (true);
    if (_xrl_socket_server.is_running())
	return (true);

    //
    // Test whether all XRL operations have completed
    //

    if (! is_dummy()) {
	if (_xrl_cli_node.xrl_router().pending())
	    return (true);
	if (_xrl_mfea_node4.xrl_router().pending())
	    return (true);
#ifdef HAVE_IPV6_MULTICAST
	if (_xrl_mfea_node6.xrl_router().pending())
	    return (true);
#endif
    }


    if (_xrl_packet_acl_target.xrl_router().pending())
	return (true);
    if (_xrl_socket_server.xrl_router().pending())
	return (true);

    if (_xrl_router.pending())
	return (true);

    return (false);
}

int
XrlFeaNode::set_dummy()
{
    _fea_node.set_dummy();

    _is_dummy = true;

    return (XORP_OK);
}

bool
XrlFeaNode::is_shutdown_received() const
{
    return (_xrl_fea_target.is_shutdown_received());
}
