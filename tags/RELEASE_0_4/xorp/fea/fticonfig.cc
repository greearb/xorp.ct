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

#ident "$XORP: xorp/fea/fticonfig.cc,v 1.5 2003/08/15 23:58:36 pavlin Exp $"


#include "fea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif

#include "fticonfig.hh"

//
// Unicast forwarding table related configuration.
//


FtiConfig::FtiConfig(EventLoop& eventloop)
    : _eventloop(eventloop),
      _ftic_entry_get(NULL), _ftic_entry_set(NULL), _ftic_entry_observer(NULL),
      _ftic_table_get(NULL), _ftic_table_set(NULL), _ftic_table_observer(NULL),
      _ftic_entry_get_dummy(*this),
      _ftic_entry_get_netlink(*this),
      _ftic_entry_get_rtsock(*this),
      _ftic_entry_set_dummy(*this),
      _ftic_entry_set_rtsock(*this),
      _ftic_entry_observer_dummy(*this),
      _ftic_entry_observer_rtsock(*this),
      _ftic_table_get_dummy(*this),
      _ftic_table_get_netlink(*this),
      _ftic_table_get_sysctl(*this),
      _ftic_table_set_dummy(*this),
      _ftic_table_set_rtsock(*this),
      _ftic_table_observer_dummy(*this),
      _ftic_table_observer_rtsock(*this),
      _unicast_forwarding_enabled4(false),
      _unicast_forwarding_enabled6(false),
      _accept_rtadv_enabled6(false)
{
    string error_msg;
    
    //
    // Get the old state from the underlying system
    //
    if (unicast_forwarding_enabled4(_unicast_forwarding_enabled4, error_msg)
	< 0) {
	XLOG_FATAL(error_msg.c_str());
    }
#ifdef HAVE_IPV6
    if (unicast_forwarding_enabled6(_unicast_forwarding_enabled6, error_msg)
	< 0) {
	XLOG_FATAL(error_msg.c_str());
    }
    if (accept_rtadv_enabled6(_accept_rtadv_enabled6, error_msg) < 0) {
	XLOG_FATAL(error_msg.c_str());
    }
#endif // HAVE_IPV6
}

FtiConfig::~FtiConfig()
{
    string error_msg;
    
    stop();
    
    //
    // Restore the old state in the underlying system
    //
    set_unicast_forwarding_enabled4(_unicast_forwarding_enabled4, error_msg);
#ifdef HAVE_IPV6
    set_unicast_forwarding_enabled6(_unicast_forwarding_enabled6, error_msg);
    set_accept_rtadv_enabled6(_accept_rtadv_enabled6, error_msg);
#endif // HAVE_IPV6
}

int
FtiConfig::register_ftic_entry_get(FtiConfigEntryGet *ftic_entry_get)
{
    _ftic_entry_get = ftic_entry_get;
    
    return (XORP_OK);
}

int
FtiConfig::register_ftic_entry_set(FtiConfigEntrySet *ftic_entry_set)
{
    _ftic_entry_set = ftic_entry_set;
    
    return (XORP_OK);
}

int
FtiConfig::register_ftic_entry_observer(FtiConfigEntryObserver *ftic_entry_observer)
{
    _ftic_entry_observer = ftic_entry_observer;
    
    return (XORP_OK);
}

int
FtiConfig::register_ftic_table_get(FtiConfigTableGet *ftic_table_get)
{
    _ftic_table_get = ftic_table_get;
    
    return (XORP_OK);
}

int
FtiConfig::register_ftic_table_set(FtiConfigTableSet *ftic_table_set)
{
    _ftic_table_set = ftic_table_set;
    
    return (XORP_OK);
}

int
FtiConfig::register_ftic_table_observer(FtiConfigTableObserver *ftic_table_observer)
{
    _ftic_table_observer = ftic_table_observer;
    
    return (XORP_OK);
}

int
FtiConfig::set_dummy()
{
    register_ftic_entry_get(&_ftic_entry_get_dummy);
    register_ftic_entry_set(&_ftic_entry_set_dummy);
    register_ftic_entry_observer(&_ftic_entry_observer_dummy);
    register_ftic_table_get(&_ftic_table_get_dummy);
    register_ftic_table_set(&_ftic_table_set_dummy);
    register_ftic_table_observer(&_ftic_table_observer_dummy);
    
    return (XORP_OK);
}

