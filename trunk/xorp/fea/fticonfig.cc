// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

#ident "$XORP: xorp/fea/fticonfig.cc,v 1.25 2004/10/02 19:49:50 atanu Exp $"


#include "fea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/profile.hh"

#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif

#include "profile_vars.hh"
#include "fticonfig.hh"

#define PROC_LINUX_FILE_FORWARDING_V4 "/proc/sys/net/ipv4/ip_forward"
#define PROC_LINUX_FILE_FORWARDING_V6 "/proc/sys/net/ipv6/conf/all/forwarding"

//
// Unicast forwarding table related configuration.
//


FtiConfig::FtiConfig(EventLoop& eventloop, Profile& profile)
    : _eventloop(eventloop),
      _profile(profile),
      _ftic_entry_get_dummy(*this),
      _ftic_entry_get_rtsock(*this),
      _ftic_entry_get_netlink(*this),
      _ftic_entry_set_dummy(*this),
      _ftic_entry_set_rtsock(*this),
      _ftic_entry_set_netlink(*this),
      _ftic_entry_observer_dummy(*this),
      _ftic_entry_observer_rtsock(*this),
      _ftic_entry_observer_netlink(*this),
      _ftic_table_get_dummy(*this),
      _ftic_table_get_sysctl(*this),
      _ftic_table_get_netlink(*this),
      _ftic_table_set_dummy(*this),
      _ftic_table_set_rtsock(*this),
      _ftic_table_set_netlink(*this),
      _ftic_table_observer_dummy(*this),
      _ftic_table_observer_rtsock(*this),
      _ftic_table_observer_netlink(*this),
      _unicast_forwarding_enabled4(false),
      _unicast_forwarding_enabled6(false),
      _accept_rtadv_enabled6(false),
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
    if (_ftic_entry_gets.empty()) {
	XLOG_FATAL("No mechanism to get forwarding table entries "
		   "from the underlying system");
    }
    if (_ftic_entry_sets.empty()) {
	XLOG_FATAL("No mechanism to set forwarding table entries "
		   "into the underlying system");
    }
    if (_ftic_entry_observers.empty()) {
	XLOG_FATAL("No mechanism to observe forwarding table entries "
		   "from the underlying system");
    }
    if (_ftic_table_gets.empty()) {
	XLOG_FATAL("No mechanism to get the forwarding table information "
		   "from the underlying system");
    }
    if (_ftic_table_sets.empty()) {
	XLOG_FATAL("No mechanism to set the forwarding table information "
		   "into the underlying system");
    }
    if (_ftic_table_observers.empty()) {
	XLOG_FATAL("No mechanism to observe the forwarding table information "
		   "from the underlying system");
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
	    XLOG_FATAL(error_msg.c_str());
	}
    }
#ifdef HAVE_IPV6
    if (_have_ipv6) {
	if (unicast_forwarding_enabled6(_unicast_forwarding_enabled6,
					error_msg) < 0) {
	    XLOG_FATAL(error_msg.c_str());
	}
	if (accept_rtadv_enabled6(_accept_rtadv_enabled6, error_msg) < 0) {
	    XLOG_FATAL(error_msg.c_str());
	}
    }
#endif // HAVE_IPV6
}

FtiConfig::~FtiConfig()
{
    stop();
}

int
FtiConfig::register_ftic_entry_get_primary(FtiConfigEntryGet *ftic_entry_get)
{
    _ftic_entry_gets.clear();
    if (ftic_entry_get != NULL)
	_ftic_entry_gets.push_back(ftic_entry_get);
    
    return (XORP_OK);
}

int
FtiConfig::register_ftic_entry_set_primary(FtiConfigEntrySet *ftic_entry_set)
{
    _ftic_entry_sets.clear();
    if (ftic_entry_set != NULL)
	_ftic_entry_sets.push_back(ftic_entry_set);
    
    return (XORP_OK);
}

int
FtiConfig::register_ftic_entry_observer_primary(FtiConfigEntryObserver *ftic_entry_observer)
{
    _ftic_entry_observers.clear();
    if (ftic_entry_observer != NULL)
	_ftic_entry_observers.push_back(ftic_entry_observer);
    
    return (XORP_OK);
}

int
FtiConfig::register_ftic_table_get_primary(FtiConfigTableGet *ftic_table_get)
{
    _ftic_table_gets.clear();
    if (ftic_table_get != NULL)
	_ftic_table_gets.push_back(ftic_table_get);
    
    return (XORP_OK);
}

