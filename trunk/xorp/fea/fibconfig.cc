// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2007 International Computer Science Institute
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

#ident "$XORP: xorp/fea/fibconfig.cc,v 1.9 2007/06/07 01:28:34 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/profile.hh"

#include "libcomm/comm_api.h"

#ifdef HAVE_STROPTS_H
#include <stropts.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_IPHLPAPI_H
#include <iphlpapi.h>
#endif

#ifdef HAVE_INET_ND_H
#include <inet/nd.h>
#endif

#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif

#include "profile_vars.hh"
#include "fibconfig.hh"
#include "fibconfig_transaction.hh"
#include "fea_node.hh"

#ifdef HOST_OS_WINDOWS
#include "libxorp/win_io.h"
#include "fea/data_plane/control_socket/windows_routing_socket.h"
#include "fea/data_plane/control_socket/windows_rras_support.hh"
#endif

#define PROC_LINUX_FILE_FORWARDING_V4 "/proc/sys/net/ipv4/ip_forward"
#define PROC_LINUX_FILE_FORWARDING_V6 "/proc/sys/net/ipv6/conf/all/forwarding"
#define DEV_SOLARIS_DRIVER_FORWARDING_V4 "/dev/ip"
#define DEV_SOLARIS_DRIVER_FORWARDING_V6 "/dev/ip6"
#define DEV_SOLARIS_DRIVER_PARAMETER_FORWARDING_V4 "ip_forwarding"
#define DEV_SOLARIS_DRIVER_PARAMETER_FORWARDING_V6 "ip6_forwarding"
#define DEV_SOLARIS_DRIVER_PARAMETER_IGNORE_REDIRECT_V6 "ip6_ignore_redirect"

#ifdef __MINGW32__
#define MIB_IP_FORWARDING	1
#define MIB_IP_NOT_FORWARDING	2
#endif

//
// Unicast forwarding table related configuration.
//


FibConfig::FibConfig(FeaNode& fea_node, const IfTree& iftree)
    : _fea_node(fea_node),
      _eventloop(fea_node.eventloop()),
      _profile(fea_node.profile()),
      _nexthop_port_mapper(fea_node.nexthop_port_mapper()),
      _iftree(iftree),
      _ftm(NULL),
      _unicast_forwarding_enabled4(false),
      _unicast_forwarding_enabled6(false),
      _accept_rtadv_enabled6(false),
      _unicast_forwarding_entries_retain_on_startup4(false),
      _unicast_forwarding_entries_retain_on_shutdown4(false),
      _unicast_forwarding_entries_retain_on_startup6(false),
      _unicast_forwarding_entries_retain_on_shutdown6(false),
      _have_ipv4(false),
      _have_ipv6(false),
      _is_running(false)
{
    string error_msg;

    _ftm = new FibConfigTransactionManager(_eventloop, *this);

    //
    // Test if the system supports IPv4 and IPv6 respectively
    //
    _have_ipv4 = test_have_ipv4();
    _have_ipv6 = test_have_ipv6();

    //
    // Get the old state from the underlying system
    //
    if (_have_ipv4) {
	if (unicast_forwarding_enabled4(_unicast_forwarding_enabled4,
					error_msg) < 0) {
	    XLOG_FATAL("%s", error_msg.c_str());
	}
    }
#ifdef HAVE_IPV6
    if (_have_ipv6) {
	if (unicast_forwarding_enabled6(_unicast_forwarding_enabled6,
					error_msg) < 0) {
	    XLOG_FATAL("%s", error_msg.c_str());
	}
	if (accept_rtadv_enabled6(_accept_rtadv_enabled6, error_msg) < 0) {
	    XLOG_FATAL("%s", error_msg.c_str());
	}
    }
#endif // HAVE_IPV6

#ifdef HOST_OS_WINDOWS
    _event = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (_event == NULL)
	XLOG_FATAL("Could not create Win32 event object.");
    memset(&_overlapped, 0, sizeof(_overlapped));
    _overlapped.hEvent = _event;
    _enablecnt = 0;
#endif
}

FibConfig::~FibConfig()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the mechanism for manipulating "
		   "the forwarding table information: %s",
		   error_msg.c_str());
    }

#ifdef HOST_OS_WINDOWS
    if (_enablecnt > 0) {
	XLOG_WARNING("EnableRouter() without %d matching "
		     "UnenableRouter() calls.", _enablecnt);
    }
    CloseHandle(_event);
#endif

    if (_ftm != NULL) {
	delete _ftm;
	_ftm = NULL;
    }
}

ProcessStatus
FibConfig::status(string& reason) const
{
    if (_ftm->pending() > 0) {
	reason = "There are transactions pending";
	return (PROC_NOT_READY);
    }
    return (PROC_READY);
}