int
FtiConfig::start()
{
    if (_ftic_entry_get != NULL) {
	if (_ftic_entry_get->start() < 0)
	    return (XORP_ERROR);
    }
    if (_ftic_entry_set != NULL) {
	if (_ftic_entry_set->start() < 0)
	    return (XORP_ERROR);
    }
    if (_ftic_entry_observer != NULL) {
	if (_ftic_entry_observer->start() < 0)
	    return (XORP_ERROR);
    }
    if (_ftic_table_get != NULL) {
	if (_ftic_table_get->start() < 0)
	    return (XORP_ERROR);
    }
    if (_ftic_table_set != NULL) {
	if (_ftic_table_set->start() < 0)
	    return (XORP_ERROR);
    }
    if (_ftic_table_observer != NULL) {
	if (_ftic_table_observer->start() < 0)
	    return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

int
FtiConfig::stop()
{
    int ret_value = XORP_OK;
    
    if (_ftic_table_observer != NULL) {
	if (_ftic_table_observer->stop() < 0)
	    ret_value = XORP_ERROR;
    }
    if (_ftic_table_set != NULL) {
	if (_ftic_table_set->stop() < 0)
	    ret_value = XORP_ERROR;
    }
    if (_ftic_table_get != NULL) {
	if (_ftic_table_get->stop() < 0)
	    ret_value = XORP_ERROR;
    }
    if (_ftic_entry_observer != NULL) {
	if (_ftic_entry_observer->stop() < 0)
	    ret_value = XORP_ERROR;
    }
    if (_ftic_entry_set != NULL) {
	if (_ftic_entry_set->stop() < 0)
	    ret_value = XORP_ERROR;
    }
    if (_ftic_entry_get != NULL) {
	if (_ftic_entry_get->stop() < 0)
	    ret_value = XORP_ERROR;
    }
    
    return (ret_value);
}

bool
FtiConfig::start_configuration()
{
    bool ret_value = false;

    //
    // XXX: we need to call start_configuration() for "entry" and "table",
    // because the top-level start/end configuration interface
    // does not distinguish between "entry" and "table" modification.
    //
    if (_ftic_entry_set != NULL) {
	if (_ftic_entry_set->start_configuration() == true)
	    ret_value = true;
    }
    if (_ftic_table_set != NULL) {
	if (_ftic_table_set->start_configuration() == true)
	    ret_value = true;
    }
    
    return ret_value;
}

bool
FtiConfig::end_configuration()
{
    bool ret_value = false;
    
    //
    // XXX: we need to call end_configuration() for "entry" and "table",
    // because the top-level start/end configuration interface
    // does not distinguish between "entry" and "table" modification.
    //
    if (_ftic_entry_set != NULL) {
	if (_ftic_entry_set->end_configuration() == true)
	    ret_value = true;
    }
    if (_ftic_table_set != NULL) {
	if (_ftic_table_set->end_configuration() == true)
	    ret_value = true;
    }
    
    return ret_value;
}

bool
FtiConfig::add_entry4(const Fte4& fte)
{
    if (_ftic_entry_set == NULL)
	return false;
    return (_ftic_entry_set->add_entry4(fte));
}

bool
FtiConfig::delete_entry4(const Fte4& fte)
{
    if (_ftic_entry_set == NULL)
	return false;
    return (_ftic_entry_set->delete_entry4(fte));
}

bool
FtiConfig::set_table4(const list<Fte4>& fte_list)
{
    if (_ftic_table_set == NULL)
	return false;
    return (_ftic_table_set->set_table4(fte_list));
}

bool
FtiConfig::delete_all_entries4()
{
    if (_ftic_table_set == NULL)
	return false;
    return (_ftic_table_set->delete_all_entries4());
}

bool
FtiConfig::lookup_route4(const IPv4& dst, Fte4& fte)
{
    if (_ftic_entry_get == NULL)
	return false;
    return (_ftic_entry_get->lookup_route4(dst, fte));
}

bool
FtiConfig::lookup_entry4(const IPv4Net& dst, Fte4& fte)
{
    if (_ftic_entry_get == NULL)
	return false;
    return (_ftic_entry_get->lookup_entry4(dst, fte));
}

bool
FtiConfig::get_table4(list<Fte4>& fte_list)
{
    if (_ftic_table_get == NULL)
	return false;
    return (_ftic_table_get->get_table4(fte_list));
}

bool
FtiConfig::add_entry6(const Fte6& fte)
{
    if (_ftic_entry_set == NULL)
	return false;
    return (_ftic_entry_set->add_entry6(fte));
}

bool
FtiConfig::delete_entry6(const Fte6& fte)
{
    if (_ftic_entry_set == NULL)
	return false;
    return (_ftic_entry_set->delete_entry6(fte));
}

bool
FtiConfig::set_table6(const list<Fte6>& fte_list)
{
    if (_ftic_table_set == NULL)
	return false;
    return (_ftic_table_set->set_table6(fte_list));
}

bool
FtiConfig::delete_all_entries6()
{
    if (_ftic_table_set == NULL)
	return false;
    return (_ftic_table_set->delete_all_entries6());
}

bool
FtiConfig::lookup_route6(const IPv6& dst, Fte6& fte)
{
    if (_ftic_entry_get == NULL)
	return false;
    return (_ftic_entry_get->lookup_route6(dst, fte));
}

bool
FtiConfig::lookup_entry6(const IPv6Net& dst, Fte6& fte)
{
    if (_ftic_entry_get == NULL)
	return false;
    return (_ftic_entry_get->lookup_entry6(dst, fte));
}

bool
FtiConfig::get_table6(list<Fte6>& fte_list)
{
    if (_ftic_table_get == NULL)
	return false;
    return (_ftic_table_get->get_table6(fte_list));
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
    size_t enabled = 0;
    size_t sz = sizeof(enabled);
    
#if defined(HOST_OS_FREEBSD) || defined(HOST_OS_NETBSD) || defined(HOST_OS_OPENBSD)
    if (sysctlbyname("net.inet.ip.forwarding", &enabled, &sz, NULL, 0)
	!= 0) {
	error_msg = c_format("Cannot get 'net.inet.ip.forwarding': %s",
			     strerror(errno));
	XLOG_ERROR(error_msg.c_str());
	return (XORP_ERROR);
    }
#elif defined(HOST_OS_LINUX)
#if 0	// TODO: XXX: PAVPAVPAV: Linux doesn't hae sysctlbyname...
    if (sysctlbyname("net.ipv4.conf.all.forwarding", &enabled, &sz, NULL, 0)
	!= 0) {
	error_msg = c_format("Cannot get 'net.ipv4.conf.all.forwarding': %s",
			     strerror(errno));
	XLOG_ERROR(error_msg.c_str());
	return (XORP_ERROR);
    }
#endif
    UNUSED(enabled);
    UNUSED(sz);
    UNUSED(error_msg);
    
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
#ifndef HAVE_IPV6
    ret_value = false;
    error_msg = c_format("Cannot test whether IPv6 unicast forwarding "
			 "is enabled: IPv6 is not supported");
    XLOG_ERROR(error_msg.c_str());
    return (XORP_ERROR);
    
#else // HAVE_IPV6
    
    size_t enabled = 0;
    size_t sz = sizeof(enabled);
    
#if defined(HOST_OS_FREEBSD) || defined(HOST_OS_NETBSD) || defined(HOST_OS_OPENBSD)
    if (sysctlbyname("net.inet6.ip6.forwarding", &enabled, &sz, NULL, 0)
	!= 0) {
	error_msg = c_format("Cannot get 'net.inet6.ip6.forwarding': %s",
			     strerror(errno));
	XLOG_ERROR(error_msg.c_str());
	return (XORP_ERROR);
    }
#elif defined(HOST_OS_LINUX)
#if 0	// TODO: XXX: PAVPAVPAV: Linux doesn't hae sysctlbyname...
    if (sysctlbyname("net.ipv6.conf.all.forwarding", &enabled, &sz, NULL, 0)
	!= 0) {
	error_msg = c_format("Cannot get 'net.ipv6.conf.all.forwarding': %s",
			     strerror(errno));
	XLOG_ERROR(error_msg.c_str());
	return (XORP_ERROR);
    }
#endif
    UNUSED(enabled);
    UNUSED(sz);
    UNUSED(error_msg);
    
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
#ifndef HAVE_IPV6
    ret_value = false;
    error_msg = c_format("Cannot test whether the acceptance of IPv6 "
			 "Router Advertisement messages is enabled: "
			 "IPv6 is not supported");
    XLOG_ERROR(error_msg.c_str());
    return (XORP_ERROR);
    
#else // HAVE_IPV6
    
    size_t enabled = 0;
    size_t sz = sizeof(enabled);
    
#if defined(HOST_OS_FREEBSD) || defined(HOST_OS_NETBSD) || defined(HOST_OS_OPENBSD)
    if (sysctlbyname("net.inet6.ip6.accept_rtadv", &enabled, &sz, NULL, 0)
	!= 0) {
	error_msg = c_format("Cannot get 'net.inet6.ip6.accept_rtadv': %s",
			     strerror(errno));
	XLOG_ERROR(error_msg.c_str());
	return (XORP_ERROR);
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
    size_t enable = (v) ? 1 : 0;
    bool old_value;
    
    if (unicast_forwarding_enabled4(old_value, error_msg) < 0)
	return (XORP_ERROR);
    
    if (old_value == v)
	return (XORP_OK);	// Nothing changed
    
#if defined(HOST_OS_FREEBSD) || defined(HOST_OS_NETBSD) || defined(HOST_OS_OPENBSD)
    if (sysctlbyname("net.inet.ip.forwarding", NULL, NULL,
		     &enable, sizeof(enable))
	!= 0) {
	error_msg = c_format("Cannot set 'net.inet.ip.forwarding' "
			     "to %d: %s", enable, strerror(errno));
	XLOG_ERROR(error_msg.c_str());
	return (XORP_ERROR);
    }
#elif defined(HOST_OS_LINUX)
#if 0	// TODO: XXX: PAVPAVPAV: Linux doesn't hae sysctlbyname...
    if (sysctlbyname("net.ipv4.conf.all.forwarding", NULL, NULL,
		     &enable, sizeof(enable))
	!= 0) {
	error_msg = c_format("Cannot set 'net.ipv4.conf.all.forwarding' "
			     "to %d: %s", enable, strerror(errno));
	XLOG_ERROR(error_msg.c_str());
	return (XORP_ERROR);
    }
#endif // 0
    UNUSED(enable);
    UNUSED(error_msg);
    
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
#ifndef HAVE_IPV6
    error_msg = c_format("Cannot set IPv6 unicast forwarding to %s: "
			 "IPv6 is not supported", (v) ? "true": "false");
    XLOG_ERROR(error_msg.c_str());
    return (XORP_ERROR);
    
#else // HAVE_IPV6
    
    size_t enable = (v) ? 1 : 0;
    bool old_value, old_value_accept_rtadv;
    
    if (unicast_forwarding_enabled6(old_value, error_msg) < 0)
	return (XORP_ERROR);
    if (accept_rtadv_enabled6(old_value_accept_rtadv, error_msg) < 0)
	return (XORP_ERROR);
    
    if ((old_value == v) && (old_value_accept_rtadv == !v))
	return (XORP_OK);	// Nothing changed
    
    if (set_accept_rtadv_enabled6(!v, error_msg) < 0)
	return (XORP_ERROR);
    
#if defined(HOST_OS_FREEBSD) || defined(HOST_OS_NETBSD) || defined(HOST_OS_OPENBSD)
    if (sysctlbyname("net.inet6.ip6.forwarding", NULL, NULL,
		     &enable, sizeof(enable))
	!= 0) {
	error_msg = c_format("Cannot set 'net.inet6.ip6.forwarding' "
			     "to %d: %s", enable, strerror(errno));
	XLOG_ERROR(error_msg.c_str());
	// Restore the old accept_rtadv value
	if (old_value_accept_rtadv != !v) {
	    string dummy_error_msg;
	    set_accept_rtadv_enabled6(old_value_accept_rtadv, dummy_error_msg);
	}
	return (XORP_ERROR);
    }
#elif defined(HOST_OS_LINUX)
#if 0	// TODO: XXX: PAVPAVPAV: Linux doesn't hae sysctlbyname...
    if (sysctlbyname("net.ipv6.conf.all.forwarding", NULL, NULL,
		     &enable, sizeof(enable))
	!= 0) {
	error_msg = c_format("Cannot set 'net.ipv6.conf.all.forwarding' "
			     "to %d: %s", enable, strerror(errno));
	XLOG_ERROR(error_msg.c_str());
	// Restore the old accept_rtadv value
	if (old_value_accept_rtadv != !v) {
	    string dummy_error_msg;
	    set_accept_rtadv_enabled6(old_value_accept_rtadv, dummy_error_msg);
	}
	return (XORP_ERROR);
    }
#endif // 0
    UNUSED(enable);
    UNUSED(error_msg);
    
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
#ifndef HAVE_IPV6
    error_msg = c_format("Cannot set the acceptance of IPv6 "
			 "Router Advertisement messages to %s: "
			 "IPv6 is not supported",
			 (v) ? "true": "false");
    XLOG_ERROR(error_msg.c_str());
    return (XORP_ERROR);
    
#else // HAVE_IPV6

    size_t enable = (v) ? 1 : 0;
    bool old_value;
    
    if (accept_rtadv_enabled6(old_value, error_msg) < 0)
	return (XORP_ERROR);
    
    if (old_value == v)
	return (XORP_OK);	// Nothing changed
    
#if defined(HOST_OS_FREEBSD) || defined(HOST_OS_NETBSD) || defined(HOST_OS_OPENBSD)
    if (sysctlbyname("net.inet6.ip6.accept_rtadv", NULL, NULL,
		     &enable, sizeof(enable))
	!= 0) {
	error_msg = c_format("Cannot set 'net.inet6.ip6.accept_rtadv' "
			     "to %d: %s", enable, strerror(errno));
	XLOG_ERROR(error_msg.c_str());
	return (XORP_ERROR);
    }
#elif defined(HOST_OS_LINUX)
    // XXX: nothing to do in case of Linux
    error_msg = "";
#else
#error "OS not supported: don't know how to enable/disable"
#error "the acceptance of IPv6 Router Advertisement messages"
#endif
    
    return (XORP_OK);
#endif // HAVE_IPV6
}
