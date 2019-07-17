// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2012 XORP, Inc and Others
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net



//
// Multicast routing kernel-access specific implementation.
//

#include "mfea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"
#include "libxorp/utils.hh"

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif
//
// XXX: We need to include <linux/types.h> before <sys/sysctl.h> because
// the latter is broken on many systems (e.g., Linux Debian-3.1, Linux
// Ubuntu-7.04, Gentoo-2006.1): file <linux/types.h> is needed if
// <linux/mroute.h> is included, but <sys/sysctl.h> contains some hacks
// to prevent the inclusion of <linux/types.h>.
//
#ifdef HAVE_LINUX_TYPES_H
#include <linux/types.h>
#endif
#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_NET_IF_VAR_H
#include <net/if_var.h>
// if_name() is a macro in FreeBSD 7.0 conflicting with our method names.
#ifdef if_name
#undef if_name
#endif
#endif
#ifdef HAVE_NETINET_IN_SYSTM_H
#include <netinet/in_systm.h>
#endif
#ifdef HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif
#ifdef HAVE_NETINET_IP6_H
#include <netinet/ip6.h>
#endif
#ifdef HAVE_NETINET_ICMP6_H
#include <netinet/icmp6.h>
#endif
#ifdef HAVE_NETINET6_IN6_VAR_H
#include <netinet6/in6_var.h>
#endif

#include "libcomm/comm_api.h"

#include "libproto/packet.hh"

#include "mrt/include/ip_mroute.h"
#include "mrt/max_vifs.h"
#include "mrt/multicast_defs.h"

#include "fea_node.hh"
#include "mfea_node.hh"
#include "mfea_vif.hh"
#include "mfea_kernel_messages.hh"
#include "mfea_osdep.hh"
#include "mfea_mrouter.hh"
#include "fibconfig.hh"


bool new_mcast_tables_api = false;

#ifdef USE_MULT_MCAST_TABLES
/** In order to support multiple routing tables, the kernel API had to be extended.
 * Since no distro has this currently in #include files, add private definitions
 * here. --Ben
 */

// Assume supported until we know otherwise.
bool supports_mcast_tables = true;

#define DFLT_MROUTE_TBL 253  /* 'default' routing table id in Linux */


// Support for multiple routing tables.
#define SIOCGETVIFCNT_NG	(SIOCPROTOPRIVATE+3)
#define SIOCGETSGCNT_NG	(SIOCPROTOPRIVATE+4)

/* For supporting multiple routing tables */
struct vifctl_ng {
	struct vifctl vif;
	unsigned int table_id;
} __attribute__ ((packed));

struct mfcctl_ng {
	struct mfcctl mfc;
	unsigned int table_id;
} __attribute__ ((packed));

/* Used with these options:
	case MRT_INIT:
	case MRT_DONE:
	case MRT_ASSERT:
#ifdef CONFIG_IP_PIMSM
	case MRT_PIM:
#endif
and all getsockopt options
*/
struct mrt_sockopt_simple {
	unsigned int optval;
	unsigned int table_id;
};

struct sioc_sg_req_ng {
	struct sioc_sg_req req;
	unsigned int table_id;
} __attribute__ ((packed));
	
struct sioc_vif_req_ng {
	struct sioc_vif_req vif;
	unsigned int table_id;
} __attribute__ ((packed));


/* New mcast API */
#ifndef MRT_TABLE
#define MRT_TABLE      (MRT_BASE+9)    /* Specify mroute table ID              */
#endif

#ifndef MRT6_TABLE
#define MRT6_TABLE      (MRT6_BASE+9)    /* Specify mroute table ID              */
#endif

#else
/* Don't support multiple mcast routing tables */
bool supports_mcast_tables = false;
#endif


#ifdef HOST_OS_WINDOWS
typedef char *caddr_t;
#endif

/**
 * MfeaMrouter::MfeaMrouter:
 * @mfea_node: The MfeaNode I belong to.
 **/
MfeaMrouter::MfeaMrouter(MfeaNode& mfea_node, const FibConfig& fibconfig)
    : ProtoUnit(mfea_node.family(), mfea_node.module_id()),
      _mfea_node(mfea_node),
      _mrt_api_mrt_mfc_flags_disable_wrongvif(false),
      _mrt_api_mrt_mfc_flags_border_vif(false),
      _mrt_api_mrt_mfc_rp(false),
      _mrt_api_mrt_mfc_bw_upcall(false),
      _multicast_forwarding_enabled(false)
#ifdef USE_MULT_MCAST_TABLES
      ,_fibconfig(fibconfig)
#endif
{
    string error_msg;

#ifndef USE_MULT_MCAST_TABLES
    UNUSED(fibconfig);
#endif

    //
    // Get the old state from the underlying system
    //
    int ret_value = XORP_OK;
    switch (family()) {
    case AF_INET:
	ret_value = multicast_forwarding_enabled4(_multicast_forwarding_enabled,
						  error_msg);
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	ret_value = multicast_forwarding_enabled6(_multicast_forwarding_enabled,
						  error_msg);
	break;
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
    }
    if (ret_value != XORP_OK) {
	XLOG_FATAL("%s", error_msg.c_str());
    }
}

MfeaMrouter::~MfeaMrouter()
{
    stop();
}

/**
 * MfeaMrouter::start:
 * @: 
 * 
 * Start the MfeaMrouter.
 * 
 * Return value: %XORP_OK on success, otherwize %XORP_ERROR.
 **/