int
FibConfig::start_transaction(uint32_t& tid, string& error_msg)
{
    if (_ftm->start(tid) != true) {
	error_msg = c_format("Resource limit on number of pending transactions hit");
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FibConfig::abort_transaction(uint32_t tid, string& error_msg)
{
    if (_ftm->abort(tid) != true) {
	error_msg = c_format("Expired or invalid transaction ID presented");
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FibConfig::add_transaction_operation(uint32_t tid,
				     const TransactionManager::Operation& op,
				     string& error_msg)
{
    uint32_t n_ops = 0;

    if (_ftm->retrieve_size(tid, n_ops) != true) {
	error_msg = c_format("Expired or invalid transaction ID presented");
	return (XORP_ERROR);
    }

    if (_ftm->max_ops() <= n_ops) {
	error_msg = c_format("Resource limit on number of operations in a transaction hit");
	return (XORP_ERROR);
    }

    //
    // In theory, resource shortage is the only thing that could get us here
    //
    if (_ftm->add(tid, op) != true) {
	error_msg = c_format("Unknown resource shortage");
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FibConfig::commit_transaction(uint32_t tid, string& error_msg)
{
    if (_ftm->commit(tid) != true) {
	error_msg = c_format("Expired or invalid transaction ID presented");
	return (XORP_ERROR);
    }

    const string& ftm_error_msg = _ftm->error();
    if (! ftm_error_msg.empty()) {
	error_msg = ftm_error_msg;
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FibConfig::register_fibconfig_entry_get(FibConfigEntryGet* fibconfig_entry_get,
					bool is_exclusive)
{
    if (is_exclusive)
	_fibconfig_entry_gets.clear();

    if ((fibconfig_entry_get != NULL)
	&& (find(_fibconfig_entry_gets.begin(), _fibconfig_entry_gets.end(),
		 fibconfig_entry_get)
	    == _fibconfig_entry_gets.end())) {
	_fibconfig_entry_gets.push_back(fibconfig_entry_get);
    }

    return (XORP_OK);
}

int
FibConfig::unregister_fibconfig_entry_get(FibConfigEntryGet* fibconfig_entry_get)
{
    if (fibconfig_entry_get == NULL)
	return (XORP_ERROR);

    list<FibConfigEntryGet*>::iterator iter;
    iter = find(_fibconfig_entry_gets.begin(), _fibconfig_entry_gets.end(),
		fibconfig_entry_get);
    if (iter == _fibconfig_entry_gets.end())
	return (XORP_ERROR);
    _fibconfig_entry_gets.erase(iter);

    return (XORP_OK);
}

int
FibConfig::register_fibconfig_entry_set(FibConfigEntrySet* fibconfig_entry_set,
					bool is_exclusive)
{
    if (is_exclusive)
	_fibconfig_entry_sets.clear();

    if ((fibconfig_entry_set != NULL) 
	&& (find(_fibconfig_entry_sets.begin(), _fibconfig_entry_sets.end(),
		 fibconfig_entry_set)
	    == _fibconfig_entry_sets.end())) {
	_fibconfig_entry_sets.push_back(fibconfig_entry_set);

	//
	// XXX: Push the current config into the new method
	//
	if (fibconfig_entry_set->is_running()) {
	    // XXX: nothing to do.
	    //
	    // XXX: note that the forwarding table state is pushed by method
	    // FibConfig::register_fibconfig_table_set(),
	    // hence we don't need to do it here again. However, this is based
	    // on the assumption that for each FibConfigEntrySet method
	    // there is a corresponding FibConfigTableSet method.
	    //
	}
    }

    return (XORP_OK);
}

int
FibConfig::unregister_fibconfig_entry_set(FibConfigEntrySet* fibconfig_entry_set)
{
    if (fibconfig_entry_set == NULL)
	return (XORP_ERROR);

    list<FibConfigEntrySet*>::iterator iter;
    iter = find(_fibconfig_entry_sets.begin(), _fibconfig_entry_sets.end(),
		fibconfig_entry_set);
    if (iter == _fibconfig_entry_sets.end())
	return (XORP_ERROR);
    _fibconfig_entry_sets.erase(iter);

    return (XORP_OK);
}

int
FibConfig::register_fibconfig_entry_observer(FibConfigEntryObserver* fibconfig_entry_observer,
					     bool is_exclusive)
{
    if (is_exclusive)
	_fibconfig_entry_observers.clear();

    if ((fibconfig_entry_observer != NULL) 
	&& (find(_fibconfig_entry_observers.begin(),
		 _fibconfig_entry_observers.end(),
		 fibconfig_entry_observer)
	    == _fibconfig_entry_observers.end())) {
	_fibconfig_entry_observers.push_back(fibconfig_entry_observer);
    }
    
    return (XORP_OK);
}

int
FibConfig::unregister_fibconfig_entry_observer(FibConfigEntryObserver* fibconfig_entry_observer)
{
    if (fibconfig_entry_observer == NULL)
	return (XORP_ERROR);

    list<FibConfigEntryObserver*>::iterator iter;
    iter = find(_fibconfig_entry_observers.begin(),
		_fibconfig_entry_observers.end(),
		fibconfig_entry_observer);
    if (iter == _fibconfig_entry_observers.end())
	return (XORP_ERROR);
    _fibconfig_entry_observers.erase(iter);

    return (XORP_OK);
}

int
FibConfig::register_fibconfig_table_get(FibConfigTableGet* fibconfig_table_get,
					bool is_exclusive)
{
    if (is_exclusive)
	_fibconfig_table_gets.clear();

    if ((fibconfig_table_get != NULL)
	&& (find(_fibconfig_table_gets.begin(), _fibconfig_table_gets.end(),
		 fibconfig_table_get)
	    == _fibconfig_table_gets.end())) {
	_fibconfig_table_gets.push_back(fibconfig_table_get);
    }
    
    return (XORP_OK);
}

int
FibConfig::unregister_fibconfig_table_get(FibConfigTableGet* fibconfig_table_get)
{
    if (fibconfig_table_get == NULL)
	return (XORP_ERROR);

    list<FibConfigTableGet*>::iterator iter;
    iter = find(_fibconfig_table_gets.begin(), _fibconfig_table_gets.end(),
		fibconfig_table_get);
    if (iter == _fibconfig_table_gets.end())
	return (XORP_ERROR);
    _fibconfig_table_gets.erase(iter);

    return (XORP_OK);
}

int
FibConfig::register_fibconfig_table_set(FibConfigTableSet* fibconfig_table_set,
					bool is_exclusive)
{
    if (is_exclusive)
	_fibconfig_table_sets.clear();

    if ((fibconfig_table_set != NULL)
	&& (find(_fibconfig_table_sets.begin(), _fibconfig_table_sets.end(),
		 fibconfig_table_set)
	    == _fibconfig_table_sets.end())) {
	_fibconfig_table_sets.push_back(fibconfig_table_set);

	//
	// XXX: Push the current config into the new method
	//
	if (fibconfig_table_set->is_running()) {
	    list<Fte4> fte_list4;

	    if (get_table4(fte_list4) == true) {
		if (fibconfig_table_set->set_table4(fte_list4) != true) {
		    XLOG_ERROR("Cannot push the current IPv4 forwarding table "
			       "into a new mechanism for setting the "
			       "forwarding table");
		}
	    }

#ifdef HAVE_IPV6
	    list<Fte6> fte_list6;

	    if (get_table6(fte_list6) == true) {
		if (fibconfig_table_set->set_table6(fte_list6) != true) {
		    XLOG_ERROR("Cannot push the current IPv6 forwarding table "
			       "into a new mechanism for setting the "
			       "forwarding table");
		}
	    }
#endif // HAVE_IPV6
	}
    }

    return (XORP_OK);
}

int
FibConfig::unregister_fibconfig_table_set(FibConfigTableSet* fibconfig_table_set)
{
    if (fibconfig_table_set == NULL)
	return (XORP_ERROR);

    list<FibConfigTableSet*>::iterator iter;
    iter = find(_fibconfig_table_sets.begin(), _fibconfig_table_sets.end(),
		fibconfig_table_set);
    if (iter == _fibconfig_table_sets.end())
	return (XORP_ERROR);
    _fibconfig_table_sets.erase(iter);

    return (XORP_OK);
}

int
FibConfig::register_fibconfig_table_observer(FibConfigTableObserver* fibconfig_table_observer,
					     bool is_exclusive)
{
    if (is_exclusive)
	_fibconfig_table_observers.clear();

    if ((fibconfig_table_observer != NULL)
	&& (find(_fibconfig_table_observers.begin(),
		 _fibconfig_table_observers.end(),
		 fibconfig_table_observer)
	    == _fibconfig_table_observers.end())) {
	_fibconfig_table_observers.push_back(fibconfig_table_observer);
    }

    return (XORP_OK);
}

int
FibConfig::unregister_fibconfig_table_observer(FibConfigTableObserver* fibconfig_table_observer)
{
    if (fibconfig_table_observer == NULL)
	return (XORP_ERROR);

    list<FibConfigTableObserver*>::iterator iter;
    iter = find(_fibconfig_table_observers.begin(),
		_fibconfig_table_observers.end(),
		fibconfig_table_observer);
    if (iter == _fibconfig_table_observers.end())
	return (XORP_ERROR);
    _fibconfig_table_observers.erase(iter);

    return (XORP_OK);
}

int
FibConfig::start(string& error_msg)
{
    list<FibConfigEntryGet*>::iterator fibconfig_entry_get_iter;
    list<FibConfigEntrySet*>::iterator fibconfig_entry_set_iter;
    list<FibConfigEntryObserver*>::iterator fibconfig_entry_observer_iter;
    list<FibConfigTableGet*>::iterator fibconfig_table_get_iter;
    list<FibConfigTableSet*>::iterator fibconfig_table_set_iter;
    list<FibConfigTableObserver*>::iterator fibconfig_table_observer_iter;

    if (_is_running)
	return (XORP_OK);

    //
    // Check whether all mechanisms are available
    //
    if (_fibconfig_entry_gets.empty()) {
	error_msg = c_format("No mechanism to get forwarding table entries");
	return (XORP_ERROR);
    }
    if (_fibconfig_entry_sets.empty()) {
	error_msg = c_format("No mechanism to set forwarding table entries");
	return (XORP_ERROR);
    }
    if (_fibconfig_entry_observers.empty()) {
	error_msg = c_format("No mechanism to observe forwarding table entries");
	return (XORP_ERROR);
    }
    if (_fibconfig_table_gets.empty()) {
	error_msg = c_format("No mechanism to get the forwarding table");
	return (XORP_ERROR);
    }
    if (_fibconfig_table_sets.empty()) {
	error_msg = c_format("No mechanism to set the forwarding table");
	return (XORP_ERROR);
    }
    if (_fibconfig_table_observers.empty()) {
	error_msg = c_format("No mechanism to observe the forwarding table");
	return (XORP_ERROR);
    }

    //
    // Start the FibConfigEntryGet methods
    //
    for (fibconfig_entry_get_iter = _fibconfig_entry_gets.begin();
	 fibconfig_entry_get_iter != _fibconfig_entry_gets.end();
	 ++fibconfig_entry_get_iter) {
	FibConfigEntryGet* fibconfig_entry_get = *fibconfig_entry_get_iter;
	if (fibconfig_entry_get->start(error_msg) < 0)
	    return (XORP_ERROR);
    }

    //
    // Start the FibConfigEntrySet methods
    //
    for (fibconfig_entry_set_iter = _fibconfig_entry_sets.begin();
	 fibconfig_entry_set_iter != _fibconfig_entry_sets.end();
	 ++fibconfig_entry_set_iter) {
	FibConfigEntrySet* fibconfig_entry_set = *fibconfig_entry_set_iter;
	if (fibconfig_entry_set->start(error_msg) < 0)
	    return (XORP_ERROR);
    }

    //
    // Start the FibConfigEntryObserver methods
    //
    for (fibconfig_entry_observer_iter = _fibconfig_entry_observers.begin();
	 fibconfig_entry_observer_iter != _fibconfig_entry_observers.end();
	 ++fibconfig_entry_observer_iter) {
	FibConfigEntryObserver* fibconfig_entry_observer = *fibconfig_entry_observer_iter;
	if (fibconfig_entry_observer->start(error_msg) < 0)
	    return (XORP_ERROR);
    }

    //
    // Start the FibConfigTableGet methods
    //
    for (fibconfig_table_get_iter = _fibconfig_table_gets.begin();
	 fibconfig_table_get_iter != _fibconfig_table_gets.end();
	 ++fibconfig_table_get_iter) {
	FibConfigTableGet* fibconfig_table_get = *fibconfig_table_get_iter;
	if (fibconfig_table_get->start(error_msg) < 0)
	    return (XORP_ERROR);
    }

    //
    // Start the FibConfigTableSet methods
    //
    for (fibconfig_table_set_iter = _fibconfig_table_sets.begin();
	 fibconfig_table_set_iter != _fibconfig_table_sets.end();
	 ++fibconfig_table_set_iter) {
	FibConfigTableSet* fibconfig_table_set = *fibconfig_table_set_iter;
	if (fibconfig_table_set->start(error_msg) < 0)
	    return (XORP_ERROR);
    }

    //
    // Start the FibConfigTableObserver methods
    //
    for (fibconfig_table_observer_iter = _fibconfig_table_observers.begin();
	 fibconfig_table_observer_iter != _fibconfig_table_observers.end();
	 ++fibconfig_table_observer_iter) {
	FibConfigTableObserver* fibconfig_table_observer = *fibconfig_table_observer_iter;
	if (fibconfig_table_observer->start(error_msg) < 0)
	    return (XORP_ERROR);
    }

    _is_running = true;

    return (XORP_OK);
}

int
FibConfig::stop(string& error_msg)
{
    list<FibConfigEntryGet*>::iterator fibconfig_entry_get_iter;
    list<FibConfigEntrySet*>::iterator fibconfig_entry_set_iter;
    list<FibConfigEntryObserver*>::iterator fibconfig_entry_observer_iter;
    list<FibConfigTableGet*>::iterator fibconfig_table_get_iter;
    list<FibConfigTableSet*>::iterator fibconfig_table_set_iter;
    list<FibConfigTableObserver*>::iterator fibconfig_table_observer_iter;
    int ret_value = XORP_OK;
    string error_msg2;

    if (! _is_running)
	return (XORP_OK);

    error_msg.erase();

    //
    // Stop the FibConfigTableObserver methods
    //
    for (fibconfig_table_observer_iter = _fibconfig_table_observers.begin();
	 fibconfig_table_observer_iter != _fibconfig_table_observers.end();
	 ++fibconfig_table_observer_iter) {
	FibConfigTableObserver* fibconfig_table_observer = *fibconfig_table_observer_iter;
	if (fibconfig_table_observer->stop(error_msg2) < 0) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    //
    // Stop the FibConfigTableSet methods
    //
    for (fibconfig_table_set_iter = _fibconfig_table_sets.begin();
	 fibconfig_table_set_iter != _fibconfig_table_sets.end();
	 ++fibconfig_table_set_iter) {
	FibConfigTableSet* fibconfig_table_set = *fibconfig_table_set_iter;
	if (fibconfig_table_set->stop(error_msg2) < 0) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    //
    // Stop the FibConfigTableGet methods
    //
    for (fibconfig_table_get_iter = _fibconfig_table_gets.begin();
	 fibconfig_table_get_iter != _fibconfig_table_gets.end();
	 ++fibconfig_table_get_iter) {
	FibConfigTableGet* fibconfig_table_get = *fibconfig_table_get_iter;
	if (fibconfig_table_get->stop(error_msg2) < 0) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    //
    // Stop the FibConfigEntryObserver methods
    //
    for (fibconfig_entry_observer_iter = _fibconfig_entry_observers.begin();
	 fibconfig_entry_observer_iter != _fibconfig_entry_observers.end();
	 ++fibconfig_entry_observer_iter) {
	FibConfigEntryObserver* fibconfig_entry_observer = *fibconfig_entry_observer_iter;
	if (fibconfig_entry_observer->stop(error_msg2) < 0) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    //
    // Stop the FibConfigEntrySet methods
    //
    for (fibconfig_entry_set_iter = _fibconfig_entry_sets.begin();
	 fibconfig_entry_set_iter != _fibconfig_entry_sets.end();
	 ++fibconfig_entry_set_iter) {
	FibConfigEntrySet* fibconfig_entry_set = *fibconfig_entry_set_iter;
	if (fibconfig_entry_set->stop(error_msg2) < 0) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    //
    // Stop the FibConfigEntryGet methods
    //
    for (fibconfig_entry_get_iter = _fibconfig_entry_gets.begin();
	 fibconfig_entry_get_iter != _fibconfig_entry_gets.end();
	 ++fibconfig_entry_get_iter) {
	FibConfigEntryGet* fibconfig_entry_get = *fibconfig_entry_get_iter;
	if (fibconfig_entry_get->stop(error_msg2) < 0) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    //
    // Restore the old forwarding state in the underlying system.
    //
    // XXX: Note that if the XORP forwarding entries are retained on shutdown,
    // then we don't restore the state.
    //
    if (_have_ipv4) {
	if (! unicast_forwarding_entries_retain_on_shutdown4()) {
	    if (set_unicast_forwarding_enabled4(_unicast_forwarding_enabled4,
						error_msg) < 0) {
		ret_value = XORP_ERROR;
	    }
	}
    }
#ifdef HAVE_IPV6
    if (_have_ipv6) {
	if (! unicast_forwarding_entries_retain_on_shutdown6()) {
	    if (set_unicast_forwarding_enabled6(_unicast_forwarding_enabled6,
						error_msg) < 0) {
		ret_value = XORP_ERROR;
	    }
	    if (set_accept_rtadv_enabled6(_accept_rtadv_enabled6, error_msg)
		< 0) {
		ret_value = XORP_ERROR;
	    }
	}
    }
#endif // HAVE_IPV6

    _is_running = false;

    return (ret_value);
}

bool
FibConfig::start_configuration(string& error_msg)
{
    list<FibConfigEntrySet*>::iterator fibconfig_entry_set_iter;
    list<FibConfigTableSet*>::iterator fibconfig_table_set_iter;
    bool ret_value = true;
    string error_msg2;

    error_msg.erase();

    //
    // XXX: We need to call start_configuration() for "entry" and "table",
    // because the top-level start/end configuration interface
    // does not distinguish between "entry" and "table" modification.
    //
    for (fibconfig_entry_set_iter = _fibconfig_entry_sets.begin();
	 fibconfig_entry_set_iter != _fibconfig_entry_sets.end();
	 ++fibconfig_entry_set_iter) {
	FibConfigEntrySet* fibconfig_entry_set = *fibconfig_entry_set_iter;
	if (fibconfig_entry_set->start_configuration(error_msg2) != true) {
	    ret_value = false;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    for (fibconfig_table_set_iter = _fibconfig_table_sets.begin();
	 fibconfig_table_set_iter != _fibconfig_table_sets.end();
	 ++fibconfig_table_set_iter) {
	FibConfigTableSet* fibconfig_table_set = *fibconfig_table_set_iter;
	if (fibconfig_table_set->start_configuration(error_msg2) != true) {
	    ret_value = false;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }
    
    return (ret_value);
}

bool
FibConfig::end_configuration(string& error_msg)
{
    list<FibConfigEntrySet*>::iterator fibconfig_entry_set_iter;
    list<FibConfigTableSet*>::iterator fibconfig_table_set_iter;
    bool ret_value = true;
    string error_msg2;

    error_msg.erase();

    //
    // XXX: We need to call end_configuration() for "entry" and "table",
    // because the top-level start/end configuration interface
    // does not distinguish between "entry" and "table" modification.
    //
    for (fibconfig_entry_set_iter = _fibconfig_entry_sets.begin();
	 fibconfig_entry_set_iter != _fibconfig_entry_sets.end();
	 ++fibconfig_entry_set_iter) {
	FibConfigEntrySet* fibconfig_entry_set = *fibconfig_entry_set_iter;
	if (fibconfig_entry_set->end_configuration(error_msg2) != true) {
	    ret_value = false;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    for (fibconfig_table_set_iter = _fibconfig_table_sets.begin();
	 fibconfig_table_set_iter != _fibconfig_table_sets.end();
	 ++fibconfig_table_set_iter) {
	FibConfigTableSet* fibconfig_table_set = *fibconfig_table_set_iter;
	if (fibconfig_table_set->end_configuration(error_msg2) != true) {
	    ret_value = false;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }
    
    return (ret_value);
}

bool
FibConfig::add_entry4(const Fte4& fte)
{
    list<FibConfigEntrySet*>::iterator fibconfig_entry_set_iter;

    if (_fibconfig_entry_sets.empty())
	return (false);

    if (_profile.enabled(profile_route_out))
	_profile.log(profile_route_out,
		     c_format("add %s", fte.net().str().c_str()));

    for (fibconfig_entry_set_iter = _fibconfig_entry_sets.begin();
	 fibconfig_entry_set_iter != _fibconfig_entry_sets.end();
	 ++fibconfig_entry_set_iter) {
	FibConfigEntrySet* fibconfig_entry_set = *fibconfig_entry_set_iter;
	if (fibconfig_entry_set->add_entry4(fte) != true)
	    return (false);
    }

    return (true);
}

bool
FibConfig::delete_entry4(const Fte4& fte)
{
    list<FibConfigEntrySet*>::iterator fibconfig_entry_set_iter;

    if (_fibconfig_entry_sets.empty())
	return (false);

    if (_profile.enabled(profile_route_out))
	_profile.log(profile_route_out,
		     c_format("delete %s", fte.net().str().c_str()));

    for (fibconfig_entry_set_iter = _fibconfig_entry_sets.begin();
	 fibconfig_entry_set_iter != _fibconfig_entry_sets.end();
	 ++fibconfig_entry_set_iter) {
	FibConfigEntrySet* fibconfig_entry_set = *fibconfig_entry_set_iter;
	if (fibconfig_entry_set->delete_entry4(fte) != true)
	    return (false);
    }

    return (true);
}

bool
FibConfig::set_table4(const list<Fte4>& fte_list)
{
    list<FibConfigTableSet*>::iterator fibconfig_table_set_iter;

    if (_fibconfig_table_sets.empty())
	return (false);

    for (fibconfig_table_set_iter = _fibconfig_table_sets.begin();
	 fibconfig_table_set_iter != _fibconfig_table_sets.end();
	 ++fibconfig_table_set_iter) {
	FibConfigTableSet* fibconfig_table_set = *fibconfig_table_set_iter;
	if (fibconfig_table_set->set_table4(fte_list) != true)
	    return (false);
    }

    return (true);
}

bool
FibConfig::delete_all_entries4()
{
    list<FibConfigTableSet*>::iterator fibconfig_table_set_iter;

    if (_fibconfig_table_sets.empty())
	return (false);

    for (fibconfig_table_set_iter = _fibconfig_table_sets.begin();
	 fibconfig_table_set_iter != _fibconfig_table_sets.end();
	 ++fibconfig_table_set_iter) {
	FibConfigTableSet* fibconfig_table_set = *fibconfig_table_set_iter;
	if (fibconfig_table_set->delete_all_entries4() != true)
	    return (false);
    }

    return (true);
}

bool
FibConfig::lookup_route_by_dest4(const IPv4& dst, Fte4& fte)
{
    if (_fibconfig_entry_gets.empty())
	return (false);

    //
    // XXX: We pull the information by using only the first method.
    // In the future we need to rething this and be more flexible.
    //
    if (_fibconfig_entry_gets.front()->lookup_route_by_dest4(dst, fte) != true)
	return (false);

    return (true);
}

bool
FibConfig::lookup_route_by_network4(const IPv4Net& dst, Fte4& fte)
{
    if (_fibconfig_entry_gets.empty())
	return (false);

    //
    // XXX: We pull the information by using only the first method.
    // In the future we need to rething this and be more flexible.
    //
    if (_fibconfig_entry_gets.front()->lookup_route_by_network4(dst, fte) != true)
	return (false);

    return (true);
}

bool
FibConfig::get_table4(list<Fte4>& fte_list)
{
    if (_fibconfig_table_gets.empty())
	return (false);

    //
    // XXX: We pull the information by using only the first method.
    // In the future we need to rething this and be more flexible.
    //
    if (_fibconfig_table_gets.front()->get_table4(fte_list) != true)
	return (false);

    return (true);
}

bool
FibConfig::add_entry6(const Fte6& fte)
{
    list<FibConfigEntrySet*>::iterator fibconfig_entry_set_iter;

    if (_fibconfig_entry_sets.empty())
	return (false);

    if (_profile.enabled(profile_route_out))
	_profile.log(profile_route_out,
		     c_format("add %s", fte.net().str().c_str()));

    for (fibconfig_entry_set_iter = _fibconfig_entry_sets.begin();
	 fibconfig_entry_set_iter != _fibconfig_entry_sets.end();
	 ++fibconfig_entry_set_iter) {
	FibConfigEntrySet* fibconfig_entry_set = *fibconfig_entry_set_iter;
	if (fibconfig_entry_set->add_entry6(fte) != true)
	    return (false);
    }

    return (true);
}

bool
FibConfig::delete_entry6(const Fte6& fte)
{
    list<FibConfigEntrySet*>::iterator fibconfig_entry_set_iter;

    if (_fibconfig_entry_sets.empty())
	return (false);

    if (_profile.enabled(profile_route_out))
	_profile.log(profile_route_out,
		     c_format("delete %s", fte.net().str().c_str()));

    for (fibconfig_entry_set_iter = _fibconfig_entry_sets.begin();
	 fibconfig_entry_set_iter != _fibconfig_entry_sets.end();
	 ++fibconfig_entry_set_iter) {
	FibConfigEntrySet* fibconfig_entry_set = *fibconfig_entry_set_iter;
	if (fibconfig_entry_set->delete_entry6(fte) != true)
	    return (false);
    }

    return (true);
}

bool
FibConfig::set_table6(const list<Fte6>& fte_list)
{
    list<FibConfigTableSet*>::iterator fibconfig_table_set_iter;

    if (_fibconfig_table_sets.empty())
	return (false);

    for (fibconfig_table_set_iter = _fibconfig_table_sets.begin();
	 fibconfig_table_set_iter != _fibconfig_table_sets.end();
	 ++fibconfig_table_set_iter) {
	FibConfigTableSet* fibconfig_table_set = *fibconfig_table_set_iter;
	if (fibconfig_table_set->set_table6(fte_list) != true)
	    return (false);
    }

    return (true);
}

bool
FibConfig::delete_all_entries6()
{
    list<FibConfigTableSet*>::iterator fibconfig_table_set_iter;

    if (_fibconfig_table_sets.empty())
	return (false);

    for (fibconfig_table_set_iter = _fibconfig_table_sets.begin();
	 fibconfig_table_set_iter != _fibconfig_table_sets.end();
	 ++fibconfig_table_set_iter) {
	FibConfigTableSet* fibconfig_table_set = *fibconfig_table_set_iter;
	if (fibconfig_table_set->delete_all_entries6() != true)
	    return (false);
    }

    return (true);
}

bool
FibConfig::lookup_route_by_dest6(const IPv6& dst, Fte6& fte)
{
    if (_fibconfig_entry_gets.empty())
	return (false);

    //
    // XXX: We pull the information by using only the first method.
    // In the future we need to rething this and be more flexible.
    //
    if (_fibconfig_entry_gets.front()->lookup_route_by_dest6(dst, fte) != true)
	return (false);

    return (true);
}

bool
FibConfig::lookup_route_by_network6(const IPv6Net& dst, Fte6& fte)
{
    if (_fibconfig_entry_gets.empty())
	return (false);

    //
    // XXX: We pull the information by using only the first method.
    // In the future we need to rething this and be more flexible.
    //
    if (_fibconfig_entry_gets.front()->lookup_route_by_network6(dst, fte) != true)
	return (false);

    return (true);
}

bool
FibConfig::get_table6(list<Fte6>& fte_list)
{
    if (_fibconfig_table_gets.empty())
	return (false);

    //
    // XXX: We pull the information by using only the first method.
    // In the future we need to rething this and be more flexible.
    //
    if (_fibconfig_table_gets.front()->get_table6(fte_list) != true)
	return (false);

    return (true);
}

bool
FibConfig::add_fib_table_observer(FibTableObserverBase* fib_table_observer)
{
    if (find(_fib_table_observers.begin(),
	     _fib_table_observers.end(),
	     fib_table_observer)
	!= _fib_table_observers.end()) {
	// XXX: we have already added that observer
	return (true);
    }

    _fib_table_observers.push_back(fib_table_observer);

    return (true);
}

bool
FibConfig::delete_fib_table_observer(FibTableObserverBase* fib_table_observer)
{
    list<FibTableObserverBase* >::iterator iter;

    iter = find(_fib_table_observers.begin(),
		_fib_table_observers.end(),
		fib_table_observer);
    if (iter == _fib_table_observers.end()) {
	// XXX: observer not found
	return (false);
    }

    _fib_table_observers.erase(iter);

    return (true);
}

void
FibConfig::propagate_fib_changes(const list<FteX>& fte_list,
				 const FibConfigTableObserver* fibconfig_table_observer)
{
    list<Fte4> fte_list4;
    list<Fte6> fte_list6;
    list<FteX>::const_iterator ftex_iter;

    //
    // XXX: propagate the changes only from the first method.
    // In the future we need to rething this and be more flexible.
    //
    if (_fibconfig_table_observers.empty()
	|| (_fibconfig_table_observers.front() != fibconfig_table_observer)) {
	return;
    }

    if (fte_list.empty())
	return;

    // Copy the FteX list into Fte4 and Fte6 lists
    for (ftex_iter = fte_list.begin();
	 ftex_iter != fte_list.end();
	 ++ftex_iter) {
	const FteX& ftex = *ftex_iter;
	if (ftex.net().is_ipv4()) {
	    // IPv4 entry
	    Fte4 fte4 = ftex.get_fte4();
	    fte_list4.push_back(fte4);
	}

	if (ftex.net().is_ipv6()) {
	    // IPv6 entry
	    Fte6 fte6 = ftex.get_fte6();
	    fte_list6.push_back(fte6);
	}
    }

    // Inform all observers about the changes
    list<FibTableObserverBase* >::iterator iter;
    for (iter = _fib_table_observers.begin();
	 iter != _fib_table_observers.end();
	 ++iter) {
	FibTableObserverBase* fib_table_observer = *iter;
	if (! fte_list4.empty())
	    fib_table_observer->process_fib_changes(fte_list4);
	if (! fte_list6.empty())
	    fib_table_observer->process_fib_changes(fte_list6);
    }
}

/**
 * Test if the underlying system supports IPv4.
 * 
 * @return true if the underlying system supports IPv4, otherwise false.
 */
bool
FibConfig::test_have_ipv4() const
{
    // XXX: always return true if running in dummy mode
    if (_fea_node.is_dummy())
	return (true);

    XorpFd s = comm_sock_open(AF_INET, SOCK_DGRAM, 0, 0);
    if (!s.is_valid())
	return (false);
   
    comm_close(s);

    return (true);
}

/**
 * Test if the underlying system supports IPv6.
 * 
 * @return true if the underlying system supports IPv6, otherwise false.
 */
bool
FibConfig::test_have_ipv6() const
{
    // XXX: always return true if running in dummy mode
    if (_fea_node.is_dummy())
	return (true);

#ifndef HAVE_IPV6
    return (false);
#else
    XorpFd s = comm_sock_open(AF_INET6, SOCK_DGRAM, 0, 0);
    if (!s.is_valid())
	return (false);
    
    comm_close(s);
    return (true);
#endif // HAVE_IPV6
}

/**
 * Test whether the IPv4 unicast forwarding engine is enabled or disabled
 * to forward packets.
 * 
 * @param ret_value if true on return, then the IPv4 unicast forwarding
 * is enabled, otherwise is disabled.
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
FibConfig::unicast_forwarding_enabled4(bool& ret_value, string& error_msg) const
{
    // XXX: always return true if running in dummy mode
    if (_fea_node.is_dummy()) {
	ret_value = true;
	return (XORP_OK);
    }

    if (! have_ipv4()) {
	ret_value = false;
	error_msg = c_format("Cannot test whether IPv4 unicast forwarding "
			     "is enabled: IPv4 is not supported");
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }

    int enabled = 0;
    
#if defined(CTL_NET) && defined(IPPROTO_IP) && defined(IPCTL_FORWARDING)
    {
	size_t sz = sizeof(enabled);
	int mib[4];
	
	mib[0] = CTL_NET;
	mib[1] = AF_INET;
	mib[2] = IPPROTO_IP;
	mib[3] = IPCTL_FORWARDING;
	
	if (sysctl(mib, sizeof(mib) / sizeof(mib[0]), &enabled, &sz, NULL, 0)
	    != 0) {
	    error_msg = c_format("Get sysctl(IPCTL_FORWARDING) failed: %s",
				 strerror(errno));
	    XLOG_ERROR("%s", error_msg.c_str());
	    return (XORP_ERROR);
	}
    }
#elif defined(HOST_OS_LINUX)
    {
	FILE *fh = fopen(PROC_LINUX_FILE_FORWARDING_V4, "r");
	
	if (fh == NULL) {
	    error_msg = c_format("Cannot open file %s for reading: %s",
				 PROC_LINUX_FILE_FORWARDING_V4,
				 strerror(errno));
	    XLOG_ERROR("%s", error_msg.c_str());
	    return (XORP_ERROR);
	}
	if (fscanf(fh, "%d", &enabled) != 1) {
	    error_msg = c_format("Error reading file %s: %s",
				 PROC_LINUX_FILE_FORWARDING_V4,
				 strerror(errno));
	    XLOG_ERROR("%s", error_msg.c_str());
	    fclose(fh);
	    return (XORP_ERROR);
	}
	fclose(fh);
    }
#elif defined(HOST_OS_SOLARIS)
    {
	struct strioctl strioctl;
	char buf[256];
	int fd;

	fd = open(DEV_SOLARIS_DRIVER_FORWARDING_V4, O_RDONLY);
	if (fd < 0) {
	    error_msg = c_format("Cannot open file %s for reading: %s",
				 DEV_SOLARIS_DRIVER_FORWARDING_V4,
				 strerror(errno));
	    XLOG_ERROR("%s", error_msg.c_str());
	    return (XORP_ERROR);
	}
	int r = isastream(fd);
	if (r < 0) {
	    error_msg = c_format("Error testing whether file %s is a stream: %s",
				 DEV_SOLARIS_DRIVER_FORWARDING_V4,
				 strerror(errno));
	    XLOG_ERROR("%s", error_msg.c_str());
	    close(fd);
	    return (XORP_ERROR);
	}
	if (r == 0) {
	    error_msg = c_format("File %s is not a stream",
				 DEV_SOLARIS_DRIVER_FORWARDING_V4);
	    XLOG_ERROR("%s", error_msg.c_str());
	    close(fd);
	    return (XORP_ERROR);
	}

	memset(&strioctl, 0, sizeof(strioctl));
	memset(buf, 0, sizeof(buf));
	strncpy(buf, DEV_SOLARIS_DRIVER_PARAMETER_FORWARDING_V4,
		sizeof(buf) - 1);
	strioctl.ic_cmd = ND_GET;
	strioctl.ic_timout = 0;
	strioctl.ic_len = sizeof(buf);
	strioctl.ic_dp = buf;
	if (ioctl(fd, I_STR, &strioctl) < 0) {
	    error_msg = c_format("Error testing whether IPv4 unicast "
				 "forwarding is enabled: %s",
				 strerror(errno));
	    XLOG_ERROR("%s", error_msg.c_str());
	    close(fd);
	    return (XORP_ERROR);
	}
	if (sscanf(buf, "%d", &enabled) != 1) {
	    error_msg = c_format("Error reading result %s: %s",
				 buf, strerror(errno));
	    XLOG_ERROR("%s", error_msg.c_str());
	    close(fd);
	    return (XORP_ERROR);
	}
	close(fd);
    }
#elif defined(HOST_OS_WINDOWS)
    {
	MIB_IPSTATS ipstats;
	DWORD error = GetIpStatistics(&ipstats);
	if (error != NO_ERROR) {
	    XLOG_ERROR("GetIpStatistics() failed: %s",
		       win_strerror(GetLastError()));
	    return (XORP_ERROR);
	}
	enabled = (int)(ipstats.dwForwarding == MIB_IP_FORWARDING);
    }
#else
#error "OS not supported: don't know how to test whether"
#error "IPv4 unicast forwarding is enabled/disabled"
#endif
    
    if (enabled > 0)
	ret_value = true;
    else
	ret_value = false;
    
    return (XORP_OK);
}

/**
 * Test whether the IPv6 unicast forwarding engine is enabled or disabled
 * to forward packets.
 * 
 * @param ret_value if true on return, then the IPv6 unicast forwarding
 * is enabled, otherwise is disabled.
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
FibConfig::unicast_forwarding_enabled6(bool& ret_value, string& error_msg) const
{
    // XXX: always return true if running in dummy mode
    if (_fea_node.is_dummy()) {
	ret_value = true;
	return (XORP_OK);
    }

#ifndef HAVE_IPV6
    ret_value = false;
    error_msg = c_format("Cannot test whether IPv6 unicast forwarding "
			 "is enabled: IPv6 is not supported");
    XLOG_ERROR("%s", error_msg.c_str());
    return (XORP_ERROR);
    
#else // HAVE_IPV6

    if (! have_ipv6()) {
	ret_value = false;
	error_msg = c_format("Cannot test whether IPv6 unicast forwarding "
			     "is enabled: IPv6 is not supported");
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    int enabled = 0;
    
#if defined(CTL_NET) && defined(IPPROTO_IPV6) && defined(IPV6CTL_FORWARDING)
    {
	size_t sz = sizeof(enabled);
	int mib[4];
	
	mib[0] = CTL_NET;
	mib[1] = AF_INET6;
	mib[2] = IPPROTO_IPV6;
	mib[3] = IPV6CTL_FORWARDING;
	
	if (sysctl(mib, sizeof(mib) / sizeof(mib[0]), &enabled, &sz, NULL, 0)
	    != 0) {
	    error_msg = c_format("Get sysctl(IPV6CTL_FORWARDING) failed: %s",
				 strerror(errno));
	    XLOG_ERROR("%s", error_msg.c_str());
	    return (XORP_ERROR);
	}
    }
#elif defined(HOST_OS_LINUX)
    {
	FILE *fh = fopen(PROC_LINUX_FILE_FORWARDING_V6, "r");
	
	if (fh == NULL) {
	    error_msg = c_format("Cannot open file %s for reading: %s",
				 PROC_LINUX_FILE_FORWARDING_V6,
				 strerror(errno));
	    XLOG_ERROR("%s", error_msg.c_str());
	    return (XORP_ERROR);
	}
	if (fscanf(fh, "%d", &enabled) != 1) {
	    error_msg = c_format("Error reading file %s: %s",
				 PROC_LINUX_FILE_FORWARDING_V6,
				 strerror(errno));
	    XLOG_ERROR("%s", error_msg.c_str());
	    fclose(fh);
	    return (XORP_ERROR);
	}
	fclose(fh);
    }
#elif defined(HOST_OS_SOLARIS)
    {
	struct strioctl strioctl;
	char buf[256];
	int fd;

	fd = open(DEV_SOLARIS_DRIVER_FORWARDING_V6, O_RDONLY);
	if (fd < 0) {
	    error_msg = c_format("Cannot open file %s for reading: %s",
				 DEV_SOLARIS_DRIVER_FORWARDING_V6,
				 strerror(errno));
	    XLOG_ERROR("%s", error_msg.c_str());
	    return (XORP_ERROR);
	}
	int r = isastream(fd);
	if (r < 0) {
	    error_msg = c_format("Error testing whether file %s is a stream: %s",
				 DEV_SOLARIS_DRIVER_FORWARDING_V6,
				 strerror(errno));
	    XLOG_ERROR("%s", error_msg.c_str());
	    close(fd);
	    return (XORP_ERROR);
	}
	if (r == 0) {
	    error_msg = c_format("File %s is not a stream",
				 DEV_SOLARIS_DRIVER_FORWARDING_V6);
	    XLOG_ERROR("%s", error_msg.c_str());
	    close(fd);
	    return (XORP_ERROR);
	}

	memset(&strioctl, 0, sizeof(strioctl));
	memset(buf, 0, sizeof(buf));
	strncpy(buf, DEV_SOLARIS_DRIVER_PARAMETER_FORWARDING_V6,
		sizeof(buf) - 1);
	strioctl.ic_cmd = ND_GET;
	strioctl.ic_timout = 0;
	strioctl.ic_len = sizeof(buf);
	strioctl.ic_dp = buf;
	if (ioctl(fd, I_STR, &strioctl) < 0) {
	    error_msg = c_format("Error testing whether IPv6 unicast "
				 "forwarding is enabled: %s",
				 strerror(errno));
	    XLOG_ERROR("%s", error_msg.c_str());
	    close(fd);
	    return (XORP_ERROR);
	}
	if (sscanf(buf, "%d", &enabled) != 1) {
	    error_msg = c_format("Error reading result %s: %s",
				 buf, strerror(errno));
	    XLOG_ERROR("%s", error_msg.c_str());
	    close(fd);
	    return (XORP_ERROR);
	}
	close(fd);
    }
#elif defined(HOST_OS_WINDOWS) && 0
    // XXX: Not in MinGW w32api yet.
    {
	MIB_IPSTATS ipstats;
	DWORD error = GetIpStatisticsEx(&ipstats, AF_INET6);
	if (error != NO_ERROR) {
	    XLOG_ERROR("GetIpStatisticsEx() failed: %s",
		       win_strerror(GetLastError()));
	    return (XORP_ERROR);
	}
	enabled = (int)(ipstats.dwForwarding == MIB_IP_FORWARDING);
    }
#else
#error "OS not supported: don't know how to test whether"
#error "IPv6 unicast forwarding is enabled/disabled"
#endif
    
    if (enabled > 0)
	ret_value = true;
    else
	ret_value = false;
    
    return (XORP_OK);
#endif // HAVE_IPV6
}

/**
 * Test whether the acceptance of IPv6 Router Advertisement messages is
 * enabled or disabled.
 * 
 * @param ret_value if true on return, then the acceptance of IPv6 Router
 * Advertisement messages is enabled, otherwise is disabled.
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
FibConfig::accept_rtadv_enabled6(bool& ret_value, string& error_msg) const
{
    // XXX: always return true if running in dummy mode
    if (_fea_node.is_dummy()) {
	ret_value = true;
	return (XORP_OK);
    }

#ifndef HAVE_IPV6
    ret_value = false;
    error_msg = c_format("Cannot test whether the acceptance of IPv6 "
			 "Router Advertisement messages is enabled: "
			 "IPv6 is not supported");
    XLOG_ERROR("%s", error_msg.c_str());
    return (XORP_ERROR);
    
#else // HAVE_IPV6

    if (! have_ipv6()) {
	ret_value = false;
	error_msg = c_format("Cannot test whether the acceptance of IPv6 "
			     "Router Advertisement messages is enabled: "
			     "IPv6 is not supported");
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    int enabled = 0;
    
#if defined(CTL_NET) && defined(IPPROTO_IPV6) && defined(IPV6CTL_ACCEPT_RTADV)
    {
	size_t sz = sizeof(enabled);
	int mib[4];
	
	mib[0] = CTL_NET;
	mib[1] = AF_INET6;
	mib[2] = IPPROTO_IPV6;
	mib[3] = IPV6CTL_ACCEPT_RTADV;
	
	if (sysctl(mib, sizeof(mib) / sizeof(mib[0]), &enabled, &sz, NULL, 0)
	    != 0) {
	    error_msg = c_format("Get sysctl(IPV6CTL_ACCEPT_RTADV) failed: %s",
				 strerror(errno));
	    XLOG_ERROR("%s", error_msg.c_str());
	    return (XORP_ERROR);
	}
    }
#elif defined(HOST_OS_LINUX)
    // XXX: nothing to do in case of Linux
    error_msg = "";
#elif defined(HOST_OS_SOLARIS)
    {
	struct strioctl strioctl;
	char buf[256];
	int fd;
	int ignore_redirect = 0;

	fd = open(DEV_SOLARIS_DRIVER_FORWARDING_V6, O_RDONLY);
	if (fd < 0) {
	    error_msg = c_format("Cannot open file %s for reading: %s",
				 DEV_SOLARIS_DRIVER_FORWARDING_V6,
				 strerror(errno));
	    XLOG_ERROR("%s", error_msg.c_str());
	    return (XORP_ERROR);
	}
	int r = isastream(fd);
	if (r < 0) {
	    error_msg = c_format("Error testing whether file %s is a stream: %s",
				 DEV_SOLARIS_DRIVER_FORWARDING_V6,
				 strerror(errno));
	    XLOG_ERROR("%s", error_msg.c_str());
	    close(fd);
	    return (XORP_ERROR);
	}
	if (r == 0) {
	    error_msg = c_format("File %s is not a stream",
				 DEV_SOLARIS_DRIVER_FORWARDING_V6);
	    XLOG_ERROR("%s", error_msg.c_str());
	    close(fd);
	    return (XORP_ERROR);
	}

	memset(&strioctl, 0, sizeof(strioctl));
	memset(buf, 0, sizeof(buf));
	strncpy(buf, DEV_SOLARIS_DRIVER_PARAMETER_IGNORE_REDIRECT_V6,
		sizeof(buf) - 1);
	strioctl.ic_cmd = ND_GET;
	strioctl.ic_timout = 0;
	strioctl.ic_len = sizeof(buf);
	strioctl.ic_dp = buf;
	if (ioctl(fd, I_STR, &strioctl) < 0) {
	    error_msg = c_format("Error testing whether the acceptance of "
				 "IPv6 Router Advertisement messages is "
				 "enabled: %s",
				 strerror(errno));
	    XLOG_ERROR("%s", error_msg.c_str());
	    close(fd);
	    return (XORP_ERROR);
	}
	if (sscanf(buf, "%d", &ignore_redirect) != 1) {
	    error_msg = c_format("Error reading result %s: %s",
				 buf, strerror(errno));
	    XLOG_ERROR("%s", error_msg.c_str());
	    close(fd);
	    return (XORP_ERROR);
	}
	close(fd);

	//
	// XXX: The logic of "Accept IPv6 Router Advertisement" is just the
	// opposite of "Ignore Redirect".
	//
	if (ignore_redirect == 0)
	    enabled = 1;
	else
	    enabled = 0;
    }
#else
#error "OS not supported: don't know how to test whether"
#error "the acceptance of IPv6 Router Advertisement messages"
#error "is enabled/disabled"
#endif
    
    if (enabled > 0)
	ret_value = true;
    else
	ret_value = false;
    
    return (XORP_OK);
#endif // HAVE_IPV6
}

/**
 * Set the IPv4 unicast forwarding engine to enable or disable forwarding
 * of packets.
 * 
 * @param v if true, then enable IPv4 unicast forwarding, otherwise
 * disable it.
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
FibConfig::set_unicast_forwarding_enabled4(bool v, string& error_msg)
{
    // XXX: don't do anything if running in dummy mode
    if (_fea_node.is_dummy())
	return (XORP_OK);

    if (! have_ipv4()) {
	if (! v) {
	    //
	    // XXX: we assume that "not supported" == "disable", hence
	    // return OK.
	    //
	    return (XORP_OK);
	}
	error_msg = c_format("Cannot set IPv4 unicast forwarding to %s: "
			     "IPv4 is not supported", bool_c_str(v));
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }

    int enable = (v) ? 1 : 0;
    bool old_value;
    
    if (unicast_forwarding_enabled4(old_value, error_msg) < 0)
	return (XORP_ERROR);
    
    if (old_value == v)
	return (XORP_OK);	// Nothing changed
    
#if defined(CTL_NET) && defined(IPPROTO_IP) && defined(IPCTL_FORWARDING)
    {
	size_t sz = sizeof(enable);
	int mib[4];
	
	mib[0] = CTL_NET;
	mib[1] = AF_INET;
	mib[2] = IPPROTO_IP;
	mib[3] = IPCTL_FORWARDING;
	
	if (sysctl(mib, sizeof(mib) / sizeof(mib[0]), NULL, NULL, &enable, sz)
	    != 0) {
	    error_msg = c_format("Set sysctl(IPCTL_FORWARDING) to %s failed: %s",
				 bool_c_str(v), strerror(errno));
	    XLOG_ERROR("%s", error_msg.c_str());
	    return (XORP_ERROR);
	}
    }
#elif defined(HOST_OS_LINUX)
    {
	FILE *fh = fopen(PROC_LINUX_FILE_FORWARDING_V4, "w");
	
	if (fh == NULL) {
	    error_msg = c_format("Cannot open file %s for writing: %s",
				 PROC_LINUX_FILE_FORWARDING_V4,
				 strerror(errno));
	    XLOG_ERROR("%s", error_msg.c_str());
	    return (XORP_ERROR);
	}
	if (fprintf(fh, "%d", enable) != 1) {
	    error_msg = c_format("Error writing %d to file %s: %s",
				 enable,
				 PROC_LINUX_FILE_FORWARDING_V4,
				 strerror(errno));
	    XLOG_ERROR("%s", error_msg.c_str());
	    fclose(fh);
	    return (XORP_ERROR);
	}
	fclose(fh);
    }
#elif defined(HOST_OS_SOLARIS)
    {
	struct strioctl strioctl;
	char buf[256];
	int fd;

	fd = open(DEV_SOLARIS_DRIVER_FORWARDING_V4, O_WRONLY);
	if (fd < 0) {
	    error_msg = c_format("Cannot open file %s for writing: %s",
				 DEV_SOLARIS_DRIVER_FORWARDING_V4,
				 strerror(errno));
	    XLOG_ERROR("%s", error_msg.c_str());
	    return (XORP_ERROR);
	}
	int r = isastream(fd);
	if (r < 0) {
	    error_msg = c_format("Error testing whether file %s is a stream: %s",
				 DEV_SOLARIS_DRIVER_FORWARDING_V4,
				 strerror(errno));
	    XLOG_ERROR("%s", error_msg.c_str());
	    close(fd);
	    return (XORP_ERROR);
	}
	if (r == 0) {
	    error_msg = c_format("File %s is not a stream",
				 DEV_SOLARIS_DRIVER_FORWARDING_V4);
	    XLOG_ERROR("%s", error_msg.c_str());
	    close(fd);
	    return (XORP_ERROR);
	}

	memset(&strioctl, 0, sizeof(strioctl));
	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf) - 1, "%s %d",
		 DEV_SOLARIS_DRIVER_PARAMETER_FORWARDING_V4, enable);
	strioctl.ic_cmd = ND_SET;
	strioctl.ic_timout = 0;
	strioctl.ic_len = sizeof(buf);
	strioctl.ic_dp = buf;
	if (ioctl(fd, I_STR, &strioctl) < 0) {
	    error_msg = c_format("Cannot set IPv4 unicast forwarding to %s: %s",
				 bool_c_str(v), strerror(errno));
	    XLOG_ERROR("%s", error_msg.c_str());
	    close(fd);
	    return (XORP_ERROR);
	}
	close(fd);
    }
#elif defined(HOST_OS_WINDOWS)
    if (enable) {
	if (WinSupport::is_rras_running()) {
	    XLOG_WARNING("RRAS is running; ignoring request to enable "
			 "IPv4 forwarding.");
	    return (XORP_OK);
	}
	HANDLE hFwd;
	DWORD result = EnableRouter(&hFwd, &_overlapped);
	if (result != ERROR_IO_PENDING) {
	    error_msg = c_format("Error '%s' from EnableRouter",
				 win_strerror(GetLastError()));
	    XLOG_ERROR("%s", error_msg.c_str());
	    return (XORP_OK);	// XXX: This error is non-fatal.
	}
	++_enablecnt;
    } else {
	if (WinSupport::is_rras_running()) {
	    XLOG_WARNING("RRAS is running; ignoring request to disable "
			 "IPv4 forwarding.");
	    return (XORP_OK);
	}
	if (_enablecnt == 0) {
	    error_msg = c_format("UnenableRouter() called without any previous "
				 "call to EnableRouter()");
	    XLOG_ERROR("%s", error_msg.c_str());
	    return (XORP_OK);	// XXX: This error is non-fatal.
	}

	DWORD result = UnenableRouter(&_overlapped, NULL);
	if (result != NO_ERROR) {
	    error_msg = c_format("Error '%s' from UnenableRouter",
				 win_strerror(GetLastError()));
	    XLOG_ERROR("%s", error_msg.c_str());
	    return (XORP_OK);	// XXX: This error is non-fatal.
	}
	--_enablecnt;
    }
#else
#error "OS not supported: don't know how to enable/disable"
#error "IPv4 unicast forwarding"
#endif
    
    return (XORP_OK);
}

/**
 * Set the IPv6 unicast forwarding engine to enable or disable forwarding
 * of packets.
 * 
 * @param v if true, then enable IPv6 unicast forwarding, otherwise
 * disable it.
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
FibConfig::set_unicast_forwarding_enabled6(bool v, string& error_msg)
{
    // XXX: don't do anything if running in dummy mode
    if (_fea_node.is_dummy())
	return (XORP_OK);

#ifndef HAVE_IPV6
    if (! v) {
	//
	// XXX: we assume that "not supported" == "disable", hence
	// return OK.
	//
	return (XORP_OK);
    }

    error_msg = c_format("Cannot set IPv6 unicast forwarding to %s: "
			 "IPv6 is not supported", bool_c_str(v));
    XLOG_ERROR("%s", error_msg.c_str());
    return (XORP_ERROR);
    
#else // HAVE_IPV6

    if (! have_ipv6()) {
	if (! v) {
	    //
	    // XXX: we assume that "not supported" == "disable", hence
	    // return OK.
	    //
	    return (XORP_OK);
	}

	error_msg = c_format("Cannot set IPv6 unicast forwarding to %s: "
			     "IPv6 is not supported", bool_c_str(v));
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }

    int enable = (v) ? 1 : 0;
    bool old_value, old_value_accept_rtadv;
    
    if (unicast_forwarding_enabled6(old_value, error_msg) < 0)
	return (XORP_ERROR);
    if (accept_rtadv_enabled6(old_value_accept_rtadv, error_msg) < 0)
	return (XORP_ERROR);
    
    if ((old_value == v) && (old_value_accept_rtadv == !v))
	return (XORP_OK);	// Nothing changed
    
    if (set_accept_rtadv_enabled6(!v, error_msg) < 0)
	return (XORP_ERROR);
    
#if defined(CTL_NET) && defined(IPPROTO_IPV6) && defined(IPV6CTL_FORWARDING)
    {
	size_t sz = sizeof(enable);
	int mib[4];
	
	mib[0] = CTL_NET;
	mib[1] = AF_INET6;
	mib[2] = IPPROTO_IPV6;
	mib[3] = IPV6CTL_FORWARDING;
	
	if (sysctl(mib, sizeof(mib) / sizeof(mib[0]), NULL, NULL, &enable, sz)
	    != 0) {
	    error_msg = c_format("Set sysctl(IPV6CTL_FORWARDING) to %s failed: %s",
				 bool_c_str(v), strerror(errno));
	    XLOG_ERROR("%s", error_msg.c_str());
	    // Restore the old accept_rtadv value
	    if (old_value_accept_rtadv != !v) {
		string dummy_error_msg;
		set_accept_rtadv_enabled6(old_value_accept_rtadv,
					  dummy_error_msg);
	    }
	    return (XORP_ERROR);
	}
    }
#elif defined(HOST_OS_LINUX)
    {
	FILE *fh = fopen(PROC_LINUX_FILE_FORWARDING_V6, "w");
	
	if (fh == NULL) {
	    error_msg = c_format("Cannot open file %s for writing: %s",
				 PROC_LINUX_FILE_FORWARDING_V6,
				 strerror(errno));
	    XLOG_ERROR("%s", error_msg.c_str());
	    return (XORP_ERROR);
	}
	if (fprintf(fh, "%d", enable) != 1) {
	    error_msg = c_format("Error writing %d to file %s: %s",
				 enable,
				 PROC_LINUX_FILE_FORWARDING_V6,
				 strerror(errno));
	    XLOG_ERROR("%s", error_msg.c_str());
	    // Restore the old accept_rtadv value
	    if (old_value_accept_rtadv != !v) {
		string dummy_error_msg;
		set_accept_rtadv_enabled6(old_value_accept_rtadv,
					  dummy_error_msg);
	    }
	    fclose(fh);
	    return (XORP_ERROR);
	}
	fclose(fh);
    }
#elif defined(HOST_OS_SOLARIS)
    {
	struct strioctl strioctl;
	char buf[256];
	int fd;

	fd = open(DEV_SOLARIS_DRIVER_FORWARDING_V6, O_WRONLY);
	if (fd < 0) {
	    error_msg = c_format("Cannot open file %s for writing: %s",
				 DEV_SOLARIS_DRIVER_FORWARDING_V6,
				 strerror(errno));
	    XLOG_ERROR("%s", error_msg.c_str());
	    return (XORP_ERROR);
	}
	int r = isastream(fd);
	if (r < 0) {
	    error_msg = c_format("Error testing whether file %s is a stream: %s",
				 DEV_SOLARIS_DRIVER_FORWARDING_V6,
				 strerror(errno));
	    XLOG_ERROR("%s", error_msg.c_str());
	    close(fd);
	    return (XORP_ERROR);
	}
	if (r == 0) {
	    error_msg = c_format("File %s is not a stream",
				 DEV_SOLARIS_DRIVER_FORWARDING_V6);
	    XLOG_ERROR("%s", error_msg.c_str());
	    close(fd);
	    return (XORP_ERROR);
	}

	memset(&strioctl, 0, sizeof(strioctl));
	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf) - 1, "%s %d",
		 DEV_SOLARIS_DRIVER_PARAMETER_FORWARDING_V6, enable);
	strioctl.ic_cmd = ND_SET;
	strioctl.ic_timout = 0;
	strioctl.ic_len = sizeof(buf);
	strioctl.ic_dp = buf;
	if (ioctl(fd, I_STR, &strioctl) < 0) {
	    error_msg = c_format("Cannot set IPv6 unicast forwarding to %s: %s",
				 bool_c_str(v), strerror(errno));
	    XLOG_ERROR("%s", error_msg.c_str());
	    close(fd);
	    return (XORP_ERROR);
	}
	close(fd);
    }
#elif defined(HOST_OS_WINDOWS) && 0
    // XXX: Not yet in MinGW w32api
    {
	MIB_IPSTATS ipstats;
	DWORD error = GetIpStatisticsEx(&ipstats, AF_INET6);
	if (error != NO_ERROR) {
	    XLOG_ERROR("GetIpStatisticsEx() failed: %s",
		       win_strerror(GetLastError()));
	    return (XORP_ERROR);
	}
	ipstats.dwForwarding = (enable != 0) ? 1 : 0;
	ipstats.dwDefaultTTL = MIB_USE_CURRENT_TTL;
	error = SetIpStatisticsEx(&ipstats, AF_INET6);
	if (error != NO_ERROR) {
	    XLOG_ERROR("SetIpStatisticsEx() failed: %s",
		       win_strerror(GetLastError()));
	    return (XORP_ERROR);
	}
    }
#else
#error "OS not supported: don't know how to enable/disable"
#error "IPv6 unicast forwarding"
#endif
    
    return (XORP_OK);
#endif // HAVE_IPV6
}

/**
 * Enable or disable the acceptance of IPv6 Router Advertisement messages
 * from other routers. It should be enabled for hosts, and disabled for
 * routers.
 * 
 * @param v if true, then enable the acceptance of IPv6 Router Advertisement
 * messages, otherwise disable it.
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
FibConfig::set_accept_rtadv_enabled6(bool v, string& error_msg)
{
    // XXX: don't do anything if running in dummy mode
    if (_fea_node.is_dummy())
	return (XORP_OK);

#ifndef HAVE_IPV6
    if (! v) {
	//
	// XXX: we assume that "not supported" == "disable", hence
	// return OK.
	//
	return (XORP_OK);
    }

    error_msg = c_format("Cannot set the acceptance of IPv6 "
			 "Router Advertisement messages to %s: "
			 "IPv6 is not supported",
			 bool_c_str(v));
    XLOG_ERROR("%s", error_msg.c_str());
    return (XORP_ERROR);
    
#else // HAVE_IPV6

    if (! have_ipv6()) {
	if (! v) {
	    //
	    // XXX: we assume that "not supported" == "disable", hence
	    // return OK.
	    //
	    return (XORP_OK);
	}

	error_msg = c_format("Cannot set the acceptance of IPv6 "
			     "Router Advertisement messages to %s: "
			     "IPv6 is not supported",
			     bool_c_str(v));
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }

    int enable = (v) ? 1 : 0;
    bool old_value;
    
    if (accept_rtadv_enabled6(old_value, error_msg) < 0)
	return (XORP_ERROR);
    
    if (old_value == v)
	return (XORP_OK);	// Nothing changed
    
#if defined(CTL_NET) && defined(IPPROTO_IPV6) && defined(IPV6CTL_ACCEPT_RTADV)
    {
	size_t sz = sizeof(enable);
	int mib[4];
	
	mib[0] = CTL_NET;
	mib[1] = AF_INET6;
	mib[2] = IPPROTO_IPV6;
	mib[3] = IPV6CTL_ACCEPT_RTADV;
	
	if (sysctl(mib, sizeof(mib) / sizeof(mib[0]), NULL, NULL, &enable, sz)
	    != 0) {
	    error_msg = c_format("Set sysctl(IPV6CTL_ACCEPT_RTADV) to %s failed: %s",
				 bool_c_str(v), strerror(errno));
	    XLOG_ERROR("%s", error_msg.c_str());
	    return (XORP_ERROR);
	}
    }
#elif defined(HOST_OS_LINUX)
    {
	// XXX: nothing to do in case of Linux
	error_msg = "";
	UNUSED(enable);
    }
#elif defined(HOST_OS_SOLARIS)
    {
	struct strioctl strioctl;
	char buf[256];
	int fd;
	int ignore_redirect = 0;

	//
	// XXX: The logic of "Accept IPv6 Router Advertisement" is just the
	// opposite of "Ignore Redirect".
	//
	if (enable == 0)
	    ignore_redirect = 1;
	else
	    ignore_redirect = 0;

	fd = open(DEV_SOLARIS_DRIVER_FORWARDING_V6, O_WRONLY);
	if (fd < 0) {
	    error_msg = c_format("Cannot open file %s for writing: %s",
				 DEV_SOLARIS_DRIVER_FORWARDING_V6,
				 strerror(errno));
	    XLOG_ERROR("%s", error_msg.c_str());
	    return (XORP_ERROR);
	}
	int r = isastream(fd);
	if (r < 0) {
	    error_msg = c_format("Error testing whether file %s is a stream: %s",
				 DEV_SOLARIS_DRIVER_FORWARDING_V6,
				 strerror(errno));
	    XLOG_ERROR("%s", error_msg.c_str());
	    close(fd);
	    return (XORP_ERROR);
	}
	if (r == 0) {
	    error_msg = c_format("File %s is not a stream",
				 DEV_SOLARIS_DRIVER_FORWARDING_V6);
	    XLOG_ERROR("%s", error_msg.c_str());
	    close(fd);
	    return (XORP_ERROR);
	}

	memset(&strioctl, 0, sizeof(strioctl));
	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf) - 1, "%s %d",
		 DEV_SOLARIS_DRIVER_PARAMETER_IGNORE_REDIRECT_V6,
		 ignore_redirect);
	strioctl.ic_cmd = ND_SET;
	strioctl.ic_timout = 0;
	strioctl.ic_len = sizeof(buf);
	strioctl.ic_dp = buf;
	if (ioctl(fd, I_STR, &strioctl) < 0) {
	    error_msg = c_format("Cannot set IPv6 unicast forwarding to %s: %s",
				 bool_c_str(v), strerror(errno));
	    XLOG_ERROR("%s", error_msg.c_str());
	    close(fd);
	    return (XORP_ERROR);
	}
	close(fd);
    }
#elif defined(HOST_OS_WINDOWS)
    {
	// XXX: nothing to do in case of Windows
	error_msg = "";
	UNUSED(enable);
    }

#else
#error "OS not supported: don't know how to enable/disable"
#error "the acceptance of IPv6 Router Advertisement messages"
#endif
    
    return (XORP_OK);
#endif // HAVE_IPV6
}

/**
 * Set the IPv4 unicast forwarding engine whether to retain existing
 * XORP forwarding entries on startup.
 *
 * @param retain if true, then retain the XORP forwarding entries,
 * otherwise delete them.
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
FibConfig::set_unicast_forwarding_entries_retain_on_startup4(bool retain,
							     string& error_msg)
{
    _unicast_forwarding_entries_retain_on_startup4 = retain;

    error_msg = "";		// XXX: reset
    return (XORP_OK);
}

/**
 * Set the IPv4 unicast forwarding engine whether to retain existing
 * XORP forwarding entries on shutdown.
 *
 * @param retain if true, then retain the XORP forwarding entries,
 * otherwise delete them.
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
FibConfig::set_unicast_forwarding_entries_retain_on_shutdown4(bool retain,
							      string& error_msg)
{
    _unicast_forwarding_entries_retain_on_shutdown4 = retain;

    error_msg = "";		// XXX: reset
    return (XORP_OK);
}

/**
 * Set the IPv6 unicast forwarding engine whether to retain existing
 * XORP forwarding entries on startup.
 *
 * @param retain if true, then retain the XORP forwarding entries,
 * otherwise delete them.
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
FibConfig::set_unicast_forwarding_entries_retain_on_startup6(bool retain,
							     string& error_msg)
{
    _unicast_forwarding_entries_retain_on_startup6 = retain;

    error_msg = "";		// XXX: reset
    return (XORP_OK);
}

/**
 * Set the IPv6 unicast forwarding engine whether to retain existing
 * XORP forwarding entries on shutdown.
 *
 * @param retain if true, then retain the XORP forwarding entries,
 * otherwise delete them.
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
FibConfig::set_unicast_forwarding_entries_retain_on_shutdown6(bool retain,
							      string& error_msg)
{
    _unicast_forwarding_entries_retain_on_shutdown6 = retain;

    error_msg = "";		// XXX: reset
    return (XORP_OK);
}
