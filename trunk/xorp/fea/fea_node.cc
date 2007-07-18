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

#ident "$XORP: xorp/fea/fea_node.cc,v 1.8 2007/07/11 22:18:02 pavlin Exp $"


//
// FEA (Forwarding Engine Abstraction) node implementation.
//

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/eventloop.hh"

#include "libcomm/comm_api.h"

#include "fea/data_plane/managers/fea_data_plane_manager_bsd.hh"
#include "fea/data_plane/managers/fea_data_plane_manager_click.hh"
#include "fea/data_plane/managers/fea_data_plane_manager_dummy.hh"
#include "fea/data_plane/managers/fea_data_plane_manager_linux.hh"
#include "fea/data_plane/managers/fea_data_plane_manager_windows.hh"

#include "fea_node.hh"
#include "profile_vars.hh"


FeaNode::FeaNode(EventLoop& eventloop, bool is_dummy)
    : _eventloop(eventloop),
      _is_running(false),
      _is_dummy(is_dummy),
      _ifconfig(*this),
      _fibconfig(*this,
#ifdef HOST_OS_WINDOWS
		 // XXX: Windows FibConfig needs to see the live ifconfig tree
		 _ifconfig.live_config()
#else
		 _ifconfig.local_config()
#endif
	  ),
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

    if (load_data_plane_managers(error_msg) != XORP_OK) {
	XLOG_FATAL("Cannot load the data plane manager(s): %s",
		   error_msg.c_str());
    }

    if (start_data_plane_managers_plugins(error_msg) != XORP_OK) {
	XLOG_FATAL("Cannot start the data plane manager(s) plugins: %s",
		   error_msg.c_str());
    }

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

    if (unload_data_plane_managers(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot unload the data plane manager(s): %s",
		   error_msg.c_str());
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

bool
FeaNode::have_ipv4() const
{
    if (_fea_data_plane_managers.empty())
	return (false);

    //
    // XXX: We pull the information by using only the first data plane manager.
    // In the future we need to rething this and be more flexible.
    //
    return (_fea_data_plane_managers.front()->have_ipv4());
}

bool
FeaNode::have_ipv6() const
{
    if (_fea_data_plane_managers.empty())
	return (false);

    //
    // XXX: We pull the information by using only the first data plane manager.
    // In the future we need to rething this and be more flexible.
    //
    return (_fea_data_plane_managers.front()->have_ipv6());
}

int
FeaNode::register_data_plane_manager(FeaDataPlaneManager* fea_data_plane_manager,
				     bool is_exclusive)
{
    string dummy_error_msg;

    if (is_exclusive) {
	// Unload and delete the previous data plane managers
	unload_data_plane_managers(dummy_error_msg);
    }

    if ((fea_data_plane_manager != NULL)
	&& (find(_fea_data_plane_managers.begin(),
		 _fea_data_plane_managers.end(),
		 fea_data_plane_manager)
	    == _fea_data_plane_managers.end())) {
	_fea_data_plane_managers.push_back(fea_data_plane_manager);
    }

    return (XORP_OK);
}

int
FeaNode::unregister_data_plane_manager(FeaDataPlaneManager* fea_data_plane_manager)
{
    string dummy_error_msg;

    if (fea_data_plane_manager == NULL)
	return (XORP_ERROR);

    list<FeaDataPlaneManager*>::iterator iter;
    iter = find(_fea_data_plane_managers.begin(),
		_fea_data_plane_managers.end(),
		fea_data_plane_manager);
    if (iter == _fea_data_plane_managers.end())
	return (XORP_ERROR);

    fea_data_plane_manager->stop_manager(dummy_error_msg);
    _fea_data_plane_managers.erase(iter);
    delete fea_data_plane_manager;

    return (XORP_OK);
}

int
FeaNode::load_data_plane_managers(string& error_msg)
{
    string dummy_error_msg;

    FeaDataPlaneManager* fea_data_plane_manager = NULL;

    unload_data_plane_managers(dummy_error_msg);

#if 0
    //
    // TODO: XXX: PAVPAVPAV: sample code for dynamic loading
    //
    void* plugin_handle = dlopen(library_name.c_str(), RTLD_LAZY);
    if (plugin_handle == NULL) {
	error_msg = c_format("Cannot open library %s: %s",
			     library_name.c_str(),
			     dlerror());
	return (XORP_ERROR);
    }
    XLOG_TRACE(false, "Loaded library %s", library_name.c_str());
    typedef FeaDataPlaneManager* (*create_t)(FeaNode& fea_node);
    create_t create = (create_t)dlsym(plugin_handle, "create");
    const char* dlsym_error = dlerror();
    if (dlsym_error != NULL) {
	error_msg = c_format("Cannot load symbol \"create\": %s", dlsym_error);
	dlclose(plugin_handler);
	return (XORP_ERROR);
    }
    fea_data_plane_manager = create(*this);

    //
    // TODO: XXX: PAVPAVPAV: sample code for destroying the dynamic loaded
    // plugin.
    //
    typedef void (*destroy_t)();
    destroy_t destroy = (destroy_t)dlsym(plugin_handle, "destroy");
    const char* dlsym_error = dlerror();
    if (dlsym_error != NULL) {
	error_msg = c_format("Cannot load symbol \"destroy\": %s",
			     dlsym_error);
	return (XORP_ERROR);
    }
    destroy();

#endif // 0

    if (is_dummy()) {
	fea_data_plane_manager = new FeaDataPlaneManagerDummy(*this);
    } else {
#if defined(HOST_OS_MACOSX) || defined(HOST_OS_DRAGONFLYBSD) || defined(HOST_OS_FREEBSD) || defined(HOST_OS_NETBSD) || defined(HOST_OS_OPENBSD)
	fea_data_plane_manager = new FeaDataPlaneManagerBsd(*this);
#elif(HOST_OS_LINUX)
	fea_data_plane_manager = new FeaDataPlaneManagerLinux(*this);
#elif defined(HOST_OS_WINDOWS)
	fea_data_plane_manager = new FeaDataPlaneManagerWindows(*this);
#else
#error "No data plane manager to load: unknown system"
#endif
    }

    if (register_data_plane_manager(fea_data_plane_manager, true)
	!= XORP_OK) {
	error_msg = c_format("Failed to register the %s data plane manager",
			     fea_data_plane_manager->manager_name().c_str());
	delete fea_data_plane_manager;
	return (XORP_ERROR);
    }

    if (fea_data_plane_manager->start_manager(error_msg) != XORP_OK) {
	error_msg = c_format("Failed to start the %s data plane manager: %s",
			     fea_data_plane_manager->manager_name().c_str(),
			     error_msg.c_str());
	unload_data_plane_managers(dummy_error_msg);
	return (XORP_ERROR);
    }

    if (fea_data_plane_manager->register_plugins(error_msg) != XORP_OK) {
	error_msg = c_format("Failed to register the %s data plane "
			     "manager plugins: %s",
			     fea_data_plane_manager->manager_name().c_str(),
			     error_msg.c_str());
	unload_data_plane_managers(dummy_error_msg);
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FeaNode::unload_data_plane_managers(string& error_msg)
{
    string dummy_error_msg;

    UNUSED(error_msg);

    //
    // Stop and delete the data plane managers
    //
    while (! _fea_data_plane_managers.empty()) {
	unregister_data_plane_manager(_fea_data_plane_managers.front());
    }

    return (XORP_OK);
}

int
FeaNode::start_data_plane_managers_plugins(string& error_msg)
{
    list<FeaDataPlaneManager*>::iterator iter;

    for (iter = _fea_data_plane_managers.begin();
	 iter != _fea_data_plane_managers.end();
	 ++iter) {
	FeaDataPlaneManager* fea_data_plane_manager = *iter;
	if (fea_data_plane_manager->start_plugins(error_msg) != XORP_OK) {
	    return (XORP_ERROR);
	}
    }

    return (XORP_OK);
}

int
FeaNode::stop_data_plane_managers_plugins(string& error_msg)
{
    int ret_value = XORP_OK;
    string error_msg2;
    list<FeaDataPlaneManager*>::iterator iter;

    error_msg.erase();

    for (iter = _fea_data_plane_managers.begin();
	 iter != _fea_data_plane_managers.end();
	 ++iter) {
	FeaDataPlaneManager* fea_data_plane_manager = *iter;
	if (fea_data_plane_manager->stop_plugins(error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    return (ret_value);
}