int
FtiConfig::register_ftic_table_set_primary(FtiConfigTableSet *ftic_table_set)
{
    _ftic_table_sets.clear();
    if (ftic_table_set != NULL)
	_ftic_table_sets.push_back(ftic_table_set);
    
    return (XORP_OK);
}

int
FtiConfig::register_ftic_table_observer_primary(FtiConfigTableObserver *ftic_table_observer)
{
    _ftic_table_observers.clear();
    if (ftic_table_observer != NULL)
	_ftic_table_observers.push_back(ftic_table_observer);
    
    return (XORP_OK);
}

int
FtiConfig::register_ftic_entry_get_secondary(FtiConfigEntryGet *ftic_entry_get)
{
    if (ftic_entry_get != NULL)
	_ftic_entry_gets.push_back(ftic_entry_get);
    
    return (XORP_OK);
}

int
FtiConfig::register_ftic_entry_set_secondary(FtiConfigEntrySet *ftic_entry_set)
{
    if (ftic_entry_set != NULL)
	_ftic_entry_sets.push_back(ftic_entry_set);
    
    return (XORP_OK);
}

int
FtiConfig::register_ftic_entry_observer_secondary(FtiConfigEntryObserver *ftic_entry_observer)
{
    if (ftic_entry_observer != NULL)
	_ftic_entry_observers.push_back(ftic_entry_observer);
    
    return (XORP_OK);
}

int
FtiConfig::register_ftic_table_get_secondary(FtiConfigTableGet *ftic_table_get)
{
    if (ftic_table_get != NULL)
	_ftic_table_gets.push_back(ftic_table_get);
    
    return (XORP_OK);
}

int
FtiConfig::register_ftic_table_set_secondary(FtiConfigTableSet *ftic_table_set)
{
    if (ftic_table_set != NULL)
	_ftic_table_sets.push_back(ftic_table_set);
    
    return (XORP_OK);
}

