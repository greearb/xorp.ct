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

#ident "$XORP: xorp/fea/fticonfig.cc,v 1.53 2006/10/17 22:26:08 pavlin Exp $"

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
#include "fticonfig.hh"

#ifdef HOST_OS_WINDOWS
#include "libxorp/win_io.h"
#include "win_rtsock.h"
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


FtiConfig::FtiConfig(EventLoop& eventloop, Profile& profile,
		     const IfTree& iftree,
		     NexthopPortMapper& nexthop_port_mapper)
    : _eventloop(eventloop),
      _profile(profile),
      _nexthop_port_mapper(nexthop_port_mapper),
      _iftree(iftree),
      _ftic_entry_get_primary(NULL),
      _ftic_entry_set_primary(NULL),
      _ftic_entry_observer_primary(NULL),
      _ftic_table_get_primary(NULL),
      _ftic_table_set_primary(NULL),
      _ftic_table_observer_primary(NULL),
      _ftic_entry_get_dummy(*this),
      _ftic_entry_get_rtsock(*this),
      _ftic_entry_get_netlink(*this),
      _ftic_entry_get_iphelper(*this),
      //_ftic_entry_get_rtmv2(*this),
      _ftic_entry_get_click(*this),
      _ftic_entry_set_dummy(*this),
      _ftic_entry_set_rtsock(*this),
      _ftic_entry_set_netlink(*this),
      _ftic_entry_set_iphelper(*this),
      _ftic_entry_set_rtmv2(*this),
      _ftic_entry_set_click(*this),
      _ftic_entry_observer_dummy(*this),
      _ftic_entry_observer_rtsock(*this),
      _ftic_entry_observer_netlink(*this),
      _ftic_entry_observer_iphelper(*this),
      //_ftic_entry_observer_rtmv2(*this),
      _ftic_table_get_dummy(*this),
      _ftic_table_get_sysctl(*this),
      _ftic_table_get_netlink(*this),
      _ftic_table_get_iphelper(*this),
      _ftic_table_get_click(*this),
      _ftic_table_set_dummy(*this),
      _ftic_table_set_rtsock(*this),
      _ftic_table_set_netlink(*this),
      _ftic_table_set_iphelper(*this),
      //_ftic_table_set_rtmv2(*this),
      _ftic_table_set_click(*this),
      _ftic_table_observer_dummy(*this),
      _ftic_table_observer_rtsock(*this),
      _ftic_table_observer_netlink(*this),
      _ftic_table_observer_iphelper(*this),
      _ftic_table_observer_rtmv2(*this),
      _unicast_forwarding_enabled4(false),
      _unicast_forwarding_enabled6(false),
      _accept_rtadv_enabled6(false),
      _unicast_forwarding_entries_retain_on_startup4(false),
      _unicast_forwarding_entries_retain_on_shutdown4(false),
      _unicast_forwarding_entries_retain_on_startup6(false),
      _unicast_forwarding_entries_retain_on_shutdown6(false),
      _have_ipv4(false),
      _have_ipv6(false),
      _is_dummy(false),
      _is_running(false)
{
    string error_msg;

    //
    // Check that all necessary mechanisms to interact with the
    // underlying system are in place.
    //
    if (_ftic_entry_get_primary == NULL) {
	XLOG_FATAL("No primary mechanism to get forwarding table entries "
		   "from the underlying system");
    }
    if (_ftic_entry_set_primary == NULL) {
	XLOG_FATAL("No primary mechanism to set forwarding table entries "
		   "into the underlying system");
    }
    if (_ftic_entry_observer_primary == NULL) {
	XLOG_FATAL("No primary mechanism to observe forwarding table entries "
		   "from the underlying system");
    }
    if (_ftic_table_get_primary == NULL) {
	XLOG_FATAL("No primary mechanism to get the forwarding table "
		   "information from the underlying system");
    }
    if (_ftic_table_set_primary == NULL) {
	XLOG_FATAL("No primary mechanism to set the forwarding table "
		   "information into the underlying system");
    }
    if (_ftic_table_observer_primary == NULL) {
	XLOG_FATAL("No primary mechanism to observe the forwarding table "
		   "information from the underlying system");
    }

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

FtiConfig::~FtiConfig()
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
}

int
FtiConfig::register_ftic_entry_get_primary(FtiConfigEntryGet *ftic_entry_get)
{
    _ftic_entry_get_primary = ftic_entry_get;
    if (ftic_entry_get != NULL)
	ftic_entry_get->set_primary();
    
    return (XORP_OK);
}

int
FtiConfig::register_ftic_entry_set_primary(FtiConfigEntrySet *ftic_entry_set)
{
    _ftic_entry_set_primary = ftic_entry_set;
    if (ftic_entry_set != NULL)
	ftic_entry_set->set_primary();
    
    return (XORP_OK);
}

int
FtiConfig::register_ftic_entry_observer_primary(FtiConfigEntryObserver *ftic_entry_observer)
{
    _ftic_entry_observer_primary = ftic_entry_observer;
    if (ftic_entry_observer != NULL)
	ftic_entry_observer->set_primary();
    
    return (XORP_OK);
}

int
FtiConfig::register_ftic_table_get_primary(FtiConfigTableGet *ftic_table_get)
{
    _ftic_table_get_primary = ftic_table_get;
    if (ftic_table_get != NULL)
	ftic_table_get->set_primary();
    
    return (XORP_OK);
}

