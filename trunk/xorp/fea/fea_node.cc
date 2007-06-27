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

#ident "$XORP: xorp/fea/fea_node.cc,v 1.6 2007/06/11 22:02:32 pavlin Exp $"


//
// FEA (Forwarding Engine Abstraction) node implementation.
//


#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/eventloop.hh"

#include "libcomm/comm_api.h"

#ifdef HOST_OS_WINDOWS
#include "fea/data_plane/control_socket/windows_rras_support.hh"
#endif

#include "fea_node.hh"
#include "profile_vars.hh"


FeaNode::FeaNode(EventLoop& eventloop)
    : _eventloop(eventloop),
      _is_running(false),
      _is_dummy(false),
      _ifconfig(eventloop, _nexthop_port_mapper),
      _fibconfig(_eventloop, _profile,
#ifdef HOST_OS_WINDOWS
		 // XXX: Windows FibConfig needs to see the live ifconfig tree
		 _ifconfig.live_config(),
#else
		 _ifconfig.local_config(),
#endif
		 _nexthop_port_mapper),
      _io_link_manager(_eventloop, ifconfig().local_config()),
      _io_ip_manager(_eventloop, ifconfig().local_config()),
      _pa_transaction_manager(_eventloop, _pa_table_manager)
{
}

FeaNode::~FeaNode()
{
    shutdown();
}

int
FeaNode::startup()
{
    string error_msg;

    _is_running = false;

    comm_init();
    initialize_profiling_variables(_profile);

#ifdef HOST_OS_WINDOWS
    //
    // Load support code for Windows Router Manager V2, if it
    // is detected as running and/or configured.
    //
    if (WinSupport::is_rras_running()) {
	WinSupport::add_protocol_to_registry(AF_INET);
#if 0
	WinSupport::add_protocol_to_registry(AF_INET6);
#endif
	WinSupport::restart_rras();
	WinSupport::add_protocol_to_rras(AF_INET);
#if 0
	WinSupport::add_protocol_to_rras(AF_INET6);
#endif
	TimerList::system_sleep(TimeVal(2, 0));
    }
#endif // HOST_OS_WINDOWS

    //
    // IfConfig and FibConfig
    //
    if (_ifconfig.start(error_msg) != XORP_OK) {
	XLOG_FATAL("Cannot start IfConfig: %s", error_msg.c_str());
    }
    if (_fibconfig.start(error_msg) != XORP_OK) {
	XLOG_FATAL("Cannot start FibConfig: %s", error_msg.c_str());
    }

    _is_running = true;

    return (XORP_OK);
}

int
FeaNode::shutdown()
{
    string error_msg;

    //
    // Gracefully stop the FEA
    //
    // TODO: this may not work if we depend on reading asynchronously
    // data from sockets. To fix this, we need to run the eventloop
    // until we get all the data we need. Tricky...
    //
    if (_fibconfig.stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop FibConfig: %s", error_msg.c_str());
    }
    if (_ifconfig.stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop IfConfig: %s", error_msg.c_str());
    }

    comm_exit();

    _is_running = false;

    return (XORP_OK);
}

bool
FeaNode::is_running() const
{
    return (_is_running);
}

int
FeaNode::set_dummy()
{
    _ifconfig.set_dummy();
    _fibconfig.set_dummy();

    _is_dummy = true;

    return (XORP_OK);
}