int
FtiConfig::register_ftic_table_observer_secondary(FtiConfigTableObserver *ftic_table_observer)
{
    if (ftic_table_observer != NULL)
	_ftic_table_observers.push_back(ftic_table_observer);
    
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
FtiConfig::start()
{
    list<FtiConfigEntryGet*>::iterator ftic_entry_get_iter;
    list<FtiConfigEntrySet*>::iterator ftic_entry_set_iter;
    list<FtiConfigEntryObserver*>::iterator ftic_entry_observer_iter;
    list<FtiConfigTableGet*>::iterator ftic_table_get_iter;
    list<FtiConfigTableSet*>::iterator ftic_table_set_iter;
    list<FtiConfigTableObserver*>::iterator ftic_table_observer_iter;

    if (_is_running)
	return (XORP_OK);

    for (ftic_entry_get_iter = _ftic_entry_gets.begin();
	 ftic_entry_get_iter != _ftic_entry_gets.end();
	 ++ftic_entry_get_iter) {
	FtiConfigEntryGet* ftic_entry_get = *ftic_entry_get_iter;
	if (ftic_entry_get->start() < 0)
	    return (XORP_ERROR);
    }

    for (ftic_entry_set_iter = _ftic_entry_sets.begin();
	 ftic_entry_set_iter != _ftic_entry_sets.end();
	 ++ftic_entry_set_iter) {
	FtiConfigEntrySet* ftic_entry_set = *ftic_entry_set_iter;
	if (ftic_entry_set->start() < 0)
	    return (XORP_ERROR);
    }

    for (ftic_entry_observer_iter = _ftic_entry_observers.begin();
	 ftic_entry_observer_iter != _ftic_entry_observers.end();
	 ++ftic_entry_observer_iter) {
	FtiConfigEntryObserver* ftic_entry_observer = *ftic_entry_observer_iter;
	if (ftic_entry_observer->start() < 0)
	    return (XORP_ERROR);
    }

    for (ftic_table_get_iter = _ftic_table_gets.begin();
	 ftic_table_get_iter != _ftic_table_gets.end();
	 ++ftic_table_get_iter) {
	FtiConfigTableGet* ftic_table_get = *ftic_table_get_iter;
	if (ftic_table_get->start() < 0)
	    return (XORP_ERROR);
    }

    for (ftic_table_set_iter = _ftic_table_sets.begin();
	 ftic_table_set_iter != _ftic_table_sets.end();
	 ++ftic_table_set_iter) {
	FtiConfigTableSet* ftic_table_set = *ftic_table_set_iter;
	if (ftic_table_set->start() < 0)
	    return (XORP_ERROR);
    }

    for (ftic_table_observer_iter = _ftic_table_observers.begin();
	 ftic_table_observer_iter != _ftic_table_observers.end();
	 ++ftic_table_observer_iter) {
	FtiConfigTableObserver* ftic_table_observer = *ftic_table_observer_iter;
	if (ftic_table_observer->start() < 0)
	    return (XORP_ERROR);
    }

    _is_running = true;

    return (XORP_OK);
}

int
FtiConfig::stop()
{
    list<FtiConfigEntryGet*>::iterator ftic_entry_get_iter;
    list<FtiConfigEntrySet*>::iterator ftic_entry_set_iter;
    list<FtiConfigEntryObserver*>::iterator ftic_entry_observer_iter;
    list<FtiConfigTableGet*>::iterator ftic_table_get_iter;
    list<FtiConfigTableSet*>::iterator ftic_table_set_iter;
    list<FtiConfigTableObserver*>::iterator ftic_table_observer_iter;
    string error_msg;
    int ret_value = XORP_OK;

    if (! _is_running)
	return (XORP_OK);

    for (ftic_table_observer_iter = _ftic_table_observers.begin();
	 ftic_table_observer_iter != _ftic_table_observers.end();
	 ++ftic_table_observer_iter) {
	FtiConfigTableObserver* ftic_table_observer = *ftic_table_observer_iter;
	if (ftic_table_observer->stop() < 0)
	    ret_value = XORP_ERROR;
    }

    for (ftic_table_set_iter = _ftic_table_sets.begin();
	 ftic_table_set_iter != _ftic_table_sets.end();
	 ++ftic_table_set_iter) {
	FtiConfigTableSet* ftic_table_set = *ftic_table_set_iter;
	if (ftic_table_set->stop() < 0)
	    ret_value = XORP_ERROR;
    }

    for (ftic_table_get_iter = _ftic_table_gets.begin();
	 ftic_table_get_iter != _ftic_table_gets.end();
	 ++ftic_table_get_iter) {
	FtiConfigTableGet* ftic_table_get = *ftic_table_get_iter;
	if (ftic_table_get->stop() < 0)
	    ret_value = XORP_ERROR;
    }

    for (ftic_entry_observer_iter = _ftic_entry_observers.begin();
	 ftic_entry_observer_iter != _ftic_entry_observers.end();
	 ++ftic_entry_observer_iter) {
	FtiConfigEntryObserver* ftic_entry_observer = *ftic_entry_observer_iter;
	if (ftic_entry_observer->stop() < 0)
	    ret_value = XORP_ERROR;
    }

    for (ftic_entry_set_iter = _ftic_entry_sets.begin();
	 ftic_entry_set_iter != _ftic_entry_sets.end();
	 ++ftic_entry_set_iter) {
	FtiConfigEntrySet* ftic_entry_set = *ftic_entry_set_iter;
	if (ftic_entry_set->stop() < 0)
	    ret_value = XORP_ERROR;
    }

    for (ftic_entry_get_iter = _ftic_entry_gets.begin();
	 ftic_entry_get_iter != _ftic_entry_gets.end();
	 ++ftic_entry_get_iter) {
	FtiConfigEntryGet* ftic_entry_get = *ftic_entry_get_iter;
	if (ftic_entry_get->stop() < 0)
	    ret_value = XORP_ERROR;
    }

    //
    // Restore the old state in the underlying system
    //
    if (_have_ipv4) {
	if (set_unicast_forwarding_enabled4(_unicast_forwarding_enabled4,
					    error_msg) < 0) {
	    ret_value = XORP_ERROR;
	}
    }
#ifdef HAVE_IPV6
    if (_have_ipv6) {
	if (set_unicast_forwarding_enabled6(_unicast_forwarding_enabled6,
					    error_msg) < 0) {
	    ret_value = XORP_ERROR;
	}
	if (set_accept_rtadv_enabled6(_accept_rtadv_enabled6, error_msg) < 0) {
	    ret_value = XORP_ERROR;
	}
    }
#endif // HAVE_IPV6

    _is_running = false;

    return (ret_value);
}

bool
FtiConfig::start_configuration()
{
    list<FtiConfigEntrySet*>::iterator ftic_entry_set_iter;
    list<FtiConfigTableSet*>::iterator ftic_table_set_iter;
    bool ret_value = true;

    //
    // XXX: we need to call start_configuration() for "entry" and "table",
    // because the top-level start/end configuration interface
    // does not distinguish between "entry" and "table" modification.
    //
    for (ftic_entry_set_iter = _ftic_entry_sets.begin();
	 ftic_entry_set_iter != _ftic_entry_sets.end();
	 ++ftic_entry_set_iter) {
	FtiConfigEntrySet* ftic_entry_set = *ftic_entry_set_iter;
	if (ftic_entry_set->start_configuration() != true)
	    ret_value = false;
    }
    for (ftic_table_set_iter = _ftic_table_sets.begin();
	 ftic_table_set_iter != _ftic_table_sets.end();
	 ++ftic_table_set_iter) {
	FtiConfigTableSet* ftic_table_set = *ftic_table_set_iter;
	if (ftic_table_set->start_configuration() != true)
	    ret_value = false;
    }
    
    return (ret_value);
}

bool
FtiConfig::end_configuration()
{
    list<FtiConfigEntrySet*>::iterator ftic_entry_set_iter;
    list<FtiConfigTableSet*>::iterator ftic_table_set_iter;
    bool ret_value = true;
    
    //
    // XXX: we need to call end_configuration() for "entry" and "table",
    // because the top-level start/end configuration interface
    // does not distinguish between "entry" and "table" modification.
    //
    for (ftic_entry_set_iter = _ftic_entry_sets.begin();
	 ftic_entry_set_iter != _ftic_entry_sets.end();
	 ++ftic_entry_set_iter) {
	FtiConfigEntrySet* ftic_entry_set = *ftic_entry_set_iter;
	if (ftic_entry_set->end_configuration() != true)
	    ret_value = false;
    }
    for (ftic_table_set_iter = _ftic_table_sets.begin();
	 ftic_table_set_iter != _ftic_table_sets.end();
	 ++ftic_table_set_iter) {
	FtiConfigTableSet* ftic_table_set = *ftic_table_set_iter;
	if (ftic_table_set->end_configuration() != true)
	    ret_value = false;
    }
    
    return (ret_value);
}

bool
FtiConfig::add_entry4(const Fte4& fte)
{
    list<FtiConfigEntrySet*>::iterator ftic_entry_set_iter;

    if (_ftic_entry_sets.empty())
	return (false);

    if (_profile.enabled(profile_route_out))
	_profile.log(profile_route_out,
		     c_format("add %s", fte.net().str().c_str()));

    for (ftic_entry_set_iter = _ftic_entry_sets.begin();
	 ftic_entry_set_iter != _ftic_entry_sets.end();
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

    if (_ftic_entry_sets.empty())
	return (false);

    if (_profile.enabled(profile_route_out))
	_profile.log(profile_route_out,
		     c_format("delete %s", fte.net().str().c_str()));

    for (ftic_entry_set_iter = _ftic_entry_sets.begin();
	 ftic_entry_set_iter != _ftic_entry_sets.end();
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

    if (_ftic_table_sets.empty())
	return (false);

    for (ftic_table_set_iter = _ftic_table_sets.begin();
	 ftic_table_set_iter != _ftic_table_sets.end();
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

    if (_ftic_table_sets.empty())
	return (false);

    for (ftic_table_set_iter = _ftic_table_sets.begin();
	 ftic_table_set_iter != _ftic_table_sets.end();
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
    list<FtiConfigEntryGet*>::iterator ftic_entry_get_iter;

    if (_ftic_entry_gets.empty())
	return (false);

    for (ftic_entry_get_iter = _ftic_entry_gets.begin();
	 ftic_entry_get_iter != _ftic_entry_gets.end();
	 ++ftic_entry_get_iter) {
	FtiConfigEntryGet* ftic_entry_get = *ftic_entry_get_iter;
	if (ftic_entry_get->lookup_route_by_dest4(dst, fte) != true)
	    return (false);
    }

    return (true);
}

bool
FtiConfig::lookup_route_by_network4(const IPv4Net& dst, Fte4& fte)
{
    list<FtiConfigEntryGet*>::iterator ftic_entry_get_iter;

    if (_ftic_entry_gets.empty())
	return (false);

    for (ftic_entry_get_iter = _ftic_entry_gets.begin();
	 ftic_entry_get_iter != _ftic_entry_gets.end();
	 ++ftic_entry_get_iter) {
	FtiConfigEntryGet* ftic_entry_get = *ftic_entry_get_iter;
	if (ftic_entry_get->lookup_route_by_network4(dst, fte) != true)
	    return (false);
    }

    return (true);
}

bool
FtiConfig::get_table4(list<Fte4>& fte_list)
{
    list<FtiConfigTableGet*>::iterator ftic_table_get_iter;

    if (_ftic_table_gets.empty())
	return (false);

    for (ftic_table_get_iter = _ftic_table_gets.begin();
	 ftic_table_get_iter != _ftic_table_gets.end();
	 ++ftic_table_get_iter) {
	FtiConfigTableGet* ftic_table_get = *ftic_table_get_iter;
	if (ftic_table_get->get_table4(fte_list) != true)
	    return (false);
    }

    return (true);
}

bool
FtiConfig::add_entry6(const Fte6& fte)
{
    list<FtiConfigEntrySet*>::iterator ftic_entry_set_iter;

    if (_ftic_entry_sets.empty())
	return (false);

    if (_profile.enabled(profile_route_out))
	_profile.log(profile_route_out,
		     c_format("add %s", fte.net().str().c_str()));

    for (ftic_entry_set_iter = _ftic_entry_sets.begin();
	 ftic_entry_set_iter != _ftic_entry_sets.end();
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

    if (_ftic_entry_sets.empty())
	return (false);

    if (_profile.enabled(profile_route_out))
	_profile.log(profile_route_out,
		     c_format("delete %s", fte.net().str().c_str()));

    for (ftic_entry_set_iter = _ftic_entry_sets.begin();
	 ftic_entry_set_iter != _ftic_entry_sets.end();
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

    if (_ftic_table_sets.empty())
	return (false);

    for (ftic_table_set_iter = _ftic_table_sets.begin();
	 ftic_table_set_iter != _ftic_table_sets.end();
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

    if (_ftic_table_sets.empty())
	return (false);

    for (ftic_table_set_iter = _ftic_table_sets.begin();
	 ftic_table_set_iter != _ftic_table_sets.end();
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
    list<FtiConfigEntryGet*>::iterator ftic_entry_get_iter;

    if (_ftic_entry_gets.empty())
	return (false);

    for (ftic_entry_get_iter = _ftic_entry_gets.begin();
	 ftic_entry_get_iter != _ftic_entry_gets.end();
	 ++ftic_entry_get_iter) {
	FtiConfigEntryGet* ftic_entry_get = *ftic_entry_get_iter;
	if (ftic_entry_get->lookup_route_by_dest6(dst, fte) != true)
	    return (false);
    }

    return (true);
}

bool
FtiConfig::lookup_route_by_network6(const IPv6Net& dst, Fte6& fte)
{
    list<FtiConfigEntryGet*>::iterator ftic_entry_get_iter;

    if (_ftic_entry_gets.empty())
	return (false);

    for (ftic_entry_get_iter = _ftic_entry_gets.begin();
	 ftic_entry_get_iter != _ftic_entry_gets.end();
	 ++ftic_entry_get_iter) {
	FtiConfigEntryGet* ftic_entry_get = *ftic_entry_get_iter;
	if (ftic_entry_get->lookup_route_by_network6(dst, fte) != true)
	    return (false);
    }

    return (true);
}

bool
FtiConfig::get_table6(list<Fte6>& fte_list)
{
    list<FtiConfigTableGet*>::iterator ftic_table_get_iter;

    if (_ftic_table_gets.empty())
	return (false);

    for (ftic_table_get_iter = _ftic_table_gets.begin();
	 ftic_table_get_iter != _ftic_table_gets.end();
	 ++ftic_table_get_iter) {
	FtiConfigTableGet* ftic_table_get = *ftic_table_get_iter;
	if (ftic_table_get->get_table6(fte_list) != true)
	    return (false);
    }

    return (true);
}

bool
FtiConfig::add_fib_table_observer(FibTableObserverBase* fib_table_observer)
{
    list<FtiConfigTableObserver*>::iterator ftic_table_observer_iter;

    if (_ftic_table_observers.empty())
	return (false);

    for (ftic_table_observer_iter = _ftic_table_observers.begin();
	 ftic_table_observer_iter != _ftic_table_observers.end();
	 ++ftic_table_observer_iter) {
	FtiConfigTableObserver* ftic_table_observer = *ftic_table_observer_iter;
	ftic_table_observer->add_fib_table_observer(fib_table_observer);
    }

    return (true);
}

bool
FtiConfig::delete_fib_table_observer(FibTableObserverBase* fib_table_observer)
{
    list<FtiConfigTableObserver*>::iterator ftic_table_observer_iter;

    if (_ftic_table_observers.empty())
	return (false);

    for (ftic_table_observer_iter = _ftic_table_observers.begin();
	 ftic_table_observer_iter != _ftic_table_observers.end();
	 ++ftic_table_observer_iter) {
	FtiConfigTableObserver* ftic_table_observer = *ftic_table_observer_iter;
	ftic_table_observer->delete_fib_table_observer(fib_table_observer);
    }

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

    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
	return (false);
    
    close(s);
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
    int s = socket(AF_INET6, SOCK_DGRAM, 0);
    if (s < 0)
	return (false);
    
    close(s);
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
	XLOG_ERROR(error_msg.c_str());
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
	    XLOG_ERROR(error_msg.c_str());
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
	    XLOG_ERROR(error_msg.c_str());
	    return (XORP_ERROR);
	}
	if (fscanf(fh, "%d", &enabled) != 1) {
	    error_msg = c_format("Error reading file %s: %s",
				 PROC_LINUX_FILE_FORWARDING_V4,
				 strerror(errno));
	    XLOG_ERROR(error_msg.c_str());
	    fclose(fh);
	    return (XORP_ERROR);
	}
	fclose(fh);
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
    XLOG_ERROR(error_msg.c_str());
    return (XORP_ERROR);
    
#else // HAVE_IPV6

    if (! have_ipv6()) {
	ret_value = false;
	error_msg = c_format("Cannot test whether IPv6 unicast forwarding "
			     "is enabled: IPv6 is not supported");
	XLOG_ERROR(error_msg.c_str());
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
	    XLOG_ERROR(error_msg.c_str());
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
	    XLOG_ERROR(error_msg.c_str());
	    return (XORP_ERROR);
	}
	if (fscanf(fh, "%d", &enabled) != 1) {
	    error_msg = c_format("Error reading file %s: %s",
				 PROC_LINUX_FILE_FORWARDING_V6,
				 strerror(errno));
	    XLOG_ERROR(error_msg.c_str());
	    fclose(fh);
	    return (XORP_ERROR);
	}
	fclose(fh);
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
    XLOG_ERROR(error_msg.c_str());
    return (XORP_ERROR);
    
#else // HAVE_IPV6

    if (! have_ipv6()) {
	ret_value = false;
	error_msg = c_format("Cannot test whether the acceptance of IPv6 "
			     "Router Advertisement messages is enabled: "
			     "IPv6 is not supported");
	XLOG_ERROR(error_msg.c_str());
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
	    XLOG_ERROR(error_msg.c_str());
	    return (XORP_ERROR);
	}
    }
#elif defined(HOST_OS_LINUX)
    // XXX: nothing to do in case of Linux
    error_msg = "";
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
	XLOG_ERROR(error_msg.c_str());
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
	    XLOG_ERROR(error_msg.c_str());
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
	    XLOG_ERROR(error_msg.c_str());
	    return (XORP_ERROR);
	}
	if (fprintf(fh, "%d", enable) != 1) {
	    error_msg = c_format("Error writing %d to file %s: %s",
				 enable,
				 PROC_LINUX_FILE_FORWARDING_V4,
				 strerror(errno));
	    XLOG_ERROR(error_msg.c_str());
	    fclose(fh);
	    return (XORP_ERROR);
	}
	fclose(fh);
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
    XLOG_ERROR(error_msg.c_str());
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
	XLOG_ERROR(error_msg.c_str());
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
	    XLOG_ERROR(error_msg.c_str());
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
	    XLOG_ERROR(error_msg.c_str());
	    return (XORP_ERROR);
	}
	if (fprintf(fh, "%d", enable) != 1) {
	    error_msg = c_format("Error writing %d to file %s: %s",
				 enable,
				 PROC_LINUX_FILE_FORWARDING_V6,
				 strerror(errno));
	    XLOG_ERROR(error_msg.c_str());
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
    XLOG_ERROR(error_msg.c_str());
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
	XLOG_ERROR(error_msg.c_str());
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
	    XLOG_ERROR(error_msg.c_str());
	    return (XORP_ERROR);
	}
    }
#elif defined(HOST_OS_LINUX)
    {
	// XXX: nothing to do in case of Linux
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