int
FtiConfig::register_ftic_table_set_primary(FtiConfigTableSet *ftic_table_set)
{
    _ftic_table_set_primary = ftic_table_set;
    if (ftic_table_set != NULL)
	ftic_table_set->set_primary();
    
    return (XORP_OK);
}

int
FtiConfig::register_ftic_table_observer_primary(FtiConfigTableObserver *ftic_table_observer)
{
    _ftic_table_observer_primary = ftic_table_observer;
    if (ftic_table_observer != NULL)
	ftic_table_observer->set_primary();
    
    return (XORP_OK);
}

int
FtiConfig::register_ftic_entry_get_secondary(FtiConfigEntryGet *ftic_entry_get)
{
    if (ftic_entry_get != NULL) {
	_ftic_entry_gets_secondary.push_back(ftic_entry_get);
	ftic_entry_get->set_secondary();
    }
    
    return (XORP_OK);
}

int
FtiConfig::register_ftic_entry_set_secondary(FtiConfigEntrySet *ftic_entry_set)
{
    if (ftic_entry_set != NULL) {
	_ftic_entry_sets_secondary.push_back(ftic_entry_set);
	ftic_entry_set->set_secondary();
    }
    
    return (XORP_OK);
}

int
FtiConfig::register_ftic_entry_observer_secondary(FtiConfigEntryObserver *ftic_entry_observer)
{
    if (ftic_entry_observer != NULL) {
	_ftic_entry_observers_secondary.push_back(ftic_entry_observer);
	ftic_entry_observer->set_secondary();
    }
    
    return (XORP_OK);
}

int
FtiConfig::register_ftic_table_get_secondary(FtiConfigTableGet *ftic_table_get)
{
    if (ftic_table_get != NULL) {
	_ftic_table_gets_secondary.push_back(ftic_table_get);
	ftic_table_get->set_secondary();
    }
    
    return (XORP_OK);
}

int
FtiConfig::register_ftic_table_set_secondary(FtiConfigTableSet *ftic_table_set)
{
    if (ftic_table_set != NULL) {
	_ftic_table_sets_secondary.push_back(ftic_table_set);
	ftic_table_set->set_secondary();
    }
    
    return (XORP_OK);
}

int
FtiConfig::register_ftic_table_observer_secondary(FtiConfigTableObserver *ftic_table_observer)
{
    if (ftic_table_observer != NULL) {
	_ftic_table_observers_secondary.push_back(ftic_table_observer);
	ftic_table_observer->set_secondary();
    }
    
    return (XORP_OK);
}

int
FtiConfig::set_dummy()
{
    register_ftic_entry_get_primary(&_ftic_entry_get_dummy);
    register_ftic_entry_set_primary(&_ftic_entry_set_dummy);
    register_ftic_entry_observer_primary(&_ftic_entry_observer_dummy);
    register_ftic_table_get_primary(&_ftic_table_get_dummy);
    register_ftic_table_set_primary(&_ftic_table_set_dummy);
    register_ftic_table_observer_primary(&_ftic_table_observer_dummy);

    //
    // XXX: if we are dummy FEA, then we always have IPv4 and IPv6
    //
    _have_ipv4 = true;
    _have_ipv6 = true;

    _is_dummy = true;

    return (XORP_OK);
}

int
FtiConfig::start(string& error_msg)
{
    list<FtiConfigEntryGet*>::iterator ftic_entry_get_iter;
    list<FtiConfigEntrySet*>::iterator ftic_entry_set_iter;
    list<FtiConfigEntryObserver*>::iterator ftic_entry_observer_iter;
    list<FtiConfigTableGet*>::iterator ftic_table_get_iter;
    list<FtiConfigTableSet*>::iterator ftic_table_set_iter;
    list<FtiConfigTableObserver*>::iterator ftic_table_observer_iter;

    if (_is_running)
	return (XORP_OK);

    //
    // Start the FtiConfigEntryGet methods
    //
    if (_ftic_entry_get_primary != NULL) {
	if (_ftic_entry_get_primary->start(error_msg) < 0)
	    return (XORP_ERROR);
    }
    for (ftic_entry_get_iter = _ftic_entry_gets_secondary.begin();
	 ftic_entry_get_iter != _ftic_entry_gets_secondary.end();
	 ++ftic_entry_get_iter) {
	FtiConfigEntryGet* ftic_entry_get = *ftic_entry_get_iter;
	if (ftic_entry_get->start(error_msg) < 0)
	    return (XORP_ERROR);
    }

    //
    // Start the FtiConfigEntrySet methods
    //
    if (_ftic_entry_set_primary != NULL) {
	if (_ftic_entry_set_primary->start(error_msg) < 0)
	    return (XORP_ERROR);
    }
    for (ftic_entry_set_iter = _ftic_entry_sets_secondary.begin();
	 ftic_entry_set_iter != _ftic_entry_sets_secondary.end();
	 ++ftic_entry_set_iter) {
	FtiConfigEntrySet* ftic_entry_set = *ftic_entry_set_iter;
	if (ftic_entry_set->start(error_msg) < 0)
	    return (XORP_ERROR);
    }

    //
    // Start the FtiConfigEntryObserver methods
    //
    if (_ftic_entry_observer_primary != NULL) {
	if (_ftic_entry_observer_primary->start(error_msg) < 0)
	    return (XORP_ERROR);
    }
    for (ftic_entry_observer_iter = _ftic_entry_observers_secondary.begin();
	 ftic_entry_observer_iter != _ftic_entry_observers_secondary.end();
	 ++ftic_entry_observer_iter) {
	FtiConfigEntryObserver* ftic_entry_observer = *ftic_entry_observer_iter;
	if (ftic_entry_observer->start(error_msg) < 0)
	    return (XORP_ERROR);
    }

    //
    // Start the FtiConfigTableGet methods
    //
    if (_ftic_table_get_primary != NULL) {
	if (_ftic_table_get_primary->start(error_msg) < 0)
	    return (XORP_ERROR);
    }
    for (ftic_table_get_iter = _ftic_table_gets_secondary.begin();
	 ftic_table_get_iter != _ftic_table_gets_secondary.end();
	 ++ftic_table_get_iter) {
	FtiConfigTableGet* ftic_table_get = *ftic_table_get_iter;
	if (ftic_table_get->start(error_msg) < 0)
	    return (XORP_ERROR);
    }

    //
    // Start the FtiConfigTableSet methods
    //
    if (_ftic_table_set_primary != NULL) {
	if (_ftic_table_set_primary->start(error_msg) < 0)
	    return (XORP_ERROR);
    }
    for (ftic_table_set_iter = _ftic_table_sets_secondary.begin();
	 ftic_table_set_iter != _ftic_table_sets_secondary.end();
	 ++ftic_table_set_iter) {
	FtiConfigTableSet* ftic_table_set = *ftic_table_set_iter;
	if (ftic_table_set->start(error_msg) < 0)
	    return (XORP_ERROR);
    }

    //
    // Start the FtiConfigTableObserver methods
    //
    if (_ftic_table_observer_primary != NULL) {
	if (_ftic_table_observer_primary->start(error_msg) < 0)
	    return (XORP_ERROR);
    }
    for (ftic_table_observer_iter = _ftic_table_observers_secondary.begin();
	 ftic_table_observer_iter != _ftic_table_observers_secondary.end();
	 ++ftic_table_observer_iter) {
	FtiConfigTableObserver* ftic_table_observer = *ftic_table_observer_iter;
	if (ftic_table_observer->start(error_msg) < 0)
	    return (XORP_ERROR);
    }

    _is_running = true;

    return (XORP_OK);
}