int
MfeaMrouter::start()
{
    string error_msg;

#ifdef HOST_OS_WINDOWS
    XLOG_ERROR("Multicast routing is not supported on Windows");
    return (XORP_ERROR);
#endif

    // XXX: MfeaMrouter is automatically enabled by default
    ProtoUnit::enable();
    
    if (is_up() || is_pending_up())
	return (XORP_OK);

    if (ProtoUnit::start() != XORP_OK)
	return (XORP_ERROR);
    
#ifndef HOST_OS_WINDOWS
    // Check if we have the necessary permission
    if (geteuid() != 0) {
	XLOG_ERROR("Must be root");
	exit (1);
	// return (XORP_ERROR);
    }
#endif // ! HOST_OS_WINDOWS

    // Register as multicast upcall receiver
    string vrf;
#ifdef USE_MULT_MCAST_TABLES
    vrf = _fibconfig.get_vrf_name();
#endif

    IoIpManager& io_ip_manager = mfea_node().fea_node().io_ip_manager();
    uint8_t ip_protocol = kernel_mrouter_ip_protocol();
    if (io_ip_manager.register_system_multicast_upcall_receiver(
	    family(),
	    ip_protocol,
	    callback(this, &MfeaMrouter::kernel_call_process),
	    _mrouter_socket,
	    vrf,
	    error_msg)
	!= XORP_OK) {
	XLOG_ERROR("Cannot register multicast upcall receiver: %s",
		   error_msg.c_str());
	return (XORP_ERROR);
    }
    if (! _mrouter_socket.is_valid()) {
	XLOG_ERROR("Failed to assign the multicast routing socket, vrf: %s", vrf.c_str());
	return (XORP_ERROR);
    }
    else {
	XLOG_INFO("Using mrouter-socket: %d\n", _mrouter_socket.getSocket());
    }

    // Start the multicast routing in the kernel
    if (start_mrt() != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

/**
 * MfeaMrouter::stop:
 * @: 
 * 
 * Stop the MfeaMrouter.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaMrouter::stop()
{
    string error_msg;

    if (is_down())
	return (XORP_OK);

    if (ProtoUnit::stop() != XORP_OK)
	return (XORP_ERROR);

    // Stop the multicast routing in the kernel
    stop_mrt();
    
    // Clear kernel multicast routing access socket
    _mrouter_socket.clear();

    // Unregister as multicast upcall receiver
    IoIpManager& io_ip_manager = mfea_node().fea_node().io_ip_manager();
    uint8_t ip_protocol = kernel_mrouter_ip_protocol();
    if (io_ip_manager.unregister_system_multicast_upcall_receiver(
	    family(),
	    ip_protocol,
	    error_msg)
	!= XORP_OK) {
	XLOG_ERROR("Cannot unregister multicast upcall receiver: %s",
		   error_msg.c_str());
	return (XORP_ERROR);
    }

    //
    // Restore the old forwarding state in the underlying system.
    //
    int ret_value = XORP_OK;
    switch (family()) {
    case AF_INET:
	ret_value = set_multicast_forwarding_enabled4(_multicast_forwarding_enabled,
						      error_msg);
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	ret_value = set_multicast_forwarding_enabled6(_multicast_forwarding_enabled,
						      error_msg);
	break;
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
    }
    if (ret_value != XORP_OK) {
	XLOG_ERROR("Cannot restore the multicast forwarding state: %s",
		   error_msg.c_str());
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

#ifdef USE_MULT_MCAST_TABLES
int MfeaMrouter::getTableId() const {
    int table_id = DFLT_MROUTE_TBL;
    if (_fibconfig.unicast_forwarding_table_id_is_configured(family())) {
        table_id = _fibconfig.unicast_forwarding_table_id(family());
    }
    return table_id;
}
#endif

/**
 * Test if the underlying system supports IPv4 multicast routing.
 * 
 * @return true if the underlying system supports IPv4 multicast routing,
 * otherwise false.
 */
bool
MfeaMrouter::have_multicast_routing4() const
{
#ifndef HAVE_IPV4_MULTICAST_ROUTING
    return (false);
#else
    int s;
    int mrouter_version = 1;	// XXX: hardcoded version

#ifdef USE_MULT_MCAST_TABLES
    struct mrt_sockopt_simple tmp;
    memset(&tmp, 0, sizeof(tmp));
    tmp.table_id = getTableId();
    tmp.optval = 1; //version
#endif

    if (! is_ipv4())
	return (false);		// Wrong family
    
    //
    // Test to open and initialize a mrouter socket. If success,
    // then we support multicast routing.
    //
    if (mrouter_socket() >= 0)
	return (true);		// XXX: already have an open mrouter socket
    
    if (kernel_mrouter_ip_protocol() < 0)
	return (false);
    
    s = socket(family(), SOCK_RAW, kernel_mrouter_ip_protocol());
    if (s < 0)
	return (false);		// Failure to open the socket

    string vrf;
#ifdef USE_MULT_MCAST_TABLES
    vrf = _fibconfig.get_vrf_name();
#endif
    comm_set_bindtodevice(s, vrf.c_str());

    // First, try for multiple routing tables.
    bool do_mrt_init = true;
    new_mcast_tables_api = false;
#ifdef USE_MULT_MCAST_TABLES
    errno = 0;
    int rv;

    // Try old hacked API
    rv = setsockopt(s, IPPROTO_IP, MRT_INIT, &tmp, sizeof(tmp));
    if (rv < 0) {
	// try new api
	uint32_t tbl = getTableId();
	rv = setsockopt(s, IPPROTO_IP, MRT_TABLE, &tbl, sizeof(tbl));
	if (rv >= 0) {
	    new_mcast_tables_api = true;
	}
    }
    else {
	// old api worked
	do_mrt_init = false;
    }

    if (rv < 0) {
	// Ok, doesn't support new or old API
	supports_mcast_tables = false;
    }
    else {
	supports_mcast_tables = true;
    }
#endif

    if (do_mrt_init) {
	if (setsockopt(s, IPPROTO_IP, MRT_INIT, &mrouter_version, sizeof(mrouter_version)) < 0) {
	    close(s);
	    return (false);
	}
    }
    
    // Success
    close(s);
    return (true);
#endif // HAVE_IPV4_MULTICAST_ROUTING
}

/**
 * Test if the underlying system supports IPv6 multicast routing.
 * 
 * @return true if the underlying system supports IPv6 multicast routing,
 * otherwise false.
 */
bool
MfeaMrouter::have_multicast_routing6() const
{
#ifndef HAVE_IPV6_MULTICAST_ROUTING
    return (false);
#else
    int s;
    int mrouter_version = 1;	// XXX: hardcoded version
    
    if (! is_ipv6())
	return (false);		// Wrong family
    
    //
    // Test to open and initialize a mrouter socket. If success,
    // then we support multicast routing.
    //
    if (mrouter_socket() >= 0)
	return (true);		// XXX: already have an open mrouter socket
    
    if (kernel_mrouter_ip_protocol() < 0)
	return (false);
    
    s = socket(family(), SOCK_RAW, kernel_mrouter_ip_protocol());
    if (s < 0)
	return (false);		// Failure to open the socket

    string vrf;
#ifdef USE_MULT_MCAST_TABLES
    vrf = _fibconfig.get_vrf_name();
#endif
    comm_set_bindtodevice(s, vrf.c_str());

    if (setsockopt(s, IPPROTO_IPV6, MRT6_INIT,
		   (void *)&mrouter_version, sizeof(mrouter_version))
	< 0) {
	close(s);
	return (false);
    }

    // Success
    close(s);
    return (true);
#endif // HAVE_IPV6_MULTICAST_ROUTING
}

/**
 * Test whether the IPv4 multicast forwarding engine is enabled or disabled
 * to forward packets.
 * 
 * @param ret_value if true on return, then the IPv4 multicast forwarding
 * is enabled, otherwise is disabled.
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
MfeaMrouter::multicast_forwarding_enabled4(bool& ret_value,
					   string& error_msg) const
{
    int enabled = 0;

    UNUSED(error_msg);

    // XXX: always return true if running in dummy mode
    if (mfea_node().is_dummy()) {
	ret_value = true;
	return (XORP_OK);
    }

    //
    // XXX: Don't check whether the system supports IPv4 multicast routing,
    // because this might require to run as a root.
    //

#if defined(CTL_NET) && defined(IPPROTO_IP) && defined(IPCTL_MFORWARDING)
    {
	size_t sz = sizeof(enabled);
	int mib[4];
	
	mib[0] = CTL_NET;
	mib[1] = AF_INET;
	mib[2] = IPPROTO_IP;
	mib[3] = IPCTL_MFORWARDING;
	
	if (sysctl(mib, sizeof(mib) / sizeof(mib[0]), &enabled, &sz, NULL, 0)
	    != 0) {
	    error_msg = c_format("Get sysctl(IPCTL_MFORWARDING) failed: %s",
				 strerror(errno));
	    XLOG_ERROR("%s", error_msg.c_str());
	    return (XORP_ERROR);
	}
    }

#else // ! IPCTL_MFORWARDING

    // XXX: Not all systems have such additional control mechanism to
    // explicitly enable multicast forwarding, hence assume it is enabled.
    enabled = 1;
#endif // ! IPCTL_MFORWARDING

    if (enabled > 0)
	ret_value = true;
    else
	ret_value = false;
    
    return (XORP_OK);
}

/**
 * Test whether the IPv6 multicast forwarding engine is enabled or disabled
 * to forward packets.
 * 
 * @param ret_value if true on return, then the IPv6 multicast forwarding
 * is enabled, otherwise is disabled.
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
MfeaMrouter::multicast_forwarding_enabled6(bool& ret_value,
					   string& error_msg) const
{
    int enabled = 0;

    UNUSED(error_msg);

    // XXX: always return true if running in dummy mode
    if (mfea_node().is_dummy()) {
	ret_value = true;
	return (XORP_OK);
    }

    //
    // XXX: Don't check whether the system supports IPv6 multicast routing,
    // because this might require to run as a root.
    //

#if defined(CTL_NET) && defined(IPPROTO_IPV6) && defined(IPV6CTL_MFORWARDING)
    {
	size_t sz = sizeof(enabled);
	int mib[4];
	
	mib[0] = CTL_NET;
	mib[1] = AF_INET6;
	mib[2] = IPPROTO_IPV6;
	mib[3] = IPV6CTL_MFORWARDING;
	
	if (sysctl(mib, sizeof(mib) / sizeof(mib[0]), &enabled, &sz, NULL, 0)
	    != 0) {
	    error_msg = c_format("Get sysctl(IPV6CTL_MFORWARDING) failed: %s",
				 strerror(errno));
	    XLOG_ERROR("%s", error_msg.c_str());
	    return (XORP_ERROR);
	}
    }

#else // ! IPV6CTL_MFORWARDING
    //
    // XXX: Not all systems have such additional control mechanism to
    // explicitly enable multicast forwarding, hence assume it is enabled.
    //
    enabled = 1;
#endif // ! IPV6CTL_MFORWARDING

    if (enabled > 0)
	ret_value = true;
    else
	ret_value = false;
    
    return (XORP_OK);
}

/**
 * Set the IPv4 multicast forwarding engine to enable or disable forwarding
 * of packets.
 * 
 * @param v if true, then enable IPv4 multicast forwarding, otherwise
 * disable it.
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
MfeaMrouter::set_multicast_forwarding_enabled4(bool v, string& error_msg)
{
    // XXX: don't do anything if running in dummy mode
    if (mfea_node().is_dummy())
	return (XORP_OK);

    if (! have_multicast_routing4()) {
	if (! v) {
	    //
	    // XXX: we assume that "not supported" == "disable", hence
	    // return OK.
	    //
	    return (XORP_OK);
	}
	error_msg = c_format("Cannot set IPv4 multicast forwarding to %s: "
			     "IPv4 multicast routing is not supported",
			     bool_c_str(v));
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }

    int enable = (v) ? 1 : 0;
    bool old_value;

    UNUSED(enable);

    if (multicast_forwarding_enabled4(old_value, error_msg) != XORP_OK)
	return (XORP_ERROR);

    if (old_value == v)
	return (XORP_OK);	// Nothing changed

#if defined(CTL_NET) && defined(IPPROTO_IP) && defined(IPCTL_MFORWARDING)
    {
	size_t sz = sizeof(enable);
	int mib[4];

	mib[0] = CTL_NET;
	mib[1] = AF_INET;
	mib[2] = IPPROTO_IP;
	mib[3] = IPCTL_MFORWARDING;
	
	if (sysctl(mib, sizeof(mib) / sizeof(mib[0]), NULL, NULL, &enable, sz)
	    != 0) {
	    error_msg = c_format("Set sysctl(IPCTL_MFORWARDING) to %s failed: %s",
				 bool_c_str(v), strerror(errno));
	    XLOG_ERROR("%s", error_msg.c_str());
	    return (XORP_ERROR);
	}
    }

#else // ! IPCTL_MFORWARDING
    //
    // XXX: Not all systems have such additional control mechanism to
    // explicitly enable multicast forwarding, hence don't do anything
    //
#endif // ! IPCTL_MFORWARDING

    return (XORP_OK);
}

/**
 * Set the IPv6 multicast forwarding engine to enable or disable forwarding
 * of packets.
 * 
 * @param v if true, then enable IPv6 multicast forwarding, otherwise
 * disable it.
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
MfeaMrouter::set_multicast_forwarding_enabled6(bool v, string& error_msg)
{
    // XXX: don't do anything if running in dummy mode
    if (mfea_node().is_dummy())
	return (XORP_OK);

    if (! have_multicast_routing6()) {
	if (! v) {
	    //
	    // XXX: we assume that "not supported" == "disable", hence
	    // return OK.
	    //
	    return (XORP_OK);
	}
	error_msg = c_format("Cannot set IPv6 multicast forwarding to %s: "
			     "IPv6 multicast routing is not supported",
			     bool_c_str(v));
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }

    int enable = (v) ? 1 : 0;
    bool old_value;

    UNUSED(enable);

    if (multicast_forwarding_enabled6(old_value, error_msg) != XORP_OK)
	return (XORP_ERROR);

    if (old_value == v)
	return (XORP_OK);	// Nothing changed

#if defined(CTL_NET) && defined(IPPROTO_IPV6) && defined(IPV6CTL_MFORWARDING)
    {
	size_t sz = sizeof(enable);
	int mib[4];

	mib[0] = CTL_NET;
	mib[1] = AF_INET6;
	mib[2] = IPPROTO_IPV6;
	mib[3] = IPV6CTL_MFORWARDING;
	
	if (sysctl(mib, sizeof(mib) / sizeof(mib[0]), NULL, NULL, &enable, sz)
	    != 0) {
	    error_msg = c_format("Set sysctl(IPV6CTL_MFORWARDING) to %s failed: %s",
				 bool_c_str(v), strerror(errno));
	    XLOG_ERROR("%s", error_msg.c_str());
	    return (XORP_ERROR);
	}
    }

#else // ! IPV6CTL_MFORWARDING
    //
    // XXX: Not all systems have such additional control mechanism to
    // explicitly enable multicast forwarding, hence don't do anything
    //
#endif // ! IPV6CTL_MFORWARDING

    return (XORP_OK);
}

/**
 * MfeaMrouter::kernel_mrouter_ip_protocol:
 * @: 
 * 
 * Get the protocol that would be used in case of mrouter socket.
 * 
 * Return value: the protocol number on success, otherwise -1.
 **/
int
MfeaMrouter::kernel_mrouter_ip_protocol() const
{
    switch (family()) {
    case AF_INET:
	return (IPPROTO_IGMP);
#ifdef HAVE_IPV6
    case AF_INET6:
	return (IPPROTO_ICMPV6);
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
	return (-1);
    }
    
    return (-1);
}

/**
 * MfeaMrouter::start_mrt:
 * @: 
 * 
 * Start/enable the multicast routing in the kernel.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaMrouter::start_mrt()
{
    int mrouter_version = 1;	// XXX: hardcoded version
    string error_msg;
    bool do_mrt_init = true;

    UNUSED(mrouter_version);
    UNUSED(error_msg);
    
    switch (family()) {
    case AF_INET:
#ifndef HAVE_IPV4_MULTICAST_ROUTING
	UNUSED(do_mrt_init);
	XLOG_ERROR("start_mrt() failed: "
		   "IPv4 multicast routing not supported");
	return (XORP_ERROR);
#else
	if (set_multicast_forwarding_enabled4(true, error_msg) != XORP_OK) {
	    XLOG_ERROR("Cannot enable IPv4 multicast forwarding: %s",
		       error_msg.c_str());
	    return (XORP_ERROR);
	}

	new_mcast_tables_api = false;
#ifdef USE_MULT_MCAST_TABLES
	struct mrt_sockopt_simple tmp;
	memset(&tmp, 0, sizeof(tmp));
	tmp.table_id = getTableId();
	tmp.optval = 1; //version

	if (setsockopt(_mrouter_socket, IPPROTO_IP, MRT_INIT, &tmp, sizeof(tmp)) < 0) {
	    // Ok, doesn't use old API at least
	    uint32_t tbl = getTableId();
	    if (setsockopt(_mrouter_socket, IPPROTO_IP, MRT_TABLE, &tbl, sizeof(tbl)) < 0) {
		// Nor this
		supports_mcast_tables = false;
		XLOG_ERROR("MROUTE:  WARNING:  setsockopt(MRT_INIT) does not support multiple routing tables:: %s",
			   strerror(errno));
	    }
	    else {
		supports_mcast_tables = true;
		new_mcast_tables_api = true;
		XLOG_INFO("NOTE: MROUTE:  setsockopt(MRT_TABLE, %d) works!  Supports multiple"
			  " mcast routing tables.\n", tbl);
	    }
	}
	else {
	    supports_mcast_tables = true;
	    do_mrt_init = false;
	    XLOG_WARNING("NOTE:  MROUTE:  setsockopt(MRT_INIT) supports multiple routing tables!");
	    XLOG_WARNING("NOTE:  mroute ioctl struct sizes: mfcctl: %i mfcctl_ng: %i  mrt_sockopt_simple: %i"
		       "  sioc_sg_req: %i  sioc_sg_req_ng: %i  sioc_vif_req: %i  sioc_vif_req_ng: %i\n",
		       (int)(sizeof(struct mfcctl)), (int)(sizeof(struct mfcctl_ng)),
		       (int)(sizeof(struct mrt_sockopt_simple)),
		       (int)(sizeof(struct sioc_sg_req)), (int)(sizeof(struct sioc_sg_req_ng)),
		       (int)(sizeof(struct sioc_vif_req)), (int)(sizeof(struct sioc_vif_req_ng)));
	}
#endif

	if (do_mrt_init) {
	    if (setsockopt(_mrouter_socket, IPPROTO_IP, MRT_INIT,
			   (void *)&mrouter_version, sizeof(mrouter_version))
		< 0) {
		XLOG_ERROR("setsockopt(MRT_INIT, %u) failed: %s",
			   mrouter_version, strerror(errno));
		return (XORP_ERROR);
	    }
	}
#endif // HAVE_IPV4_MULTICAST_ROUTING
	break;

#ifdef HAVE_IPV6
    case AF_INET6:
    {
#ifndef HAVE_IPV6_MULTICAST_ROUTING
	XLOG_ERROR("start_mrt() failed: "
		   "IPv6 multicast routing not supported");
	return (XORP_ERROR);
#else
	if (set_multicast_forwarding_enabled6(true, error_msg) != XORP_OK) {
	    XLOG_ERROR("Cannot enable IPv6 multicast forwarding: %s",
		       error_msg.c_str());
	    return (XORP_ERROR);
	}

#ifdef USE_MULT_MCAST_TABLES
	uint32_t tbl = getTableId();
	if (setsockopt(_mrouter_socket, SOL_IPV6, MRT6_TABLE, &tbl, sizeof(tbl)) < 0) {
	    XLOG_ERROR("MROUTE:  WARNING:  setsockopt(MRT6_TABLE, %d) does not support"
		       " multiple routing tables:: %s",
		       tbl, strerror(errno));
	}
	else {
	    XLOG_INFO("NOTE: MROUTE:  setsockopt(MRT6_TABLE, %d) works!  Supports"
		      " multiple mcast-6 routing tables.\n", tbl);
	}
#endif

	if (setsockopt(_mrouter_socket, IPPROTO_IPV6, MRT6_INIT,
		       (void *)&mrouter_version, sizeof(mrouter_version))
	    < 0) {
	    XLOG_ERROR("setsockopt(MRT6_INIT, %u) failed: %s",
		       mrouter_version, strerror(errno));
	    return (XORP_ERROR);
	}

#endif // HAVE_IPV6_MULTICAST_ROUTING
    }
    break;
#endif // HAVE_IPV6

    default:
	XLOG_UNREACHABLE();
	return (XORP_ERROR);
    }
    
    //
    // Configure advanced multicast API
    //
#if defined(MRT_API_CONFIG) && defined(ENABLE_ADVANCED_MULTICAST_API)
    if (family() == AF_INET) {
#ifndef HAVE_IPV4_MULTICAST_ROUTING
	XLOG_ERROR("start_mrt() failed: "
		   "IPv4 multicast routing not supported");
	return (XORP_ERROR);
#else
	uint32_t mrt_api = 0;
	
	//
	// Set the desired API
	//
#ifdef MRT_MFC_FLAGS_DISABLE_WRONGVIF
	// Try to enable the support for disabling WRONGVIF signals per vif
	mrt_api |= MRT_MFC_FLAGS_DISABLE_WRONGVIF;
#endif
#ifdef MRT_MFC_FLAGS_BORDER_VIF
	// Try to enable the border bit flag (per MFC per vif)
	mrt_api |= MRT_MFC_FLAGS_BORDER_VIF;
#endif
#ifdef MRT_MFC_RP
	// Try to enable kernel-level PIM Register encapsulation
	mrt_api |= MRT_MFC_RP;
#endif
#ifdef MRT_MFC_BW_UPCALL
	// Try to enable bandwidth-related upcalls from the kernel
	mrt_api |= MRT_MFC_BW_UPCALL;
#endif
	
	//
	// Try to configure the kernel with the desired API
	//
	if (setsockopt(_mrouter_socket, IPPROTO_IP, MRT_API_CONFIG,
		       (void *)&mrt_api, sizeof(mrt_api)) < 0) {
	    XLOG_ERROR("setsockopt(MRT_API_CONFIG) failed: %s",
		       strerror(errno));
	    return (XORP_ERROR);
	}
	
	//
	// Test which of the desired API support succeded
	//
#ifdef MRT_MFC_FLAGS_DISABLE_WRONGVIF
	// Test the support for disabling WRONGVIF signals per vif
	if (mrt_api & MRT_MFC_FLAGS_DISABLE_WRONGVIF)
	    _mrt_api_mrt_mfc_flags_disable_wrongvif = true;
#endif
#ifdef MRT_MFC_FLAGS_BORDER_VIF
	// Test the support for the border bit flag (per MFC per vif)
	if (mrt_api & MRT_MFC_FLAGS_BORDER_VIF)
	    _mrt_api_mrt_mfc_flags_border_vif = true;
#endif
#ifdef MRT_MFC_RP
	// Test the support for kernel-level PIM Register encapsulation
	if (mrt_api & MRT_MFC_RP)
	    _mrt_api_mrt_mfc_rp = true;
#endif
#ifdef MRT_MFC_BW_UPCALL
	// Test the support for bandwidth-related upcalls from the kernel
	if (mrt_api & MRT_MFC_BW_UPCALL)
	    _mrt_api_mrt_mfc_bw_upcall = true;
#endif

#endif // HAVE_IPV4_MULTICAST_ROUTING	
	
    }
#endif // MRT_API_CONFIG && ENABLE_ADVANCED_MULTICAST_API
    
#if defined(MRT6_API_CONFIG) && defined(ENABLE_ADVANCED_MULTICAST_API)
#ifdef HAVE_IPV6
    if (family == AF_INET6) {
#ifndef HAVE_IPV6_MULTICAST_ROUTING
	XLOG_ERROR("start_mrt() failed: "
		   "IPv6 multicast routing not supported");
	return (XORP_ERROR);
#else
	
	uint32_t mrt_api = 0;
	
	//
	// Set the desired API
	//
#ifdef MRT6_MFC_FLAGS_DISABLE_WRONGVIF
	// Try to enable the support for disabling WRONGVIF signals per vif
	mrt_api |= MRT6_MFC_FLAGS_DISABLE_WRONGVIF;
#endif
#ifdef MRT6_MFC_FLAGS_BORDER_VIF
	// Try to enable the border bit flag (per MFC per vif)
	mrt_api |= MRT6_MFC_FLAGS_BORDER_VIF;
#endif
#ifdef MRT6_MFC_RP
	// Try to enable kernel-level PIM Register encapsulation
	mrt_api |= MRT6_MFC_RP;
#endif
#ifdef MRT6_MFC_BW_UPCALL
	// Try to enable bandwidth-related upcalls from the kernel
	mrt_api |= MRT6_MFC_BW_UPCALL;
#endif
	
	//
	// Try to configure the kernel with the desired API
	//
	if (setsockopt(_mrouter_socket, IPPROTO_IP, MRT6_API_CONFIG,
		       (void *)&mrt_api, sizeof(mrt_api)) < 0) {
	    XLOG_ERROR("setsockopt(MRT6_API_CONFIG) failed: %s",
		       strerror(errno));
	    return (XORP_ERROR);
	}
	
	//
	// Test which of the desired API support succeded
	//
#ifdef MRT6_MFC_FLAGS_DISABLE_WRONGVIF
	// Test the support for disabling WRONGVIF signals per vif
	if (mrt_api & MRT6_MFC_FLAGS_DISABLE_WRONGVIF)
	    _mrt_api_mrt_mfc_flags_disable_wrongvif = true;
#endif
#ifdef MRT6_MFC_FLAGS_BORDER_VIF
	// Test the support for the border bit flag (per MFC per vif)
	if (mrt_api & MRT6_MFC_FLAGS_BORDER_VIF)
	    _mrt_api_mrt_mfc_flags_border_vif = true;
#endif
#ifdef MRT6_MFC_RP
	// Test the support for kernel-level PIM Register encapsulation
	if (mrt_api & MRT6_MFC_RP)
	    _mrt_api_mrt_mfc_rp = true;
#endif
#ifdef MRT6_MFC_BW_UPCALL
	// Test the support for bandwidth-related upcalls from the kernel
	if (mrt_api & MRT6_MFC_BW_UPCALL)
	    _mrt_api_mrt_mfc_bw_upcall = true;
#endif
	
#endif // HAVE_IPV6_MULTICAST_ROUTING	
    }
#endif // HAVE_IPV6
#endif // MRT6_API_CONFIG && ENABLE_ADVANCED_MULTICAST_API
    
    return (XORP_OK);
}


/**
 * MfeaMrouter::stop_mrt:
 * @: 
 * 
 * Stop/disable the multicast routing in the kernel.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaMrouter::stop_mrt()
{
    string error_msg;

    UNUSED(error_msg);

    _mrt_api_mrt_mfc_flags_disable_wrongvif = false;
    _mrt_api_mrt_mfc_flags_border_vif = false;
    _mrt_api_mrt_mfc_rp = false;
    _mrt_api_mrt_mfc_bw_upcall = false;
    
    if (!_mrouter_socket.is_valid())
	return (XORP_ERROR);
    
    size_t sz = 0;
    void* o = NULL;

    switch (family()) {
    case AF_INET:
#ifndef HAVE_IPV4_MULTICAST_ROUTING
	UNUSED(sz);
	UNUSED(o);
	XLOG_ERROR("stop_mrt() failed: "
		   "IPv4 multicast routing not supported");
	return (XORP_ERROR);
#else
	if (set_multicast_forwarding_enabled4(false, error_msg) != XORP_OK) {
	    XLOG_ERROR("Cannot disable IPv4 multicast forwarding: %s",
		       error_msg.c_str());
	    return (XORP_ERROR);
	}

#ifdef USE_MULT_MCAST_TABLES
	struct mrt_sockopt_simple tmp;
	memset(&tmp, 0, sizeof(tmp));
	tmp.table_id = getTableId();
	tmp.optval = 1; //version
	sz = sizeof(tmp);
	o = &tmp;
	if (new_mcast_tables_api || !supports_mcast_tables) {
	    sz = 0;
	    o = NULL;
	}
#endif

	if (setsockopt(_mrouter_socket, IPPROTO_IP, MRT_DONE, o, sz) < 0) {
	    XLOG_ERROR("setsockopt(MRT_DONE) failed: %s", strerror(errno));
	    return (XORP_ERROR);
	}
#endif // HAVE_IPV4_MULTICAST_ROUTING
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
    {
#ifndef HAVE_IPV6_MULTICAST_ROUTING
	XLOG_ERROR("stop_mrt() failed: "
		   "IPv6 multicast routing not supported");
	return (XORP_ERROR);
#else
	if (set_multicast_forwarding_enabled6(false, error_msg) != XORP_OK) {
	    XLOG_ERROR("Cannot disable IPv6 multicast forwarding: %s",
		       error_msg.c_str());
	    return (XORP_ERROR);
	}
	if (setsockopt(_mrouter_socket, IPPROTO_IPV6, MRT6_DONE, NULL, 0)
	    < 0) {
	    XLOG_ERROR("setsockopt(MRT6_DONE) failed: %s", strerror(errno));
	    return (XORP_ERROR);
	}
#endif // HAVE_IPV6_MULTICAST_ROUTING
    }
    break;
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

int
MfeaMrouter::start_pim(string& error_msg)
{
    int v = 1;
    size_t sz = 0;
    void* o = NULL;

    switch (family()) {
    case AF_INET:
#ifndef HAVE_IPV4_MULTICAST_ROUTING
	UNUSED(sz);
	UNUSED(o);
	error_msg = c_format("start_pim() failed: "
			     "IPv4 multicast routing not supported");
	return (XORP_ERROR);
#else
#ifdef USE_MULT_MCAST_TABLES
	struct mrt_sockopt_simple tmp;
	memset(&tmp, 0, sizeof(tmp));
	tmp.table_id = getTableId();
	tmp.optval = 1; //pim
	sz = sizeof(tmp);
	o = &tmp;
	if (new_mcast_tables_api || !supports_mcast_tables) {
	    sz = sizeof(v);
	    o = &v;
	}
#else
	sz = sizeof(v);
	o = &v;
#endif

	if (setsockopt(_mrouter_socket, IPPROTO_IP, MRT_PIM, o, sz) < 0) {
	    error_msg = c_format("setsockopt(MRT_PIM, %u) failed: %s",
				 v, strerror(errno));
	    return (XORP_ERROR);
	}
#endif // HAVE_IPV4_MULTICAST_ROUTING
	break;

#ifdef HAVE_IPV6
    case AF_INET6:
    {
#ifndef HAVE_IPV6_MULTICAST_ROUTING
	error_msg = c_format("start_pim() failed: "
			     "IPv6 multicast routing not supported");
	return (XORP_ERROR);
#else
	if (setsockopt(_mrouter_socket, IPPROTO_IPV6, MRT6_PIM,
		       (void *)&v, sizeof(v)) < 0) {
	    error_msg = c_format("setsockopt(MRT6_PIM, %u) failed: %s",
				 v, strerror(errno));
	    return (XORP_ERROR);
	}
#endif // HAVE_IPV6_MULTICAST_ROUTING
    }
    break;
#endif // HAVE_IPV6

    default:
	UNUSED(v);
	XLOG_UNREACHABLE();
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
MfeaMrouter::stop_pim(string& error_msg)
{
    int v = 0;
    size_t sz = 0;
    void* o = NULL;

    if (!_mrouter_socket.is_valid())
	return (XORP_ERROR);
    
    switch (family()) {
    case AF_INET:
#ifndef HAVE_IPV4_MULTICAST_ROUTING
	UNUSED(sz);
	UNUSED(o);
	error_msg = c_format("stop_pim() failed: "
			     "IPv4 multicast routing not supported");
	return (XORP_ERROR);
#else
#ifdef USE_MULT_MCAST_TABLES
	struct mrt_sockopt_simple tmp;
	memset(&tmp, 0, sizeof(tmp));
	tmp.table_id = getTableId();
	tmp.optval = 0; //pim
	sz = sizeof(tmp);
	o = &tmp;
	if (new_mcast_tables_api || !supports_mcast_tables) {
	    sz = sizeof(v);
	    o = &v;
	}
#else
	sz = sizeof(v);
	o = &v;
#endif

	if (setsockopt(_mrouter_socket, IPPROTO_IP, MRT_PIM, o, sz) < 0) {
	    error_msg = c_format("setsockopt(MRT_PIM, %u) failed: %s",
				 v, strerror(errno));
	    return (XORP_ERROR);
	}
#endif
	break;

#ifdef HAVE_IPV6
    case AF_INET6:
    {
#ifndef HAVE_IPV6_MULTICAST_ROUTING
	error_msg = c_format("stop_pim() failed: "
			     "IPv6 multicast routing not supported");
	return (XORP_ERROR);
#else
	v = 0;
	if (setsockopt(_mrouter_socket, IPPROTO_IPV6, MRT6_PIM,
		       (void *)&v, sizeof(v)) < 0) {
	    error_msg = c_format("setsockopt(MRT6_PIM, %u) failed: %s",
				 v, strerror(errno));
	    return (XORP_ERROR);
	}
#endif // HAVE_IPV6_MULTICAST_ROUTING
    }
    break;
#endif // HAVE_IPV6

    default:
	UNUSED(v);
	XLOG_UNREACHABLE();
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

/**
 * MfeaMrouter::add_multicast_vif:
 * @vif_index: The vif index of the virtual interface to add.
 * 
 * Add a virtual multicast interface to the kernel.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaMrouter::add_multicast_vif(uint32_t vif_index, string& error_msg)
{
    MfeaVif *mfea_vif = mfea_node().vif_find_by_vif_index(vif_index);
    
    if (mfea_vif == NULL) {
	error_msg = c_format("Could not find vif: %i\n", vif_index);
	return XORP_ERROR;
    }

    void* sopt_arg = NULL;
    size_t sz = 0;
    
    switch (family()) {
    case AF_INET:
    {
#ifndef HAVE_IPV4_MULTICAST_ROUTING
	UNUSED(sz);
	UNUSED(sopt_arg);
	error_msg = "add_multicast_vif() failed: IPv4 multicast routing not supported";
	return XORP_ERROR;
#else
#ifdef USE_MULT_MCAST_TABLES
	struct vifctl_ng vc_ng __attribute__((aligned(64)));
	// work around invalid g++ 9.1.1 warning about taking addr of packed element
	unsigned char* tmpp = (unsigned char*)(&(vc_ng.vif));
	struct vifctl* vcp = (struct vifctl*)(tmpp);
	memset(&vc_ng, 0, sizeof(vc_ng));
	sopt_arg = &vc_ng;
	sz = sizeof(vc_ng);
	vc_ng.table_id = getTableId();
	if (new_mcast_tables_api || !supports_mcast_tables) {
	    sopt_arg = &(vc_ng.vif);
	    sz = sizeof(vc_ng.vif);
	}
#else
	struct vifctl vc;
	struct vifctl* vcp = &vc;
	memset(&vc, 0, sizeof(vc));
	sopt_arg = &vc;
	sz = sizeof(vc);
#endif

	vcp->vifc_vifi = mfea_vif->vif_index();
	// XXX: we don't (need to) support VIFF_TUNNEL; VIFF_SRCRT is obsolete
	vcp->vifc_flags = 0;
	if (mfea_vif->is_pim_register())
	    vcp->vifc_flags	|= VIFF_REGISTER;
	    
	vcp->vifc_threshold	= mfea_vif->min_ttl_threshold();
	vcp->vifc_rate_limit	= mfea_vif->max_rate_limit();

#ifdef XORP_USE_VIFF_USE_IFINDEX
	// NOTE:  This will break on un-patched kernels earlier than
	// 2.6.31 or so.  See:  http://patchwork.ozlabs.org/patch/33723/
	// You have to enable this with "scons enable_viff_use_ifindex=true"
	// since I see no way to auto-detect if a kernel supports this feature
	// or not. --Ben
	if (! mfea_vif->is_pim_register()) {
	    vcp->vifc_flags = VIFF_USE_IFINDEX;
	    vcp->vifc_lcl_ifindex = mfea_vif->pif_index();
	} else
#endif
	{
	    if (mfea_vif->addr_ptr() == NULL) {
		error_msg = c_format("add_multicast_vif() by-addr failed: vif %s has no address",
				     mfea_vif->name().c_str());
		return XORP_ERROR;
	    }
	    mfea_vif->addr_ptr()->copy_out(vcp->vifc_lcl_addr);
	}

	//
	// XXX: no need to copy any remote address to vc.vifc_rmt_addr,
	// because we don't (need to) support IPIP tunnels.
	//
	if (setsockopt(_mrouter_socket, IPPROTO_IP, MRT_ADD_VIF,
		       sopt_arg, sz) < 0) {
	    error_msg = c_format("setsockopt(%d, MRT_ADD_VIF, vif %s) failed: %s  sz: %i, ifindex: %i addr: %s",
				 _mrouter_socket.getSocket(), mfea_vif->name().c_str(), XSTRERROR,
				 (int)(sz), mfea_vif->pif_index(),
				 mfea_vif->addr_ptr() ? mfea_vif->addr_ptr()->str().c_str() : "NULL");
	    return XORP_ERROR;
	}
#endif // HAVE_IPV4_MULTICAST_ROUTING
    }
    break;
    
#ifdef HAVE_IPV6
    case AF_INET6:
    {
#ifndef HAVE_IPV6_MULTICAST_ROUTING
	error_msg = "add_multicast_vif() failed: IPv6 multicast routing not supported";
	return XORP_ERROR;
#else
	struct mif6ctl mc;
	
	memset(&mc, 0, sizeof(mc));
	mc.mif6c_mifi = mfea_vif->vif_index();
	mc.mif6c_flags = 0;
	if (mfea_vif->is_pim_register())
	    mc.mif6c_flags |= MIFF_REGISTER;
 	mc.mif6c_pifi = mfea_vif->pif_index();
#ifdef HAVE_STRUCT_MIF6CTL_VIFC_THRESHOLD /* BSD does not, it seems, newer Linux does */
	mc.vifc_threshold = mfea_vif->min_ttl_threshold();
	mc.vifc_rate_limit = mfea_vif->max_rate_limit();
#endif
	if (setsockopt(_mrouter_socket, IPPROTO_IPV6, MRT6_ADD_MIF,
		       (void *)&mc, sizeof(mc)) < 0) {
	    error_msg = c_format("setsockopt(%i, MRT6_ADD_MIF, vif %s) failed: %s  mifi: %i  flags: 0x%x pifi: %i",
				 _mrouter_socket.getSocket(),
				 mfea_vif->name().c_str(), strerror(errno),
				 (int)(mc.mif6c_mifi), (int)(mc.mif6c_flags), (int)(mc.mif6c_pifi));
	    return XORP_ERROR;
	}
#endif // HAVE_IPV6_MULTICAST_ROUTING
    }
    break;
#endif // HAVE_IPV6
    
    default:
	XLOG_UNREACHABLE();
	return XORP_ERROR;
    }
    
    return XORP_OK;
}