int
FtiConfig::stop(string& error_msg)
{
    list<FtiConfigEntryGet*>::iterator ftic_entry_get_iter;
    list<FtiConfigEntrySet*>::iterator ftic_entry_set_iter;
    list<FtiConfigEntryObserver*>::iterator ftic_entry_observer_iter;
    list<FtiConfigTableGet*>::iterator ftic_table_get_iter;
    list<FtiConfigTableSet*>::iterator ftic_table_set_iter;
    list<FtiConfigTableObserver*>::iterator ftic_table_observer_iter;
    int ret_value = XORP_OK;
    string error_msg2;

    if (! _is_running)
	return (XORP_OK);

    error_msg.erase();

    //
    // Stop the FtiConfigTableObserver methods
    //
    for (ftic_table_observer_iter = _ftic_table_observers_secondary.begin();
	 ftic_table_observer_iter != _ftic_table_observers_secondary.end();
	 ++ftic_table_observer_iter) {
	FtiConfigTableObserver* ftic_table_observer = *ftic_table_observer_iter;
	if (ftic_table_observer->stop(error_msg2) < 0) {
	    ret_value = XORP_ERROR;
	    if (error_msg.empty())
		error_msg = error_msg2;
	}
    }
    if (_ftic_table_observer_primary != NULL) {
	if (_ftic_table_observer_primary->stop(error_msg2) < 0) {
	    ret_value = XORP_ERROR;
	    if (error_msg.empty())
		error_msg = error_msg2;
	}
    }

    //
    // Stop the FtiConfigTableSet methods
    //
    for (ftic_table_set_iter = _ftic_table_sets_secondary.begin();
	 ftic_table_set_iter != _ftic_table_sets_secondary.end();
	 ++ftic_table_set_iter) {
	FtiConfigTableSet* ftic_table_set = *ftic_table_set_iter;
	if (ftic_table_set->stop(error_msg2) < 0) {
	    ret_value = XORP_ERROR;
	    if (error_msg.empty())
		error_msg = error_msg2;
	}
    }
    if (_ftic_table_set_primary != NULL) {
	if (_ftic_table_set_primary->stop(error_msg2) < 0) {
	    ret_value = XORP_ERROR;
	    if (error_msg.empty())
		error_msg = error_msg2;
	}
    }

    //
    // Stop the FtiConfigTableGet methods
    //
    for (ftic_table_get_iter = _ftic_table_gets_secondary.begin();
	 ftic_table_get_iter != _ftic_table_gets_secondary.end();
	 ++ftic_table_get_iter) {
	FtiConfigTableGet* ftic_table_get = *ftic_table_get_iter;
	if (ftic_table_get->stop(error_msg2) < 0) {
	    ret_value = XORP_ERROR;
	    if (error_msg.empty())
		error_msg = error_msg2;
	}
    }
    if (_ftic_table_get_primary != NULL) {
	if (_ftic_table_get_primary->stop(error_msg2) < 0) {
	    ret_value = XORP_ERROR;
	    if (error_msg.empty())
		error_msg = error_msg2;
	}
    }

    //
    // Stop the FtiConfigEntryObserver methods
    //
    for (ftic_entry_observer_iter = _ftic_entry_observers_secondary.begin();
	 ftic_entry_observer_iter != _ftic_entry_observers_secondary.end();
	 ++ftic_entry_observer_iter) {
	FtiConfigEntryObserver* ftic_entry_observer = *ftic_entry_observer_iter;
	if (ftic_entry_observer->stop(error_msg2) < 0) {
	    ret_value = XORP_ERROR;
	    if (error_msg.empty())
		error_msg = error_msg2;
	}
    }
    if (_ftic_entry_observer_primary != NULL) {
	if (_ftic_entry_observer_primary->stop(error_msg2) < 0) {
	    ret_value = XORP_ERROR;
	    if (error_msg.empty())
		error_msg = error_msg2;
	}
    }

    //
    // Stop the FtiConfigEntrySet methods
    //
    for (ftic_entry_set_iter = _ftic_entry_sets_secondary.begin();
	 ftic_entry_set_iter != _ftic_entry_sets_secondary.end();
	 ++ftic_entry_set_iter) {
	FtiConfigEntrySet* ftic_entry_set = *ftic_entry_set_iter;
	if (ftic_entry_set->stop(error_msg2) < 0) {
	    ret_value = XORP_ERROR;
	    if (error_msg.empty())
		error_msg = error_msg2;
	}
    }
    if (_ftic_entry_set_primary != NULL) {
	if (_ftic_entry_set_primary->stop(error_msg2) < 0) {
	    ret_value = XORP_ERROR;
	    if (error_msg.empty())
		error_msg = error_msg2;
	}
    }

    //
    // Stop the FtiConfigEntryGet methods
    //
    for (ftic_entry_get_iter = _ftic_entry_gets_secondary.begin();
	 ftic_entry_get_iter != _ftic_entry_gets_secondary.end();
	 ++ftic_entry_get_iter) {
	FtiConfigEntryGet* ftic_entry_get = *ftic_entry_get_iter;
	if (ftic_entry_get->stop(error_msg2) < 0) {
	    ret_value = XORP_ERROR;
	    if (error_msg.empty())
		error_msg = error_msg2;
	}
    }
    if (_ftic_entry_get_primary != NULL) {
	if (_ftic_entry_get_primary->stop(error_msg2) < 0) {
	    ret_value = XORP_ERROR;
	    if (error_msg.empty())
		error_msg = error_msg2;
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

/**
 * Enable/disable Click support.
 *
 * @param enable if true, then enable Click support, otherwise disable it.
 */
void
FtiConfig::enable_click(bool enable)
{
    _ftic_entry_get_click.enable_click(enable);
    _ftic_entry_set_click.enable_click(enable);
    _ftic_table_get_click.enable_click(enable);
    _ftic_table_set_click.enable_click(enable);
}

/**
 * Start Click support.
 *
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
FtiConfig::start_click(string& error_msg)
{
    if (_ftic_entry_get_click.start(error_msg) < 0) {
	return (XORP_ERROR);
    }
    if (_ftic_entry_set_click.start(error_msg) < 0) {
	string error_msg2;
	_ftic_entry_get_click.stop(error_msg2);
	return (XORP_ERROR);
    }
    if (_ftic_table_get_click.start(error_msg) < 0) {
	string error_msg2;
	_ftic_entry_get_click.stop(error_msg2);
	_ftic_entry_set_click.stop(error_msg2);
	return (XORP_ERROR);
    }
    if (_ftic_table_set_click.start(error_msg) < 0) {
	string error_msg2;
	_ftic_entry_get_click.stop(error_msg2);
	_ftic_entry_set_click.stop(error_msg2);
	_ftic_table_get_click.stop(error_msg2);
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

/**
 * Stop Click support.
 *
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
FtiConfig::stop_click(string& error_msg)
{
    if (_ftic_entry_get_click.stop(error_msg) < 0) {
	string error_msg2;
	_ftic_entry_set_click.stop(error_msg2);
	_ftic_table_get_click.stop(error_msg2);
	_ftic_table_set_click.stop(error_msg2);
	return (XORP_ERROR);
    }
    if (_ftic_entry_set_click.stop(error_msg) < 0) {
	string error_msg2;
	_ftic_table_get_click.stop(error_msg2);
	_ftic_table_set_click.stop(error_msg2);
	return (XORP_ERROR);
    }
    if (_ftic_table_get_click.stop(error_msg) < 0) {
	string error_msg2;
	_ftic_table_set_click.stop(error_msg2);
	return (XORP_ERROR);
    }
    if (_ftic_table_set_click.stop(error_msg) < 0) {
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

/**
 * Enable/disable duplicating the Click routes to the system kernel.
 *
 * @param enable if true, then enable duplicating the Click routes to the
 * system kernel, otherwise disable it.
 */
void
FtiConfig::enable_duplicate_routes_to_kernel(bool enable)
{
    _ftic_entry_get_click.enable_duplicate_routes_to_kernel(enable);
    _ftic_entry_set_click.enable_duplicate_routes_to_kernel(enable);
    _ftic_table_get_click.enable_duplicate_routes_to_kernel(enable);
    _ftic_table_set_click.enable_duplicate_routes_to_kernel(enable);
}

/**
 * Enable/disable kernel-level Click support.
 *
 * @param enable if true, then enable the kernel-level Click support,
 * otherwise disable it.
 */
void
FtiConfig::enable_kernel_click(bool enable)
{
    _ftic_entry_get_click.enable_kernel_click(enable);
    _ftic_entry_set_click.enable_kernel_click(enable);
    _ftic_table_get_click.enable_kernel_click(enable);
    _ftic_table_set_click.enable_kernel_click(enable);
}

/**
 * Enable/disable installing kernel-level Click on startup.
 *
 * @param enable if true, then install kernel-level Click on startup.
 */
void
FtiConfig::enable_kernel_click_install_on_startup(bool enable)
{
    // XXX: only IfConfigGet should install the kernel-level Click
    _ftic_entry_get_click.enable_kernel_click_install_on_startup(enable);
    _ftic_entry_set_click.enable_kernel_click_install_on_startup(false);
    _ftic_table_get_click.enable_kernel_click_install_on_startup(false);
    _ftic_table_set_click.enable_kernel_click_install_on_startup(false);
}

/**
 * Specify the list of kernel Click modules to load on startup if
 * installing kernel-level Click on startup is enabled.
 *
 * @param modules the list of kernel Click modules to load.
 */
void
FtiConfig::set_kernel_click_modules(const list<string>& modules)
{
    _ftic_entry_get_click.set_kernel_click_modules(modules);
    _ftic_entry_set_click.set_kernel_click_modules(modules);
    _ftic_table_get_click.set_kernel_click_modules(modules);
    _ftic_table_set_click.set_kernel_click_modules(modules);
}

/**
 * Specify the kernel-level Click mount directory.
 *
 * @param directory the kernel-level Click mount directory.
 */
void
FtiConfig::set_kernel_click_mount_directory(const string& directory)
{
    _ftic_entry_get_click.set_kernel_click_mount_directory(directory);
    _ftic_entry_set_click.set_kernel_click_mount_directory(directory);
    _ftic_table_get_click.set_kernel_click_mount_directory(directory);
    _ftic_table_set_click.set_kernel_click_mount_directory(directory);
}

/**
 * Specify the external program to generate the kernel-level Click
 * configuration.
 *
 * @param v the name of the external program to generate the kernel-level
 * Click configuration.
 */
void
FtiConfig::set_kernel_click_config_generator_file(const string& v)
{
    _ftic_entry_get_click.set_kernel_click_config_generator_file(v);
    _ftic_entry_set_click.set_kernel_click_config_generator_file(v);
    _ftic_table_get_click.set_kernel_click_config_generator_file(v);
    _ftic_table_set_click.set_kernel_click_config_generator_file(v);
}

/**
 * Enable/disable user-level Click support.
 *
 * @param enable if true, then enable the user-level Click support,
 * otherwise disable it.
 */
void
FtiConfig::enable_user_click(bool enable)
{
    _ftic_entry_get_click.enable_user_click(enable);
    _ftic_entry_set_click.enable_user_click(enable);
    _ftic_table_get_click.enable_user_click(enable);
    _ftic_table_set_click.enable_user_click(enable);
}

/**
 * Specify the user-level Click command file.
 *
 * @param v the name of the user-level Click command file.
 */
void
FtiConfig::set_user_click_command_file(const string& v)
{
    _ftic_entry_get_click.set_user_click_command_file(v);
    _ftic_entry_set_click.set_user_click_command_file(v);
    _ftic_table_get_click.set_user_click_command_file(v);
    _ftic_table_set_click.set_user_click_command_file(v);
}

/**
 * Specify the extra arguments to the user-level Click command.
 *
 * @param v the extra arguments to the user-level Click command.
 */
void
FtiConfig::set_user_click_command_extra_arguments(const string& v)
{
    _ftic_entry_get_click.set_user_click_command_extra_arguments(v);
    _ftic_entry_set_click.set_user_click_command_extra_arguments(v);
    _ftic_table_get_click.set_user_click_command_extra_arguments(v);
    _ftic_table_set_click.set_user_click_command_extra_arguments(v);
}

/**
 * Specify whether to execute on startup the user-level Click command.
 *
 * @param v if true, then execute the user-level Click command on startup.
 */
void
FtiConfig::set_user_click_command_execute_on_startup(bool v)
{
    UNUSED(v);

    // XXX: only IfConfigGet should execute the user-level Click command
    _ftic_entry_get_click.set_user_click_command_execute_on_startup(false);
    _ftic_entry_set_click.set_user_click_command_execute_on_startup(false);
    _ftic_table_get_click.set_user_click_command_execute_on_startup(false);
    _ftic_table_set_click.set_user_click_command_execute_on_startup(false);
}

/**
 * Specify the address to use for control access to the user-level
 * Click.
 *
 * @param v the address to use for control access to the user-level Click.
 */
void
FtiConfig::set_user_click_control_address(const IPv4& v)
{
    _ftic_entry_get_click.set_user_click_control_address(v);
    _ftic_entry_set_click.set_user_click_control_address(v);
    _ftic_table_get_click.set_user_click_control_address(v);
    _ftic_table_set_click.set_user_click_control_address(v);
}

/**
 * Specify the socket port to use for control access to the user-level
 * Click.
 *
 * @param v the socket port to use for control access to the user-level
 * Click.
 */
void
FtiConfig::set_user_click_control_socket_port(uint32_t v)
{
    _ftic_entry_get_click.set_user_click_control_socket_port(v);
    _ftic_entry_set_click.set_user_click_control_socket_port(v);
    _ftic_table_get_click.set_user_click_control_socket_port(v);
    _ftic_table_set_click.set_user_click_control_socket_port(v);
}

/**
 * Specify the configuration file to be used by user-level Click on
 * startup.
 *
 * @param v the name of the configuration file to be used by user-level
 * Click on startup.
 */
void
FtiConfig::set_user_click_startup_config_file(const string& v)
{
    _ftic_entry_get_click.set_user_click_startup_config_file(v);
    _ftic_entry_set_click.set_user_click_startup_config_file(v);
    _ftic_table_get_click.set_user_click_startup_config_file(v);
    _ftic_table_set_click.set_user_click_startup_config_file(v);
}

/**
 * Specify the external program to generate the user-level Click
 * configuration.
 *
 * @param v the name of the external program to generate the user-level
 * Click configuration.
 */
void
FtiConfig::set_user_click_config_generator_file(const string& v)
{
    _ftic_entry_get_click.set_user_click_config_generator_file(v);
    _ftic_entry_set_click.set_user_click_config_generator_file(v);
    _ftic_table_get_click.set_user_click_config_generator_file(v);
    _ftic_table_set_click.set_user_click_config_generator_file(v);
}

bool
FtiConfig::start_configuration(string& error_msg)
{
    list<FtiConfigEntrySet*>::iterator ftic_entry_set_iter;
    list<FtiConfigTableSet*>::iterator ftic_table_set_iter;
    bool ret_value = true;
    string error_msg2;

    error_msg.erase();

    //
    // XXX: we need to call start_configuration() for "entry" and "table",
    // because the top-level start/end configuration interface
    // does not distinguish between "entry" and "table" modification.
    //
    if (_ftic_entry_set_primary != NULL) {
	if (_ftic_entry_set_primary->start_configuration(error_msg2) != true) {
	    ret_value = false;
	    if (error_msg.empty())
		error_msg = error_msg2;
	}
    }
    for (ftic_entry_set_iter = _ftic_entry_sets_secondary.begin();
	 ftic_entry_set_iter != _ftic_entry_sets_secondary.end();
	 ++ftic_entry_set_iter) {
	FtiConfigEntrySet* ftic_entry_set = *ftic_entry_set_iter;
	if (ftic_entry_set->start_configuration(error_msg2) != true) {
	    ret_value = false;
	    if (error_msg.empty())
		error_msg = error_msg2;
	}
    }

    if (_ftic_table_set_primary != NULL) {
	if (_ftic_table_set_primary->start_configuration(error_msg2) != true) {
	    ret_value = false;
	    if (error_msg.empty())
		error_msg = error_msg2;
	}
    }
    for (ftic_table_set_iter = _ftic_table_sets_secondary.begin();
	 ftic_table_set_iter != _ftic_table_sets_secondary.end();
	 ++ftic_table_set_iter) {
	FtiConfigTableSet* ftic_table_set = *ftic_table_set_iter;
	if (ftic_table_set->start_configuration(error_msg2) != true) {
	    ret_value = false;
	    if (error_msg.empty())
		error_msg = error_msg2;
	}
    }
    
    return (ret_value);
}

bool
FtiConfig::end_configuration(string& error_msg)
{
    list<FtiConfigEntrySet*>::iterator ftic_entry_set_iter;
    list<FtiConfigTableSet*>::iterator ftic_table_set_iter;
    bool ret_value = true;
    string error_msg2;

    error_msg.erase();

    //
    // XXX: we need to call end_configuration() for "entry" and "table",
    // because the top-level start/end configuration interface
    // does not distinguish between "entry" and "table" modification.
    //
    if (_ftic_entry_set_primary != NULL) {
	if (_ftic_entry_set_primary->end_configuration(error_msg) != true) {
	    ret_value = false;
	    if (error_msg.empty())
		error_msg = error_msg2;
	}
    }
    for (ftic_entry_set_iter = _ftic_entry_sets_secondary.begin();
	 ftic_entry_set_iter != _ftic_entry_sets_secondary.end();
	 ++ftic_entry_set_iter) {
	FtiConfigEntrySet* ftic_entry_set = *ftic_entry_set_iter;
	if (ftic_entry_set->end_configuration(error_msg) != true) {
	    ret_value = false;
	    if (error_msg.empty())
		error_msg = error_msg2;
	}
    }

    if (_ftic_table_set_primary != NULL) {
	if (_ftic_table_set_primary->end_configuration(error_msg) != true) {
	    ret_value = false;
	    if (error_msg.empty())
		error_msg = error_msg2;
	}
    }
    for (ftic_table_set_iter = _ftic_table_sets_secondary.begin();
	 ftic_table_set_iter != _ftic_table_sets_secondary.end();
	 ++ftic_table_set_iter) {
	FtiConfigTableSet* ftic_table_set = *ftic_table_set_iter;
	if (ftic_table_set->end_configuration(error_msg) != true) {
	    ret_value = false;
	    if (error_msg.empty())
		error_msg = error_msg2;
	}
    }
    
    return (ret_value);
}

bool
FtiConfig::add_entry4(const Fte4& fte)
{
    list<FtiConfigEntrySet*>::iterator ftic_entry_set_iter;

    if ((_ftic_entry_set_primary == NULL) && _ftic_entry_sets_secondary.empty())
	return (false);

    if (_profile.enabled(profile_route_out))
	_profile.log(profile_route_out,
		     c_format("add %s", fte.net().str().c_str()));

    if (_ftic_entry_set_primary != NULL) {
	if (_ftic_entry_set_primary->add_entry4(fte) != true)
	    return (false);
    }
    for (ftic_entry_set_iter = _ftic_entry_sets_secondary.begin();
	 ftic_entry_set_iter != _ftic_entry_sets_secondary.end();
	 ++ftic_entry_set_iter) {
	FtiConfigEntrySet* ftic_entry_set = *ftic_entry_set_iter;
	if (ftic_entry_set->add_entry4(fte) != true)
	    return (false);
    }

    return (true);
}

bool
FtiConfig::delete_entry4(const Fte4& fte)
{
    list<FtiConfigEntrySet*>::iterator ftic_entry_set_iter;

    if ((_ftic_entry_set_primary == NULL) && _ftic_entry_sets_secondary.empty())
	return (false);

    if (_profile.enabled(profile_route_out))
	_profile.log(profile_route_out,
		     c_format("delete %s", fte.net().str().c_str()));

    if (_ftic_entry_set_primary != NULL) {
	if (_ftic_entry_set_primary->delete_entry4(fte) != true)
	    return (false);
    }
    for (ftic_entry_set_iter = _ftic_entry_sets_secondary.begin();
	 ftic_entry_set_iter != _ftic_entry_sets_secondary.end();
	 ++ftic_entry_set_iter) {
	FtiConfigEntrySet* ftic_entry_set = *ftic_entry_set_iter;
	if (ftic_entry_set->delete_entry4(fte) != true)
	    return (false);
    }

    return (true);
}

bool
FtiConfig::set_table4(const list<Fte4>& fte_list)
{
    list<FtiConfigTableSet*>::iterator ftic_table_set_iter;

    if ((_ftic_table_set_primary == NULL) && _ftic_table_sets_secondary.empty())
	return (false);

    if (_ftic_table_set_primary != NULL) {
	if (_ftic_table_set_primary->set_table4(fte_list) != true)
	    return (false);
    }
    for (ftic_table_set_iter = _ftic_table_sets_secondary.begin();
	 ftic_table_set_iter != _ftic_table_sets_secondary.end();
	 ++ftic_table_set_iter) {
	FtiConfigTableSet* ftic_table_set = *ftic_table_set_iter;
	if (ftic_table_set->set_table4(fte_list) != true)
	    return (false);
    }

    return (true);
}

bool
FtiConfig::delete_all_entries4()
{
    list<FtiConfigTableSet*>::iterator ftic_table_set_iter;

    if ((_ftic_table_set_primary == NULL) && _ftic_table_sets_secondary.empty())
	return (false);

    if (_ftic_table_set_primary != NULL) {
	if (_ftic_table_set_primary->delete_all_entries4() != true)
	    return (false);
    }
    for (ftic_table_set_iter = _ftic_table_sets_secondary.begin();
	 ftic_table_set_iter != _ftic_table_sets_secondary.end();
	 ++ftic_table_set_iter) {
	FtiConfigTableSet* ftic_table_set = *ftic_table_set_iter;
	if (ftic_table_set->delete_all_entries4() != true)
	    return (false);
    }

    return (true);
}

bool
FtiConfig::lookup_route_by_dest4(const IPv4& dst, Fte4& fte)
{
    if (_ftic_entry_get_primary == NULL)
	return (false);

    if (_ftic_entry_get_primary->lookup_route_by_dest4(dst, fte) != true)
	return (false);

    return (true);
}

bool
FtiConfig::lookup_route_by_network4(const IPv4Net& dst, Fte4& fte)
{
    if (_ftic_entry_get_primary == NULL)
	return (false);

    if (_ftic_entry_get_primary->lookup_route_by_network4(dst, fte) != true)
	return (false);

    return (true);
}

bool
FtiConfig::get_table4(list<Fte4>& fte_list)
{
    if (_ftic_table_get_primary == NULL)
	return (false);

    if (_ftic_table_get_primary->get_table4(fte_list) != true)
	return (false);

    return (true);
}

bool
FtiConfig::add_entry6(const Fte6& fte)
{
    list<FtiConfigEntrySet*>::iterator ftic_entry_set_iter;

    if ((_ftic_entry_set_primary == NULL) && _ftic_entry_sets_secondary.empty())
	return (false);

    if (_profile.enabled(profile_route_out))
	_profile.log(profile_route_out,
		     c_format("add %s", fte.net().str().c_str()));

    if (_ftic_entry_set_primary != NULL) {
	if (_ftic_entry_set_primary->add_entry6(fte) != true)
	    return (false);
    }
    for (ftic_entry_set_iter = _ftic_entry_sets_secondary.begin();
	 ftic_entry_set_iter != _ftic_entry_sets_secondary.end();
	 ++ftic_entry_set_iter) {
	FtiConfigEntrySet* ftic_entry_set = *ftic_entry_set_iter;
	if (ftic_entry_set->add_entry6(fte) != true)
	    return (false);
    }

    return (true);
}

bool
FtiConfig::delete_entry6(const Fte6& fte)
{
    list<FtiConfigEntrySet*>::iterator ftic_entry_set_iter;

    if ((_ftic_entry_set_primary == NULL) && _ftic_entry_sets_secondary.empty())
	return (false);

    if (_profile.enabled(profile_route_out))
	_profile.log(profile_route_out,
		     c_format("delete %s", fte.net().str().c_str()));

    if (_ftic_entry_set_primary != NULL) {
	if (_ftic_entry_set_primary->delete_entry6(fte) != true)
	    return (false);
    }
    for (ftic_entry_set_iter = _ftic_entry_sets_secondary.begin();
	 ftic_entry_set_iter != _ftic_entry_sets_secondary.end();
	 ++ftic_entry_set_iter) {
	FtiConfigEntrySet* ftic_entry_set = *ftic_entry_set_iter;
	if (ftic_entry_set->delete_entry6(fte) != true)
	    return (false);
    }

    return (true);
}

bool
FtiConfig::set_table6(const list<Fte6>& fte_list)
{
    list<FtiConfigTableSet*>::iterator ftic_table_set_iter;

    if ((_ftic_table_set_primary == NULL) && _ftic_table_sets_secondary.empty())
	return (false);

    if (_ftic_table_set_primary != NULL) {
	if (_ftic_table_set_primary->set_table6(fte_list) != true)
	    return (false);
    }
    for (ftic_table_set_iter = _ftic_table_sets_secondary.begin();
	 ftic_table_set_iter != _ftic_table_sets_secondary.end();
	 ++ftic_table_set_iter) {
	FtiConfigTableSet* ftic_table_set = *ftic_table_set_iter;
	if (ftic_table_set->set_table6(fte_list) != true)
	    return (false);
    }

    return (true);
}

bool
FtiConfig::delete_all_entries6()
{
    list<FtiConfigTableSet*>::iterator ftic_table_set_iter;

    if ((_ftic_table_set_primary == NULL) && _ftic_table_sets_secondary.empty())
	return (false);

    if (_ftic_table_set_primary != NULL) {
	if (_ftic_table_set_primary->delete_all_entries6() != true)
	    return (false);
    }
    for (ftic_table_set_iter = _ftic_table_sets_secondary.begin();
	 ftic_table_set_iter != _ftic_table_sets_secondary.end();
	 ++ftic_table_set_iter) {
	FtiConfigTableSet* ftic_table_set = *ftic_table_set_iter;
	if (ftic_table_set->delete_all_entries6() != true)
	    return (false);
    }

    return (true);
}

bool
FtiConfig::lookup_route_by_dest6(const IPv6& dst, Fte6& fte)
{
    if (_ftic_entry_get_primary == NULL)
	return (false);

    if (_ftic_entry_get_primary->lookup_route_by_dest6(dst, fte) != true)
	return (false);

    return (true);
}

bool
FtiConfig::lookup_route_by_network6(const IPv6Net& dst, Fte6& fte)
{
    if (_ftic_entry_get_primary == NULL)
	return (false);

    if (_ftic_entry_get_primary->lookup_route_by_network6(dst, fte) != true)
	return (false);

    return (true);
}

bool
FtiConfig::get_table6(list<Fte6>& fte_list)
{
    if (_ftic_table_get_primary == NULL)
	return (false);

    if (_ftic_table_get_primary->get_table6(fte_list) != true)
	return (false);

    return (true);
}

bool
FtiConfig::add_fib_table_observer(FibTableObserverBase* fib_table_observer)
{
    if (_ftic_table_observer_primary == NULL)
	return (false);

    _ftic_table_observer_primary->add_fib_table_observer(fib_table_observer);

    return (true);
}

bool
FtiConfig::delete_fib_table_observer(FibTableObserverBase* fib_table_observer)
{
    if (_ftic_table_observer_primary == NULL)
	return (false);

    _ftic_table_observer_primary->delete_fib_table_observer(fib_table_observer);

    return (true);
}

/**
 * Test if the underlying system supports IPv4.
 * 
 * @return true if the underlying system supports IPv4, otherwise false.
 */
bool
FtiConfig::test_have_ipv4() const
{
    // XXX: always return true if running in dummy mode
    if (is_dummy())
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
FtiConfig::test_have_ipv6() const
{
    // XXX: always return true if running in dummy mode
    if (is_dummy())
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
FtiConfig::unicast_forwarding_enabled4(bool& ret_value, string& error_msg) const
{
    // XXX: always return true if running in dummy mode
    if (is_dummy()) {
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
FtiConfig::unicast_forwarding_enabled6(bool& ret_value, string& error_msg) const
{
    // XXX: always return true if running in dummy mode
    if (is_dummy()) {
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
FtiConfig::accept_rtadv_enabled6(bool& ret_value, string& error_msg) const
{
    // XXX: always return true if running in dummy mode
    if (is_dummy()) {
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
FtiConfig::set_unicast_forwarding_enabled4(bool v, string& error_msg)
{
    // XXX: don't do anything if running in dummy mode
    if (is_dummy())
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
			     "IPv4 is not supported", (v) ? "true": "false");
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
				 (v) ? "true" : "false", strerror(errno));
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
				 (v) ? "true": "false", strerror(errno));
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
FtiConfig::set_unicast_forwarding_enabled6(bool v, string& error_msg)
{
    // XXX: don't do anything if running in dummy mode
    if (is_dummy())
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
			 "IPv6 is not supported", (v) ? "true": "false");
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
			     "IPv6 is not supported", (v) ? "true": "false");
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
				 (v) ? "true" : "false", strerror(errno));
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
				 (v) ? "true": "false", strerror(errno));
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
FtiConfig::set_accept_rtadv_enabled6(bool v, string& error_msg)
{
    // XXX: don't do anything if running in dummy mode
    if (is_dummy())
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
			 (v) ? "true": "false");
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
			     (v) ? "true": "false");
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
				 (v) ? "true" : "false", strerror(errno));
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
				 (v) ? "true": "false", strerror(errno));
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
FtiConfig::set_unicast_forwarding_entries_retain_on_startup4(bool retain,
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
FtiConfig::set_unicast_forwarding_entries_retain_on_shutdown4(bool retain,
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
FtiConfig::set_unicast_forwarding_entries_retain_on_startup6(bool retain,
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
FtiConfig::set_unicast_forwarding_entries_retain_on_shutdown6(bool retain,
							      string& error_msg)
{
    _unicast_forwarding_entries_retain_on_shutdown6 = retain;

    error_msg = "";		// XXX: reset
    return (XORP_OK);
}