/**
 * MfeaMrouter::delete_multicast_vif:
 * @vif_index: The vif index of the interface to delete.
 * 
 * Delete a virtual multicast interface from the kernel.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaMrouter::delete_multicast_vif(uint32_t vif_index)
{
    MfeaVif *mfea_vif = mfea_node().vif_find_by_vif_index(vif_index);
    
    if (mfea_vif == NULL) {
	XLOG_WARNING("Could not find mfea-vif for index: %i\n",
		     vif_index);
	return (XORP_ERROR);
    }
    
    switch (family()) {
    case AF_INET:
    {
#ifndef HAVE_IPV4_MULTICAST_ROUTING
	XLOG_ERROR("delete_multicast_vif() failed: "
		   "IPv4 multicast routing not supported");
	return (XORP_ERROR);
#else
	int ret_value = -1;

	//
	// XXX: In case of Linux, MRT_DEL_VIF expects an argument
	// of type "struct vifctl", while other systems expect
	// an argument of type "vifi_t".
	//
#ifdef HOST_OS_LINUX
#ifdef USE_MULT_MCAST_TABLES
	struct vifctl_ng vc_ng __attribute__((aligned(64)));
	// work around invalid g++ 9.1.1 warning about taking addr of packed element
	unsigned char* tmpp = (unsigned char*)(&(vc_ng.vif));
	struct vifctl* vcp = (struct vifctl*)(tmpp);
	memset(&vc_ng, 0, sizeof(vc_ng));
	void* sopt_arg = &vc_ng;
	size_t sz = sizeof(vc_ng);
	vc_ng.table_id = getTableId();
	if (new_mcast_tables_api || !supports_mcast_tables) {
	    sopt_arg = &(vc_ng.vif);
	    sz = sizeof(vc_ng.vif);
	}
#else
	struct vifctl vc;
	struct vifctl* vcp = &vc;
	memset(&vc, 0, sizeof(vc));
	void* sopt_arg = &vc;
	size_t sz = sizeof(vc);
#endif
	vcp->vifc_vifi = mfea_vif->vif_index();
	ret_value = setsockopt(_mrouter_socket, IPPROTO_IP, MRT_DEL_VIF,
			       sopt_arg, sz);
#else
	vifi_t vifi = mfea_vif->vif_index();
	ret_value = setsockopt(_mrouter_socket, IPPROTO_IP, MRT_DEL_VIF,
			       (void *)&vifi, sizeof(vifi));
#endif

	if (ret_value < 0) {
	    XLOG_WARNING("setsockopt(MRT_DEL_VIF, %s (%i)) failed: %s",
			 mfea_vif->name().c_str(), mfea_vif->vif_index(),
			 XSTRERROR);
	    return XORP_ERROR;
	}
#endif // HAVE_IPV4_MULTICAST_ROUTING
    }
    break;
    
#ifdef HAVE_IPV6
    case AF_INET6:
    {
#ifndef HAVE_IPV6_MULTICAST_ROUTING
	XLOG_ERROR("delete_multicast_vif() failed: "
		   "IPv6 multicast routing not supported");
	return XORP_ERROR;
#else
	int ret_value = -1;

	mifi_t vifi = mfea_vif->vif_index();
	ret_value = setsockopt(_mrouter_socket, IPPROTO_IPV6, MRT6_DEL_MIF,
			       (void *)&vifi, sizeof(vifi));
	if (ret_value < 0) {
	    XLOG_WARNING("setsockopt(MRT6_DEL_MIF, %s (%i)) failed: %s",
			 mfea_vif->name().c_str(), mfea_vif->vif_index(),
			 XSTRERROR);
	    return XORP_ERROR;
	}
#endif // HAVE_IPV6_MULTICAST_ROUTING
    }
    break;
#endif // HAVE_IPV6
    
    default:
	XLOG_UNREACHABLE();
	return XORP_ERROR;
    }
    
    return XORP_OK;
}


/**
 * MfeaMrouter::add_mfc:
 * @source: The MFC source address.
 * @group: The MFC group address.
 * @iif_vif_index: The MFC incoming interface index.
 * @oifs_ttl: An array with the min. TTL a packet should have to be forwarded.
 * @oifs_flags: An array with misc. flags for the MFC to install.
 * Note that those flags are supported only by the advanced multicast API.
 * @rp_addr: The RP address.
 * 
 * Install/modify a Multicast Forwarding Cache (MFC) entry in the kernel.
 * If the MFC entry specified by (source, group) pair was not
 * installed before, a new MFC entry will be created in the kernel;
 * otherwise, the existing entry's fields will be modified.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaMrouter::add_mfc(const IPvX& source, const IPvX& group,
		     uint32_t iif_vif_index, uint8_t *oifs_ttl,
		     uint8_t *oifs_flags,
		     const IPvX& rp_addr)
{

    //XLOG_ERROR("MfeaMrouter::add_mfc, source: %s group: %s iif_vif_index: %i  rp_addr: %s\n",
    //       source.str().c_str(), group.str().c_str(), iif_vif_index,
    //       rp_addr.str().c_str());

    if (iif_vif_index >= mfea_node().maxvifs())
	return (XORP_ERROR);
    
    oifs_ttl[iif_vif_index] = 0;		// Pre-caution

#if !defined(HAVE_IPV4_MULTICAST_ROUTING) && !defined(HAVE_IPV6_MULTICAST_ROUTING)
    UNUSED(source);
    UNUSED(group);
#endif

    UNUSED(oifs_flags);
    UNUSED(rp_addr);

    if (mfea_node().is_log_trace()) {
	string res;
	for (uint32_t i = 0; i < mfea_node().maxvifs(); i++) {
	    if (oifs_ttl[i] > 0)
		res += "O";
	    else
		res += ".";
	}
	XLOG_TRACE(mfea_node().is_log_trace(),
		   "Add MFC entry: (%s, %s) iif = %d olist = %s",
		   cstring(source),
		   cstring(group),
		   iif_vif_index,
		   res.c_str());
    }

    switch (family()) {
    case AF_INET:
    {
#ifndef HAVE_IPV4_MULTICAST_ROUTING
	XLOG_ERROR("add_mfc() failed: "
		   "IPv4 multicast routing not supported");
	return (XORP_ERROR);
#else

#ifdef USE_MULT_MCAST_TABLES
	struct mfcctl_ng mc_ng __attribute__((aligned(64)));
	// work around invalid g++ 9.1.1 warning about taking addr of packed element
	unsigned char* tmpp = (unsigned char*)(&(mc_ng.mfc));
	struct mfcctl* mcp = (struct mfcctl*)(tmpp);
	memset(&mc_ng, 0, sizeof(mc_ng));
	void* sopt_arg = &mc_ng;
	size_t sz = sizeof(mc_ng);
	mc_ng.table_id = getTableId();
	if (new_mcast_tables_api || !supports_mcast_tables) {
	    sopt_arg = &(mc_ng.mfc);
	    sz = sizeof(mc_ng.mfc);
	}
#else
#if defined(HAVE_STRUCT_MFCCTL2) && defined(ENABLE_ADVANCED_MULTICAST_API)
	struct mfcctl2 mc;
	struct mfcctl2* mcp = &mc;
#else
	struct mfcctl mc;
	struct mfcctl* mcp = &mc;
#endif
	void* sopt_arg = mcp;
	size_t sz = sizeof(mc);
	memset(&mc, 0, sizeof(mc));
#endif
	
	source.copy_out(mcp->mfcc_origin);
	group.copy_out(mcp->mfcc_mcastgrp);
	mcp->mfcc_parent = iif_vif_index;
	for (uint32_t i = 0; i < mfea_node().maxvifs(); i++) {
	    mcp->mfcc_ttls[i] = oifs_ttl[i];
#if defined(HAVE_STRUCT_MFCCTL2_MFCC_FLAGS) && defined(ENABLE_ADVANCED_MULTICAST_API)
	    mcp->mfcc_flags[i] = oifs_flags[i];
#endif
	}
#if defined(HAVE_STRUCT_MFCCTL2_MFCC_RP) && defined(ENABLE_ADVANCED_MULTICAST_API)
	if (_mrt_api_mrt_mfc_rp)
	    rp_addr.copy_out(mcp->mfcc_rp);
#endif
	
	if (setsockopt(_mrouter_socket, IPPROTO_IP, MRT_ADD_MFC,
		       sopt_arg, sz) < 0) {
	    XLOG_ERROR("setsockopt(MRT_ADD_MFC, (%s, %s)) failed: %s",
		       cstring(source), cstring(group), strerror(errno));
	    return (XORP_ERROR);
	}
#endif // HAVE_IPV4_MULTICAST_ROUTING
    }
    break;
    
#ifdef HAVE_IPV6
    case AF_INET6:
    {
#ifndef HAVE_IPV6_MULTICAST_ROUTING
	XLOG_ERROR("add_mfc() failed: "
		   "IPv6 multicast routing not supported");
	return (XORP_ERROR);
#else
	
#if defined(HAVE_STRUCT_MF6CCTL2) && defined(ENABLE_ADVANCED_MULTICAST_API)
	struct mf6cctl2 mc;
#else
	struct mf6cctl mc;
#endif
	
	memset(&mc, 0, sizeof(mc));
	IF_ZERO(&mc.mf6cc_ifset);	
	source.copy_out(mc.mf6cc_origin);
	group.copy_out(mc.mf6cc_mcastgrp);
	mc.mf6cc_parent = iif_vif_index;
	for (uint32_t i = 0; i < mfea_node().maxvifs(); i++) {
	    if (oifs_ttl[i] > 0)
		IF_SET(i, &mc.mf6cc_ifset);
#if defined(HAVE_STRUCT_MF6CCTL2_MF6CC_FLAGS) && defined(ENABLE_ADVANCED_MULTICAST_API)
	    mc.mf6cc_flags[i] = oifs_flags[i];
#endif
	}
#if defined(HAVE_STRUCT_MF6CCTL2_MF6CC_RP) && defined(ENABLE_ADVANCED_MULTICAST_API)
	if (_mrt_api_mrt_mfc_rp)
	    rp_addr.copy_out(mc.mf6cc_rp);
#endif
	
	if (setsockopt(_mrouter_socket, IPPROTO_IPV6, MRT6_ADD_MFC,
		       (void *)&mc, sizeof(mc)) < 0) {
	    XLOG_ERROR("setsockopt(MRT6_ADD_MFC, (%s, %s)) failed: %s",
		       cstring(source), cstring(group), strerror(errno));
	    return (XORP_ERROR);
	}
#endif // HAVE_IPV6_MULTICAST_ROUTING
    }
    break;
#endif // HAVE_IPV6
    
    default:
	XLOG_UNREACHABLE();
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}


/**
 * MfeaMrouter::delete_mfc:
 * @source: The MFC source address.
 * @group: The MFC group address.
 * 
 * Delete a Multicast Forwarding Cache (MFC) entry in the kernel.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaMrouter::delete_mfc(const IPvX& source, const IPvX& group)
{
#if !defined(HAVE_IPV4_MULTICAST_ROUTING) && !defined(HAVE_IPV6_MULTICAST_ROUTING)
    UNUSED(source);
    UNUSED(group);
#endif

    XLOG_TRACE(mfea_node().is_log_trace(),
	       "Delete MFC entry: (%s, %s)",
	       cstring(source),
	       cstring(group));

    switch (family()) {
    case AF_INET:
    {
#ifndef HAVE_IPV4_MULTICAST_ROUTING
	XLOG_ERROR("delete_mfc() failed: "
		   "IPv4 multicast routing not supported");
	return (XORP_ERROR);
#else
#ifdef USE_MULT_MCAST_TABLES
	struct mfcctl_ng mc_ng __attribute__((aligned(64)));
	// work around invalid g++ 9.1.1 warning about taking addr of packed element
	unsigned char* tmpp = (unsigned char*)(&(mc_ng.mfc));
	struct mfcctl* mcp = (struct mfcctl*)(tmpp);
	memset(&mc_ng, 0, sizeof(mc_ng));
	void* sopt_arg = &mc_ng;
	size_t sz = sizeof(mc_ng);
	mc_ng.table_id = getTableId();
	if (new_mcast_tables_api || !supports_mcast_tables) {
	    sopt_arg = &(mc_ng.mfc);
	    sz = sizeof(mc_ng.mfc);
	}
#else
	struct mfcctl mc;
	struct mfcctl* mcp = &mc;
	void* sopt_arg = &mc;
	size_t sz = sizeof(mc);
#endif

	source.copy_out(mcp->mfcc_origin);
	group.copy_out(mcp->mfcc_mcastgrp);
	
	if (setsockopt(_mrouter_socket, IPPROTO_IP, MRT_DEL_MFC,
		       sopt_arg, sz) < 0) {
	    XLOG_ERROR("setsockopt(MRT_DEL_MFC, (%s, %s)) failed: %s",
		       cstring(source), cstring(group), strerror(errno));
	    return (XORP_ERROR);
	}
#endif // HAVE_IPV4_MULTICAST_ROUTING
    }
    break;
    
#ifdef HAVE_IPV6
    case AF_INET6:
    {
#ifndef HAVE_IPV6_MULTICAST_ROUTING
	XLOG_ERROR("delete_mfc() failed: "
		   "IPv6 multicast routing not supported");
	return (XORP_ERROR);
#else
	
	struct mf6cctl mc;

	source.copy_out(mc.mf6cc_origin);
	group.copy_out(mc.mf6cc_mcastgrp);
	
	if (setsockopt(_mrouter_socket, IPPROTO_IPV6, MRT6_DEL_MFC,
		       (void *)&mc, sizeof(mc)) < 0) {
	    XLOG_ERROR("setsockopt(MRT6_DEL_MFC, (%s, %s)) failed: %s",
		       cstring(source), cstring(group), strerror(errno));
	    return (XORP_ERROR);
	}
#endif // HAVE_IPV6_MULTICAST_ROUTING
    }
    break;
#endif // HAVE_IPV6
    
    default:
	XLOG_UNREACHABLE();
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

/**
 * MfeaMrouter::add_bw_upcall:
 * @source: The source address.
 * @group: The group address.
 * @threshold_interval: The dataflow threshold interval.
 * @threshold_packets: The threshold (in number of packets) to compare against.
 * @threshold_bytes: The threshold (in number of bytes) to compare against.
 * @is_threshold_in_packets: If true, @threshold_packets is valid.
 * @is_threshold_in_bytes: If true, @threshold_bytes is valid.
 * @is_geq_upcall: If true, the operation for comparison is ">=".
 * @is_leq_upcall: If true, the operation for comparison is "<=".
 * @error_msg: The error message (if error).
 * 
 * Add a dataflow monitor entry in the kernel.
 * Note: either @is_threshold_in_packets or @is_threshold_in_bytes (or both)
 * must be true.
 * Note: either @is_geq_upcall or @is_leq_upcall (but not both) must be true.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaMrouter::add_bw_upcall(const IPvX& source, const IPvX& group,
			   const TimeVal& threshold_interval,
			   uint32_t threshold_packets,
			   uint32_t threshold_bytes,
			   bool is_threshold_in_packets,
			   bool is_threshold_in_bytes,
			   bool is_geq_upcall,
			   bool is_leq_upcall,
			   string& error_msg)
{
    XLOG_TRACE(mfea_node().is_log_trace(),
	       "Add dataflow monitor: "
	       "source = %s group = %s "
	       "threshold_interval_sec = %d threshold_interval_usec = %d "
	       "threshold_packets = %d threshold_bytes = %d "
	       "is_threshold_in_packets = %d is_threshold_in_bytes = %d "
	       "is_geq_upcall = %d is_leq_upcall = %d",
	       cstring(source), cstring(group),
	       threshold_interval.sec(), threshold_interval.usec(),
	       threshold_packets, threshold_bytes,
	       is_threshold_in_packets, is_threshold_in_bytes,
	       is_geq_upcall, is_leq_upcall);

#if ! (defined(ENABLE_ADVANCED_MULTICAST_API) && defined(MRT_ADD_BW_UPCALL))
    UNUSED(threshold_interval);
    UNUSED(threshold_packets);
    UNUSED(threshold_bytes);
    UNUSED(is_threshold_in_packets);
    UNUSED(is_threshold_in_bytes);
    UNUSED(is_geq_upcall);
    UNUSED(is_leq_upcall);
#endif

    //
    // Check if the kernel supports the bandwidth-upcall mechanism.
    //
    if (! mrt_api_mrt_mfc_bw_upcall()) {
	error_msg = c_format("add_bw_upcall(%s, %s) failed: "
			     "dataflow monitor entry in the kernel "
			     "is not supported",
			     cstring(source), cstring(group));
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }

    // XXX: flags is_geq_upcall and is_leq_upcall are mutually exclusive
    if (! (is_geq_upcall ^ is_leq_upcall)) {
	error_msg = c_format("Cannot add dataflow monitor for (%s, %s): "
			     "the GEQ and LEQ flags are mutually exclusive "
			     "(GEQ = %s; LEQ = %s)",
			     cstring(source), cstring(group),
			     bool_c_str(is_geq_upcall),
			     bool_c_str(is_leq_upcall));
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);		// Invalid arguments
    }
    // XXX: at least one of the threshold flags must be set
    if (! (is_threshold_in_packets || is_threshold_in_bytes)) {
	error_msg = c_format("Cannot add dataflow monitor for (%s, %s): "
			     "invalid threshold flags "
			     "(is_threshold_in_packets = %s; "
			     "is_threshold_in_bytes = %s)",
			     cstring(source), cstring(group),
			     bool_c_str(is_threshold_in_packets),
			     bool_c_str(is_threshold_in_bytes));
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);		// Invalid arguments
    }
    
    //
    // Do the job
    //
    switch (family()) {
    case AF_INET:
    {
#if defined(MRT_ADD_BW_UPCALL) && defined(ENABLE_ADVANCED_MULTICAST_API)
	struct bw_upcall bw_upcall;
	
	//
	// Set the argument
	//
	memset(&bw_upcall, 0, sizeof(bw_upcall));
	source.copy_out(bw_upcall.bu_src);
	group.copy_out(bw_upcall.bu_dst);
	threshold_interval.copy_out(bw_upcall.bu_threshold.b_time);
	bw_upcall.bu_threshold.b_packets = threshold_packets;
	bw_upcall.bu_threshold.b_bytes = threshold_bytes;
	if (is_threshold_in_packets)
	    bw_upcall.bu_flags |= BW_UPCALL_UNIT_PACKETS;
	if (is_threshold_in_bytes)
	    bw_upcall.bu_flags |= BW_UPCALL_UNIT_BYTES;
	do {
	    if (is_geq_upcall) {
		bw_upcall.bu_flags |= BW_UPCALL_GEQ;
		break;
	    }
	    if (is_leq_upcall) {
		bw_upcall.bu_flags |= BW_UPCALL_LEQ;
		break;
	    }
	    XLOG_UNREACHABLE();
	    return (XORP_ERROR);
	} while (false);
	
	if (setsockopt(_mrouter_socket, IPPROTO_IP, MRT_ADD_BW_UPCALL,
		       (void *)&bw_upcall, sizeof(bw_upcall)) < 0) {
	    error_msg = c_format("setsockopt(MRT_ADD_BW_UPCALL, (%s, %s)) failed: %s",
				 cstring(source), cstring(group),
				 strerror(errno));
	    XLOG_ERROR("%s", error_msg.c_str());
	    return (XORP_ERROR);
	}
#endif // MRT_ADD_BW_UPCALL && ENABLE_ADVANCED_MULTICAST_API
    }
    break;
    
#ifdef HAVE_IPV6
    case AF_INET6:
    {
#ifndef HAVE_IPV6_MULTICAST_ROUTING
	error_msg = c_format("add_bw_upcall() failed: "
			     "IPv6 multicast routing not supported");
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
#else
	
#if defined(MRT6_ADD_BW_UPCALL) && defined(ENABLE_ADVANCED_MULTICAST_API)
	struct bw6_upcall bw_upcall;
	
	//
	// Set the argument
	//
	memset(&bw_upcall, 0, sizeof(bw_upcall));
	source.copy_out(bw_upcall.bu6_src);
	group.copy_out(bw_upcall.bu6_dst);
	threshold_interval.copy_out(bw_upcall.bu6_threshold.b_time);
	bw_upcall.bu6_threshold.b_packets = threshold_packets;
	bw_upcall.bu6_threshold.b_bytes = threshold_bytes;
	if (is_threshold_in_packets)
	    bw_upcall.bu6_flags |= BW_UPCALL_UNIT_PACKETS;
	if (is_threshold_in_bytes)
	    bw_upcall.bu6_flags |= BW_UPCALL_UNIT_BYTES;
	do {
	    if (is_geq_upcall) {
		bw_upcall.bu6_flags |= BW_UPCALL_GEQ;
		break;
	    }
	    if (is_leq_upcall) {
		bw_upcall.bu6_flags |= BW_UPCALL_LEQ;
		break;
	    }
	    XLOG_UNREACHABLE();
	    return (XORP_ERROR);
	} while (false);
	
	if (setsockopt(_mrouter_socket, IPPROTO_IP, MRT6_ADD_BW_UPCALL,
		       (void *)&bw_upcall, sizeof(bw_upcall)) < 0) {
	    error_msg = c_format("setsockopt(MRT6_ADD_BW_UPCALL, (%s, %s)) failed: %s",
				 cstring(source), cstring(group),
				 strerror(errno));
	    XLOG_ERROR(("%s", error_msg.c_str());
	    return (XORP_ERROR);
	}
#endif // MRT6_ADD_BW_UPCALL && ENABLE_ADVANCED_MULTICAST_API
#endif // HAVE_IPV6_MULTICAST_ROUTING
    }
    break;
#endif // HAVE_IPV6
    
    default:
	XLOG_UNREACHABLE();
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

/**
 * MfeaMrouter::delete_bw_upcall:
 * @source: The source address.
 * @group: The group address.
 * @threshold_interval: The dataflow threshold interval.
 * @threshold_packets: The threshold (in number of packets) to compare against.
 * @threshold_bytes: The threshold (in number of bytes) to compare against.
 * @is_threshold_in_packets: If true, @threshold_packets is valid.
 * @is_threshold_in_bytes: If true, @threshold_bytes is valid.
 * @is_geq_upcall: If true, the operation for comparison is ">=".
 * @is_leq_upcall: If true, the operation for comparison is "<=".
 * @error_msg: The error message (if error).
 * 
 * Delete a dataflow monitor entry from the kernel.
 * Note: either @is_threshold_in_packets or @is_threshold_in_bytes (or both)
 * must be true.
 * Note: either @is_geq_upcall or @is_leq_upcall (but not both) must be true.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaMrouter::delete_bw_upcall(const IPvX& source, const IPvX& group,
			      const TimeVal& threshold_interval,
			      uint32_t threshold_packets,
			      uint32_t threshold_bytes,
			      bool is_threshold_in_packets,
			      bool is_threshold_in_bytes,
			      bool is_geq_upcall,
			      bool is_leq_upcall,
			      string& error_msg)
{
    XLOG_TRACE(mfea_node().is_log_trace(),
	       "Delete dataflow monitor: "
	       "source = %s group = %s "
	       "threshold_interval_sec = %d threshold_interval_usec = %d "
	       "threshold_packets = %d threshold_bytes = %d "
	       "is_threshold_in_packets = %d is_threshold_in_bytes = %d "
	       "is_geq_upcall = %d is_leq_upcall = %d",
	       cstring(source), cstring(group),
	       threshold_interval.sec(), threshold_interval.usec(),
	       threshold_packets, threshold_bytes,
	       is_threshold_in_packets, is_threshold_in_bytes,
	       is_geq_upcall, is_leq_upcall);

#if ! (defined(ENABLE_ADVANCED_MULTICAST_API) && defined(MRT_ADD_BW_UPCALL))
    UNUSED(threshold_interval);
    UNUSED(threshold_packets);
    UNUSED(threshold_bytes);
    UNUSED(is_threshold_in_packets);
    UNUSED(is_threshold_in_bytes);
    UNUSED(is_geq_upcall);
    UNUSED(is_leq_upcall);
#endif

    //
    // Check if the kernel supports the bandwidth-upcall mechanism.
    //
    if (! mrt_api_mrt_mfc_bw_upcall()) {
	error_msg = c_format("add_bw_upcall(%s, %s) failed: "
			     "dataflow monitor entry in the kernel "
			     "is not supported",
			     cstring(source), cstring(group));
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }

    // XXX: flags is_geq_upcall and is_leq_upcall are mutually exclusive
    if (! (is_geq_upcall ^ is_leq_upcall)) {
	error_msg = c_format("Cannot add dataflow monitor for (%s, %s): "
			     "the GEQ and LEQ flags are mutually exclusive "
			     "(GEQ = %s; LEQ = %s)",
			     cstring(source), cstring(group),
			     bool_c_str(is_geq_upcall),
			     bool_c_str(is_leq_upcall));
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);		// Invalid arguments
    }
    // XXX: at least one of the threshold flags must be set
    if (! (is_threshold_in_packets || is_threshold_in_bytes)) {
	error_msg = c_format("Cannot add dataflow monitor for (%s, %s): "
			     "invalid threshold flags "
			     "(is_threshold_in_packets = %s; "
			     "is_threshold_in_bytes = %s)",
			     cstring(source), cstring(group),
			     bool_c_str(is_threshold_in_packets),
			     bool_c_str(is_threshold_in_bytes));
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);		// Invalid arguments
    }
    
    //
    // Do the job
    //
    switch (family()) {
    case AF_INET:
    {
#if defined(MRT_ADD_BW_UPCALL) && defined(ENABLE_ADVANCED_MULTICAST_API)
	struct bw_upcall bw_upcall;
	
	//
	// Set the argument
	//
	memset(&bw_upcall, 0, sizeof(bw_upcall));
	source.copy_out(bw_upcall.bu_src);
	group.copy_out(bw_upcall.bu_dst);
	threshold_interval.copy_out(bw_upcall.bu_threshold.b_time);
	bw_upcall.bu_threshold.b_packets = threshold_packets;
	bw_upcall.bu_threshold.b_bytes = threshold_bytes;
	if (is_threshold_in_packets)
	    bw_upcall.bu_flags |= BW_UPCALL_UNIT_PACKETS;
	if (is_threshold_in_bytes)
	    bw_upcall.bu_flags |= BW_UPCALL_UNIT_BYTES;
	do {
	    if (is_geq_upcall) {
		bw_upcall.bu_flags |= BW_UPCALL_GEQ;
		break;
	    }
	    if (is_leq_upcall) {
		bw_upcall.bu_flags |= BW_UPCALL_LEQ;
		break;
	    }
	    XLOG_UNREACHABLE();
	    return (XORP_ERROR);
	} while (false);
	
	if (setsockopt(_mrouter_socket, IPPROTO_IP, MRT_DEL_BW_UPCALL,
		       (void *)&bw_upcall, sizeof(bw_upcall)) < 0) {
	    error_msg = c_format("setsockopt(MRT_DEL_BW_UPCALL, (%s, %s)) failed: %s",
				 cstring(source), cstring(group),
				 strerror(errno));
	    XLOG_ERROR("%s", error_msg.c_str());
	    return (XORP_ERROR);
	}
#endif // MRT_ADD_BW_UPCALL && ENABLE_ADVANCED_MULTICAST_API
    }
    break;
    
#ifdef HAVE_IPV6
    case AF_INET6:
    {
#ifndef HAVE_IPV6_MULTICAST_ROUTING
	error_msg = ("delete_bw_upcall() failed: "
		     "IPv6 multicast routing not supported");
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
#else
	
#if defined(MRT6_ADD_BW_UPCALL) && defined(ENABLE_ADVANCED_MULTICAST_API)
	struct bw6_upcall bw_upcall;
	
	//
	// Set the argument
	//
	memset(&bw_upcall, 0, sizeof(bw_upcall));
	source.copy_out(bw_upcall.bu6_src);
	group.copy_out(bw_upcall.bu6_dst);
	threshold_interval.copy_out(bw_upcall.bu6_threshold.b_time);
	bw_upcall.bu6_threshold.b_packets = threshold_packets;
	bw_upcall.bu6_threshold.b_bytes = threshold_bytes;
	if (is_threshold_in_packets)
	    bw_upcall.bu6_flags |= BW_UPCALL_UNIT_PACKETS;
	if (is_threshold_in_bytes)
	    bw_upcall.bu6_flags |= BW_UPCALL_UNIT_BYTES;
	do {
	    if (is_geq_upcall) {
		bw_upcall.bu6_flags |= BW_UPCALL_GEQ;
		break;
	    }
	    if (is_leq_upcall) {
		bw_upcall.bu6_flags |= BW_UPCALL_LEQ;
		break;
	    }
	    XLOG_UNREACHABLE();
	    return (XORP_ERROR);
	} while (false);
	
	if (setsockopt(_mrouter_socket, IPPROTO_IP, MRT6_DEL_BW_UPCALL,
		       (void *)&bw_upcall, sizeof(bw_upcall)) < 0) {
	    error_msg = c_format("setsockopt(MRT6_DEL_BW_UPCALL, (%s, %s)) failed: %s",
				 cstring(source), cstring(group),
				 strerror(errno));
	    XLOG_ERROR("%s", error_msg.c_str());
	    return (XORP_ERROR);
	}
#endif // MRT6_ADD_BW_UPCALL && ENABLE_ADVANCED_MULTICAST_API
#endif // HAVE_IPV6_MULTICAST_ROUTING
    }
    break;
#endif // HAVE_IPV6
    
    default:
	XLOG_UNREACHABLE();
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

/**
 * MfeaMrouter::delete_all_bw_upcall:
 * @source: The source address.
 * @group: The group address.
 * @error_msg: The error message (if error).
 * 
 * Delete all dataflow monitor entries from the kernel
 * for a given @source and @group address.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaMrouter::delete_all_bw_upcall(const IPvX& source, const IPvX& group,
				  string& error_msg)
{
    XLOG_TRACE(mfea_node().is_log_trace(),
	       "Delete all dataflow monitors: "
	       "source = %s group = %s",
	       cstring(source), cstring(group));

    //
    // Check if the kernel supports the bandwidth-upcall mechanism.
    //
    if (! mrt_api_mrt_mfc_bw_upcall()) {
	error_msg = c_format("add_bw_upcall(%s, %s) failed: "
			     "dataflow monitor entry in the kernel "
			     "is not supported",
			     cstring(source), cstring(group));
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    //
    // Do the job
    //
    switch (family()) {
    case AF_INET:
    {
#ifndef HAVE_IPV4_MULTICAST_ROUTING
	error_msg = c_format("delete_all_bw_upcall() failed: "
			     "IPv4 multicast routing not supported");
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
#else

#if defined(MRT_ADD_BW_UPCALL) && defined(ENABLE_ADVANCED_MULTICAST_API)
	struct bw_upcall bw_upcall;
	
	//
	// Set the argument
	//
	memset(&bw_upcall, 0, sizeof(bw_upcall));
	source.copy_out(bw_upcall.bu_src);
	group.copy_out(bw_upcall.bu_dst);
	bw_upcall.bu_flags |= BW_UPCALL_DELETE_ALL;
	
	if (setsockopt(_mrouter_socket, IPPROTO_IP, MRT_DEL_BW_UPCALL,
		       (void *)&bw_upcall, sizeof(bw_upcall)) < 0) {
	    error_msg = c_format("setsockopt(MRT_DEL_BW_UPCALL, (%s, %s)) failed: %s",
				 cstring(source), cstring(group),
				 strerror(errno));
	    XLOG_ERROR("%s", error_msg.c_str());
	    return (XORP_ERROR);
	}
#endif // MRT_ADD_BW_UPCALL && ENABLE_ADVANCED_MULTICAST_API

#endif // HAVE_IPV4_MULTICAST_ROUTING
    }
    break;
    
#ifdef HAVE_IPV6
    case AF_INET6:
    {
#ifndef HAVE_IPV6_MULTICAST_ROUTING
	error_msg = c_format("delete_all_bw_upcall() failed: "
			     "IPv6 multicast routing not supported");
	XLOG_ERROR("%s", error_msg.c_str());
	return (XORP_ERROR);
#else
	
#if defined(MRT6_ADD_BW_UPCALL) && defined(ENABLE_ADVANCED_MULTICAST_API)
	struct bw6_upcall bw_upcall;
	
	//
	// Set the argument
	//
	memset(&bw_upcall, 0, sizeof(bw_upcall));
	source.copy_out(bw_upcall.bu6_src);
	group.copy_out(bw_upcall.bu6_dst);
	bw_upcall.bu6_flags |= BW_UPCALL_DELETE_ALL;
	
	if (setsockopt(_mrouter_socket, IPPROTO_IP, MRT6_DEL_BW_UPCALL,
		       (void *)&bw_upcall, sizeof(bw_upcall)) < 0) {
	    error_msg = c_format("setsockopt(MRT6_DEL_BW_UPCALL, (%s, %s)) failed: %s",
				 cstring(source), cstring(group),
				 strerror(errno));
	    XLOG_ERROR("%s", error_msg.c_str());
	    return (XORP_ERROR);
	}
#endif // MRT6_ADD_BW_UPCALL && ENABLE_ADVANCED_MULTICAST_API
#endif // HAVE_IPV6_MULTICAST_ROUTING
    }
    break;
#endif // HAVE_IPV6
    
    default:
	XLOG_UNREACHABLE();
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

/**
 * MfeaMrouter::get_sg_count:
 * @source: The MFC source address.
 * @group: The MFC group address.
 * @sg_count: A reference to a #SgCount class to place the result.
 * 
 * Get various counters per (S,G) entry.
 * Get the number of packets and bytes forwarded by a particular
 * Multicast Forwarding Cache (MFC) entry in the kernel, and the number
 * of packets arrived on wrong interface for that entry.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaMrouter::get_sg_count(const IPvX& source, const IPvX& group,
			  SgCount& sg_count)
{
#if !defined(HAVE_IPV4_MULTICAST_ROUTING) && !defined(HAVE_IPV6_MULTICAST_ROUTING)
    UNUSED(source);
    UNUSED(group);
    UNUSED(sg_count);
#endif

    switch (family()) {
    case AF_INET:
    {
#ifndef HAVE_IPV4_MULTICAST_ROUTING
	XLOG_ERROR("get_sg_count() failed: "
		   "IPv4 multicast routing not supported");
	return (XORP_ERROR);
#else
	int ioctl_cmd = SIOCGETSGCNT;
#ifdef USE_MULT_MCAST_TABLES
	struct sioc_sg_req_ng sgreq_ng __attribute__((aligned(64)));
	// work around invalid g++ 9.1.1 warning about taking addr of packed element
	unsigned char* tmpp = (unsigned char*)(&(sgreq_ng.req));
	struct sioc_sg_req* sgreqp = (struct sioc_sg_req*)(tmpp);
	memset(&sgreq_ng, 0, sizeof(sgreq_ng));
	sgreq_ng.table_id = getTableId();
	void* o = &sgreq_ng;
	ioctl_cmd = SIOCGETSGCNT_NG;
	if (new_mcast_tables_api || !supports_mcast_tables) {
	    o = &(sgreq_ng.req);
	    ioctl_cmd = SIOCGETSGCNT;
	}
#else
	struct sioc_sg_req sgreq;
	struct sioc_sg_req* sgreqp = &sgreq;
	memset(&sgreq, 0, sizeof(sgreq));
	void* o = &sgreq;
#endif
	
	source.copy_out(sgreqp->src);
	group.copy_out(sgreqp->grp);

	//
	// XXX: some older mcast code has bug in ip_mroute.c, get_sg_cnt():
	// the return code is always 0, so this is why we need to check
	// if all values are 0xffffffffU (the indication for error).
	// TODO: remove the 0xffffffffU check in the future.
	//
	if ((ioctl(_mrouter_socket, ioctl_cmd, o) < 0)
	    || ((sgreqp->pktcnt == 0xffffffffU)
		&& (sgreqp->bytecnt == 0xffffffffU)
		&& (sgreqp->wrong_if == 0xffffffffU))) {
	    XLOG_ERROR("ioctl(SIOCGETSGCNT(%i), (%s %s)) failed: %s",
		       ioctl_cmd, cstring(source), cstring(group), strerror(errno));
	    sg_count.set_pktcnt(~0U);
	    sg_count.set_bytecnt(~0U);
	    sg_count.set_wrong_if(~0U);
	    return (XORP_ERROR);
	}
	sg_count.set_pktcnt(sgreqp->pktcnt);
	sg_count.set_bytecnt(sgreqp->bytecnt);
	sg_count.set_wrong_if(sgreqp->wrong_if);
#endif // HAVE_IPV4_MULTICAST_ROUTING
    }
    break;
    
#ifdef HAVE_IPV6
    case AF_INET6:
    {
#ifndef HAVE_IPV6_MULTICAST_ROUTING
	XLOG_ERROR("get_sg_count() failed: "
		   "IPv6 multicast routing not supported");
	return (XORP_ERROR);
#else
	struct sioc_sg_req6 sgreq;

	memset(&sgreq, 0, sizeof(sgreq));
	source.copy_out(sgreq.src);
	group.copy_out(sgreq.grp);
	if (ioctl(_mrouter_socket, SIOCGETSGCNT_IN6, &sgreq) < 0) {
	    XLOG_ERROR("ioctl(SIOCGETSGCNT_IN6, (%s %s)) failed: %s",
		       cstring(source), cstring(group), strerror(errno));
	    sg_count.set_pktcnt(~0U);
	    sg_count.set_bytecnt(~0U);
	    sg_count.set_wrong_if(~0U);
	    return (XORP_ERROR);
	}
	sg_count.set_pktcnt(sgreq.pktcnt);
	sg_count.set_bytecnt(sgreq.bytecnt);
	sg_count.set_wrong_if(sgreq.wrong_if);
#endif // HAVE_IPV6_MULTICAST_ROUTING
    }
    break;
#endif // HAVE_IPV6
    
    default:
	XLOG_UNREACHABLE();
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}


/**
 * MfeaMrouter::get_vif_count:
 * @vif_index: The vif index of the virtual multicast interface whose
 * statistics we need.
 * @vif_count: A reference to a #VifCount class to store the result.
 * 
 * Get various counters per virtual interface.
 * Get the number of packets and bytes received on, or forwarded on
 * a particular multicast interface.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaMrouter::get_vif_count(uint32_t vif_index, VifCount& vif_count)
{
    MfeaVif *mfea_vif = mfea_node().vif_find_by_vif_index(vif_index);
    
    if (mfea_vif == NULL)
	return (XORP_ERROR);

#if !defined(HAVE_IPV4_MULTICAST_ROUTING) && !defined(HAVE_IPV6_MULTICAST_ROUTING)
    UNUSED(vif_count);
#endif
    
    switch (family()) {
    case AF_INET:
    {
#ifndef HAVE_IPV4_MULTICAST_ROUTING
	XLOG_ERROR("get_vif_count() failed: "
		   "IPv4 multicast routing not supported");
	return (XORP_ERROR);
#else
	int ioctl_cmd = SIOCGETVIFCNT;
#ifdef USE_MULT_MCAST_TABLES
	struct sioc_vif_req_ng vreq_ng __attribute__((aligned(64)));
	// work around invalid g++ 9.1.1 warning about taking addr of packed element
	unsigned char* tmpp = (unsigned char*)(&(vreq_ng.vif));
	struct sioc_vif_req* vreqp = (struct sioc_vif_req*)(tmpp);
	memset(&vreq_ng, 0, sizeof(vreq_ng));
	vreq_ng.table_id = getTableId();
	void* o = &vreq_ng;
	ioctl_cmd = SIOCGETVIFCNT_NG;
	if (new_mcast_tables_api || !supports_mcast_tables) {
	    o = &(vreq_ng.vif);
	    ioctl_cmd = SIOCGETVIFCNT;
	}
#else
	struct sioc_vif_req vreq;
	struct sioc_vif_req* vreqp = &vreq;
	memset(&vreq, 0, sizeof(vreq));
	void* o = &vreq;
#endif

	vreqp->vifi = mfea_vif->vif_index();

	if (ioctl(_mrouter_socket, ioctl_cmd, o) < 0) {
	    XLOG_ERROR("ioctl(SIOCGETVIFCNT, vif %s) failed: %s",
		       mfea_vif->name().c_str(), strerror(errno));
	    vif_count.set_icount(~0U);
	    vif_count.set_ocount(~0U);
	    vif_count.set_ibytes(~0U);
	    vif_count.set_obytes(~0U);
	    return (XORP_ERROR);
	}
	vif_count.set_icount(vreqp->icount);
	vif_count.set_ocount(vreqp->ocount);
	vif_count.set_ibytes(vreqp->ibytes);
	vif_count.set_obytes(vreqp->obytes);
#endif // HAVE_IPV4_MULTICAST_ROUTING
    }
    break;
    
#ifdef HAVE_IPV6
    case AF_INET6:
    {
#ifndef HAVE_IPV6_MULTICAST_ROUTING
	XLOG_ERROR("get_vif_count() failed: "
		   "IPv6 multicast routing not supported");
	return (XORP_ERROR);
#else
	struct sioc_mif_req6 mreq;
	
	memset(&mreq, 0, sizeof(mreq));
	mreq.mifi = mfea_vif->vif_index();
	if (ioctl(_mrouter_socket, SIOCGETMIFCNT_IN6, &mreq) < 0) {
	    XLOG_ERROR("ioctl(SIOCGETMIFCNT_IN6, vif %s) failed: %s",
		       mfea_vif->name().c_str(), strerror(errno));
	    vif_count.set_icount(~0U);
	    vif_count.set_ocount(~0U);
	    vif_count.set_ibytes(~0U);
	    vif_count.set_obytes(~0U);
	    return (XORP_ERROR);
	}
	vif_count.set_icount(mreq.icount);
	vif_count.set_ocount(mreq.ocount);
	vif_count.set_ibytes(mreq.ibytes);
	vif_count.set_obytes(mreq.obytes);
#endif // HAVE_IPV6_MULTICAST_ROUTING
    }
    break;
#endif // HAVE_IPV6
    
    default:
	XLOG_UNREACHABLE();
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

/**
 * kernel_call_process:
 * @databuf: The data buffer.
 * @datalen: The length of the data in 'databuf'.
 * 
 * Process a call from the kernel (e.g., "nocache", "wrongiif", "wholepkt")
 * XXX: It is OK for im_src/im6_src to be 0 (for 'nocache' or 'wrongiif'),
 *	just in case the kernel supports (*,G) MFC.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
MfeaMrouter::kernel_call_process(const uint8_t *databuf, size_t datalen)
{
    uint32_t	iif_vif_index;
    int		message_type;
    IPvX	src(family()), dst(family());

#if !defined(HAVE_IPV4_MULTICAST_ROUTING) && !defined(HAVE_IPV6_MULTICAST_ROUTING)
    UNUSED(iif_vif_index);
    UNUSED(message_type);
    UNUSED(databuf);
    UNUSED(datalen);
#endif

    //XLOG_INFO("kernel-call-process, datalen: %d\n", (int)(datalen));
    
    switch (family()) {
    case AF_INET:
    {
#ifndef HAVE_IPV4_MULTICAST_ROUTING
	XLOG_ERROR("kernel_call_process() failed: "
		   "IPv4 multicast routing not supported");
	return (XORP_ERROR);
#else
	struct igmpmsg igmpmsg;
	memcpy(&igmpmsg, databuf, sizeof(igmpmsg));
	
	//
	// Get the message type, the iif, and source and destination address
	//
	message_type = igmpmsg.im_msgtype;
	iif_vif_index = igmpmsg.im_vif;
	if (message_type == IGMPMSG_WHOLEPKT) {
	    //
	    // If a WHOLEPKT message, then get the inner source and
	    // destination addresses
	    //
	    IpHeader4 ip4(databuf + sizeof(struct igmpmsg));
	    if (datalen - sizeof(struct igmpmsg) < ip4.size()) {
		// The inner packet is too small
		return (XORP_ERROR);
	    }
	    src = ip4.ip_src();
	    dst = ip4.ip_dst();
	} else {
	    src.copy_in(igmpmsg.im_src);
	    dst.copy_in(igmpmsg.im_dst);
	}

	//
	// Check if the iif is valid and UP
	//
	switch (message_type) {
	case IGMPMSG_NOCACHE:
	case IGMPMSG_WRONGVIF:
	case IGMPMSG_WHOLEPKT:
	{
	    MfeaVif *mfea_vif = mfea_node().vif_find_by_vif_index(iif_vif_index);
	    if ((mfea_vif == NULL) || (! mfea_vif->is_up())) {
		// Silently ignore the packet
		XLOG_ERROR("kernel_call_process, ignoring pkt, can't find mfea_vif by index: %i",
			   iif_vif_index);

		return (XORP_ERROR);
	    }
	}
	break;
#if defined(IGMPMSG_BW_UPCALL) && defined(ENABLE_ADVANCED_MULTICAST_API)
	case IGMPMSG_BW_UPCALL:
	    break;
#endif
	default:
	    break;
	}
	
	//
	// Check 'src' and 'dst' addresses
	//
	switch (message_type) {
	case IGMPMSG_NOCACHE:
	case IGMPMSG_WRONGVIF:
	case IGMPMSG_WHOLEPKT:
	    if ((! src.is_unicast())
		|| (! dst.is_multicast())
		|| (dst.is_linklocal_multicast())) {
		// XXX: LAN-scoped addresses are not routed
		XLOG_ERROR("kernel_call_process, src and/or dst not valid, src: %s  dst: %s",
			   src.str().c_str(), dst.str().c_str());

		return (XORP_ERROR);
	    }
	    break;
#if defined(IGMPMSG_BW_UPCALL) && defined(ENABLE_ADVANCED_MULTICAST_API)
	case IGMPMSG_BW_UPCALL:
	    break;
#endif
	default:
	    break;
	}
	//
	// Deliver the signal
	//
	mfea_node().signal_message_recv("",
					message_type,
					iif_vif_index, src, dst,
					databuf + sizeof(struct igmpmsg),
					datalen - sizeof(struct igmpmsg));
#endif // HAVE_IPV4_MULTICAST_ROUTING
    }
    break;
    
#ifdef HAVE_IPV6
    case AF_INET6:
    {
#ifndef HAVE_IPV6_MULTICAST_ROUTING
	XLOG_ERROR("kernel_call_process() failed: "
		   "IPv6 multicast routing not supported");
	return (XORP_ERROR);
#else
	struct mrt6msg mrt6msg;
	memcpy(&mrt6msg, databuf, sizeof(mrt6msg));
	//
	// Get the message type, the iif, and source and destination address
	//
	message_type = mrt6msg.im6_msgtype;
	iif_vif_index = mrt6msg.im6_mif;
	if (message_type == MRT6MSG_WHOLEPKT) {
	    //
	    // If a WHOLEPKT message, then get the inner source and
	    // destination addresses
	    //
	    IpHeader6 ip6(databuf + sizeof(struct mrt6msg));
	    if (datalen - sizeof(struct mrt6msg) < ip6.size()) {
		// The inner packet is too small
		return (XORP_ERROR);
	    }
	    src = ip6.ip_src();
	    dst = ip6.ip_dst();
	} else {
	    src.copy_in(mrt6msg.im6_src);
	    dst.copy_in(mrt6msg.im6_dst);
	}
	
	//
	// Check if the iif is valid and UP
	//
	switch (message_type) {
	case MRT6MSG_NOCACHE:
	case MRT6MSG_WRONGMIF:
	case MRT6MSG_WHOLEPKT:
	{
	    // Check if the iif is valid and UP
	    MfeaVif *mfea_vif = mfea_node().vif_find_by_vif_index(iif_vif_index);
	    if ((mfea_vif == NULL) || (! mfea_vif->is_up())) {
		// Silently ignore the packet
		return (XORP_ERROR);
	    }
	}
	break;
#if defined(MRT6MSG_BW_UPCALL) && defined(ENABLE_ADVANCED_MULTICAST_API)
	case MRT6MSG_BW_UPCALL:
	    break;
#endif
	default:
	    break;
	}
	
	//
	// Check 'src' and 'dst' addresses
	//
	switch (message_type) {
	case MRT6MSG_NOCACHE:
	case MRT6MSG_WRONGMIF:
	case MRT6MSG_WHOLEPKT:
	    if ((! src.is_unicast())
		|| (! dst.is_multicast())
		|| dst.is_linklocal_multicast()) {
		// XXX: LAN-scoped addresses are not routed
		return (XORP_ERROR);
	    }
	    break;
#if defined(MRT6MSG_BW_UPCALL) && defined(ENABLE_ADVANCED_MULTICAST_API)
	case MRT6MSG_BW_UPCALL:
	    break;
#endif
	default:
	    break;
	}
	//
	// Deliver the signal
	//
	mfea_node().signal_message_recv("",
					message_type,
					iif_vif_index, src, dst,
					databuf + sizeof(struct mrt6msg),
					datalen - sizeof(struct mrt6msg));
#endif // HAVE_IPV6_MULTICAST_ROUTING
    }
    break;
#endif // HAVE_IPV6
    
    default:
	XLOG_UNREACHABLE();
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}
