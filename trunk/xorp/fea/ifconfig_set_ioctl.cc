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

#ident "$XORP: xorp/fea/ifconfig_set_ioctl.cc,v 1.29 2004/09/15 18:47:26 pavlin Exp $"


#include "fea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ether_compat.h"

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#include <net/if.h>
#include <net/if_arp.h>
#ifdef HAVE_NET_IF_VAR_H
#include <net/if_var.h>
#endif

#ifdef HAVE_NETINET_IN_VAR_H
#include <netinet/in_var.h>
#endif
#ifdef HAVE_NETINET6_IN6_VAR_H
#include <netinet6/in6_var.h>
#endif
#ifdef HAVE_NETINET6_ND6_H
#ifdef HAVE_BROKEN_CXX_NETINET6_ND6_H
// XXX: a hack needed if <netinet6/nd6.h> is not C++ friendly
#define prf_ra in6_prflags::prf_ra
#endif
#include <netinet6/nd6.h>
#endif

#include "ifconfig.hh"
#include "ifconfig_set.hh"

#ifdef HAVE_IPV6
#ifdef HOST_OS_LINUX
//
// XXX: In case of Linux, we have "struct in6_ifreq" defined
// in <linux/ipv6.h>. However, we cannot include that file along
// with <netinet/in.h> because of replicated structure definitions
// in <netinet/in.h> and <linux/in6.h> where the latter one is
// included by <linux/ipv6.h>.
// Hence, we have no choice but explicitly define here "struct in6_ifreq".
// BTW, please note that the Linux struct in6_ifreq has nothing in common
// with the original KAME's "struct in6_ifreq".
//
struct in6_ifreq {
    struct in6_addr ifr6_addr;
    uint32_t ifr6_prefixlen;
    unsigned int ifr6_ifindex;
};
#endif // HOST_OS_LINUX
#endif // HAVE_IPV6


//
// Set information about network interfaces configuration with the
// underlying system.
//
// The mechanism to set the information is ioctl(2).
//

IfConfigSetIoctl::IfConfigSetIoctl(IfConfig& ifc)
    : IfConfigSet(ifc), _s4(-1), _s6(-1)
{
#ifdef HAVE_IOCTL_SIOCGIFCONF
    register_ifc_primary();
#endif
}

IfConfigSetIoctl::~IfConfigSetIoctl()
{
    stop();
}

int
IfConfigSetIoctl::start()
{
    if (_is_running)
	return (XORP_OK);

    if (ifc().have_ipv4()) {
	if (_s4 < 0) {
	    _s4 = socket(AF_INET, SOCK_DGRAM, 0);
	    if (_s4 < 0) {
		XLOG_FATAL("Could not initialize IPv4 ioctl() socket");
	    }
	}
    }
    
#ifdef HAVE_IPV6
    if (ifc().have_ipv6()) {
	if (_s6 < 0) {
	    _s6 = socket(AF_INET6, SOCK_DGRAM, 0);
	    if (_s6 < 0) {
		XLOG_FATAL("Could not initialize IPv6 ioctl() socket");
	    }
	}
    }
#endif // HAVE_IPV6

    _is_running = true;

    return (XORP_OK);
}

int
IfConfigSetIoctl::stop()
{
    if (! _is_running)
	return (XORP_OK);

    if (_s4 >= 0) {
	close(_s4);
	_s4 = -1;
    }
    if (_s6 >= 0) {
	close(_s6);
	_s6 = -1;
    }

    _is_running = false;

    return (XORP_OK);
}

#ifndef HAVE_IOCTL_SIOCGIFCONF
int
IfConfigSetIoctl::config_begin(string& errmsg)
{
    debug_msg("config_begin\n");

    errmsg = "method not supported";

    return (XORP_ERROR);
}

int
IfConfigSetIoctl::config_end(string& errmsg)
{
    debug_msg("config_end\n");

    errmsg = "method not supported";

    return (XORP_ERROR);
}

int
IfConfigSetIoctl::add_interface(const string& ifname,
				uint16_t if_index,
				string& errmsg)
{
    debug_msg("add_interface "
	      "(ifname = %s if_index = %u)\n",
	      ifname.c_str(), if_index);

    UNUSED(ifname);
    UNUSED(if_index);

    errmsg = "method not supported";

    return (XORP_ERROR);
}

int
IfConfigSetIoctl::add_vif(const string& ifname,
			  const string& vifname,
			  uint16_t if_index,
			  string& errmsg)
{
    debug_msg("add_vif "
	      "(ifname = %s vifname = %s if_index = %u)\n",
	      ifname.c_str(), vifname.c_str(), if_index);

    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(if_index);

    errmsg = "method not supported";

    return (XORP_ERROR);
}

int
IfConfigSetIoctl::config_interface(const string& ifname,
				   uint16_t if_index,
				   uint32_t flags,
				   bool is_up,
				   bool is_deleted,
				   string& errmsg)
{
    debug_msg("config_interface "
	      "(ifname = %s if_index = %u flags = 0x%x is_up = %s "
	      "is_deleted = %s)\n",
	      ifname.c_str(), if_index, flags, (is_up)? "true" : "false",
	      (is_deleted)? "true" : "false");

    UNUSED(ifname);
    UNUSED(if_index);
    UNUSED(flags);
    UNUSED(is_up);
    UNUSED(is_deleted);

    errmsg = "method not supported";

    return (XORP_ERROR);
}

int
IfConfigSetIoctl::config_vif(const string& ifname,
			     const string& vifname,
			     uint16_t if_index,
			     uint32_t flags,
			     bool is_up,
			     bool is_deleted,
			     bool broadcast,
			     bool loopback,
			     bool point_to_point,
			     bool multicast,
			     string& errmsg)
{
    debug_msg("config_vif "
	      "(ifname = %s vifname = %s if_index = %u flags = 0x%x "
	      "is_up = %s is_deleted = %s broadcast = %s loopback = %s "
	      "point_to_point = %s multicast = %s)\n",
	      ifname.c_str(), vifname.c_str(), if_index, flags,
	      (is_up)? "true" : "false",
	      (is_deleted)? "true" : "false",
	      (broadcast)? "true" : "false",
	      (loopback)? "true" : "false",
	      (point_to_point)? "true" : "false",
	      (multicast)? "true" : "false");

    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(if_index);
    UNUSED(flags);
    UNUSED(is_up);
    UNUSED(is_deleted);
    UNUSED(broadcast);
    UNUSED(loopback);
    UNUSED(point_to_point);
    UNUSED(multicast);

    errmsg = "method not supported";

    return (XORP_ERROR);
}

int
IfConfigSetIoctl::set_interface_mac_address(const string& ifname,
					    uint16_t if_index,
					    const struct ether_addr& ether_addr,
					    string& errmsg)
{
    debug_msg("set_interface_mac "
	      "(ifname = %s if_index = %u mac = %s)\n",
	      ifname.c_str(), if_index, EtherMac(ether_addr).str().c_str());

    UNUSED(ifname);
    UNUSED(if_index);
    UNUSED(ether_addr);

    errmsg = "method not supported";

    return (XORP_ERROR);
}

int
IfConfigSetIoctl::set_interface_mtu(const string& ifname,
				    uint16_t if_index,
				    uint32_t mtu,
				    string& errmsg)
{
    debug_msg("set_interface_mtu "
	      "(ifname = %s if_index = %u mtu = %u)\n",
	      ifname.c_str(), if_index, mtu);

    UNUSED(ifname);
    UNUSED(if_index);
    UNUSED(mtu);

    errmsg = "method not supported";

    return (XORP_ERROR);
}

int
IfConfigSetIoctl::add_vif_address(const string& ifname,
				  const string& vifname,
				  uint16_t if_index,
				  bool is_broadcast,
				  bool is_p2p,
				  const IPvX& addr,
				  const IPvX& dst_or_bcast,
				  uint32_t prefix_len,
				  string& errmsg)
{
    debug_msg("add_vif_address "
	      "(ifname = %s vifname = %s if_index = %u is_broadcast = %s "
	      "is_p2p = %s addr = %s dst/bcast = %s prefix_len = %u)\n",
	      ifname.c_str(), vifname.c_str(), if_index,
	      (is_broadcast)? "true" : "false", (is_p2p)? "true" : "false",
	      addr.str().c_str(), dst_or_bcast.str().c_str(), prefix_len);

    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(if_index);
    UNUSED(is_broadcast);
    UNUSED(is_p2p);
    UNUSED(addr);
    UNUSED(dst_or_bcast);
    UNUSED(prefix_len);

    errmsg = "method not supported";

    return (XORP_ERROR);
}

int
IfConfigSetIoctl::delete_vif_address(const string& ifname,
				     const string& vifname,
				     uint16_t if_index,
				     const IPvX& addr,
				     uint32_t prefix_len,
				     string& errmsg)
{
    debug_msg("delete_vif_address "
	      "(ifname = %s vifname = %s if_index = %u addr = %s "
	      "prefix_len = %u)\n",
	      ifname.c_str(), vifname.c_str(), if_index, addr.str().c_str(),
	      prefix_len);

    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(if_index);
    UNUSED(addr);
    UNUSED(prefix_len);

    errmsg = "method not supported";

    return (XORP_ERROR);
}

#else // HAVE_IOCTL_SIOCGIFCONF

int
IfConfigSetIoctl::config_begin(string& errmsg)
{
    debug_msg("config_begin\n");

    // XXX: nothing to do

    UNUSED(errmsg);

    return (XORP_OK);
}

int
IfConfigSetIoctl::config_end(string& errmsg)
{
    debug_msg("config_end\n");

    // XXX: nothing to do

    UNUSED(errmsg);

    return (XORP_OK);
}

int
IfConfigSetIoctl::add_interface(const string& ifname,
				uint16_t if_index,
				string& errmsg)
{
    debug_msg("add_interface "
	      "(ifname = %s if_index = %u)\n",
	      ifname.c_str(), if_index);

    // XXX: nothing to do

    UNUSED(ifname);
    UNUSED(if_index);
    UNUSED(errmsg);

    return (XORP_OK);
}

int
IfConfigSetIoctl::add_vif(const string& ifname,
			  const string& vifname,
			  uint16_t if_index,
			  string& errmsg)
{
    debug_msg("add_vif "
	      "(ifname = %s vifname = %s if_index = %u)\n",
	      ifname.c_str(), vifname.c_str(), if_index);

    // XXX: nothing to do

    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(if_index);
    UNUSED(errmsg);

    return (XORP_OK);
}

int
IfConfigSetIoctl::config_interface(const string& ifname,
				   uint16_t if_index,
				   uint32_t flags,
				   bool is_up,
				   bool is_deleted,
				   string& errmsg)
{
    struct ifreq ifreq;

    debug_msg("config_interface "
	      "(ifname = %s if_index = %u flags = 0x%x is_up = %s "
	      "is_deleted = %s)\n",
	      ifname.c_str(), if_index, flags, (is_up)? "true" : "false",
	      (is_deleted)? "true" : "false");

    UNUSED(if_index);
    UNUSED(is_up);
    UNUSED(is_deleted);

    memset(&ifreq, 0, sizeof(ifreq));
    strncpy(ifreq.ifr_name, ifname.c_str(), sizeof(ifreq.ifr_name) - 1);

    ifreq.ifr_flags = flags;
    if (ioctl(_s4, SIOCSIFFLAGS, &ifreq) < 0) {
	errmsg = c_format("%s", strerror(errno));
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
IfConfigSetIoctl::config_vif(const string& ifname,
			     const string& vifname,
			     uint16_t if_index,
			     uint32_t flags,
			     bool is_up,
			     bool is_deleted,
			     bool broadcast,
			     bool loopback,
			     bool point_to_point,
			     bool multicast,
			     string& errmsg)
{
    debug_msg("config_vif "
	      "(ifname = %s vifname = %s if_index = %u flags = 0x%x "
	      "is_up = %s is_deleted = %s broadcast = %s loopback = %s "
	      "point_to_point = %s multicast = %s)\n",
	      ifname.c_str(), vifname.c_str(), if_index, flags,
	      (is_up)? "true" : "false",
	      (is_deleted)? "true" : "false",
	      (broadcast)? "true" : "false",
	      (loopback)? "true" : "false",
	      (point_to_point)? "true" : "false",
	      (multicast)? "true" : "false");

    // XXX: nothing to do

    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(if_index);
    UNUSED(flags);
    UNUSED(is_up);
    UNUSED(is_deleted);
    UNUSED(broadcast);
    UNUSED(loopback);
    UNUSED(point_to_point);
    UNUSED(multicast);
    UNUSED(errmsg);

    return (XORP_OK);
}

int
IfConfigSetIoctl::set_interface_mac_address(const string& ifname,
					    uint16_t if_index,
					    const struct ether_addr& ether_addr,
					    string& errmsg)
{
    struct ifreq ifreq;

    debug_msg("set_interface_mac "
	      "(ifname = %s if_index = %u mac = %s)\n",
	      ifname.c_str(), if_index, EtherMac(ether_addr).str().c_str());

    UNUSED(if_index);

    memset(&ifreq, 0, sizeof(ifreq));
    strncpy(ifreq.ifr_name, ifname.c_str(), sizeof(ifreq.ifr_name) - 1);

#if defined(SIOCSIFLLADDR)
    //
    // FreeBSD
    //
    ifreq.ifr_addr.sa_family = AF_LINK;
    memcpy(ifreq.ifr_addr.sa_data, &ether_addr, ETHER_ADDR_LEN);
#ifdef HAVE_SA_LEN
    ifreq.ifr_addr.sa_len = ETHER_ADDR_LEN;
#endif
    if (ioctl(_s4, SIOCSIFLLADDR, &ifreq) < 0) {
	errmsg = c_format("%s", strerror(errno));
	return (XORP_ERROR);
    }
    return (XORP_OK);

#elif defined(SIOCSIFHWADDR)
    //
    // Linux
    //
    ifreq.ifr_hwaddr.sa_family = ARPHRD_ETHER;
    memcpy(ifreq.ifr_hwaddr.sa_data, &ether_addr, ETH_ALEN);
#ifdef HAVE_SA_LEN
    ifreq.ifr_hwaddr.sa_len = ETH_ALEN;
#endif
    if (ioctl(_s4, SIOCSIFHWADDR, &ifreq) < 0) {
	errmsg = c_format("%s", strerror(errno));
	return (XORP_ERROR);
    }
    return (XORP_OK);

#else
    //
    // No mechanism: NetBSD and OpenBSD, et. al.
    //
    // XXX: currently (NetBSD-1.6.1 and OpenBSD-3.3) do not support
    // setting the MAC address.
    //
    UNUSED(ether_addr);
    errmsg = c_format("No mechanism to set the MAC address on an interface");
    return (XORP_ERROR);
#endif
}

int
IfConfigSetIoctl::set_interface_mtu(const string& ifname,
				    uint16_t if_index,
				    uint32_t mtu,
				    string& errmsg)
{
    struct ifreq ifreq;

    debug_msg("set_interface_mtu "
	      "(ifname = %s if_index = %u mtu = %u)\n",
	      ifname.c_str(), if_index, mtu);

    UNUSED(if_index);

    memset(&ifreq, 0, sizeof(ifreq));
    strncpy(ifreq.ifr_name, ifname.c_str(), sizeof(ifreq.ifr_name) - 1);

    ifreq.ifr_mtu = mtu;
    if (ioctl(_s4, SIOCSIFMTU, &ifreq) < 0) {
	errmsg = c_format("%s", strerror(errno));
	return (XORP_ERROR);
    }
    return (XORP_OK);
}

int
IfConfigSetIoctl::add_vif_address(const string& ifname,
				  const string& vifname,
				  uint16_t if_index,
				  bool is_broadcast,
				  bool is_p2p,
				  const IPvX& addr,
				  const IPvX& dst_or_bcast,
				  uint32_t prefix_len,
				  string& errmsg)
{
    debug_msg("add_vif_address "
	      "(ifname = %s vifname = %s if_index = %u is_broadcast = %s "
	      "is_p2p = %s addr = %s dst/bcast = %s prefix_len = %u)\n",
	      ifname.c_str(), vifname.c_str(), if_index,
	      (is_broadcast)? "true" : "false", (is_p2p)? "true" : "false",
	      addr.str().c_str(), dst_or_bcast.str().c_str(), prefix_len);

    switch (addr.af()) {
    case AF_INET:
	return add_vif_address4(ifname, vifname, if_index, is_broadcast,
				is_p2p, addr, dst_or_bcast, prefix_len,
				errmsg);
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	return add_vif_address6(ifname, vifname, if_index, is_p2p,
				addr, dst_or_bcast, prefix_len, errmsg);
	break;
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
	break;
    }

    XLOG_UNREACHABLE();
    return (XORP_ERROR);
}

/**
 * Add an IPv4 address to an interface.
 *
 * Note: if the system has ioctl(SIOCAIFADDR) (e.g., FreeBSD), then
 * the interface address is added as an alias, otherwise it overwrites the
 * previous address (if such exists).
 */
int
IfConfigSetIoctl::add_vif_address4(const string& ifname,
				   const string& vifname,
				   uint16_t if_index,
				   bool is_broadcast,
				   bool is_p2p,
				   const IPvX& addr,
				   const IPvX& dst_or_bcast,
				   uint32_t prefix_len,
				   string& errmsg)
{
    debug_msg("add_vif_address4 "
	      "(ifname = %s vifname = %s if_index = %u is_broadcast = %s "
	      "is_p2p = %s addr = %s dst/bcast = %s prefix_len = %u)\n",
	      ifname.c_str(), vifname.c_str(), if_index,
	      (is_broadcast)? "true" : "false", (is_p2p)? "true" : "false",
	      addr.str().c_str(), dst_or_bcast.str().c_str(), prefix_len);

    UNUSED(vifname);
    UNUSED(if_index);
    UNUSED(is_broadcast);

    if (! ifc().have_ipv4()) {
	errmsg = "IPv4 is not supported";
	return (XORP_ERROR);
    }

#ifdef SIOCAIFADDR
    //
    // Add an alias address
    //
    struct in_aliasreq ifra;

    memset(&ifra, 0, sizeof(ifra));
    strncpy(ifra.ifra_name, ifname.c_str(), sizeof(ifra.ifra_name) - 1);
    addr.copy_out(ifra.ifra_addr);
    if (is_p2p)
	dst_or_bcast.copy_out(ifra.ifra_dstaddr);
    else
	dst_or_bcast.copy_out(ifra.ifra_broadaddr);
    IPvX::make_prefix(addr.af(), prefix_len).copy_out(ifra.ifra_mask);
    if (ioctl(_s4, SIOCAIFADDR, &ifra) < 0) {
	errmsg = c_format("%s", strerror(errno));
	return (XORP_ERROR);
    }

#endif // SIOCAIFADDR

    //
    // Set a new address
    //
    struct ifreq ifreq;

    memset(&ifreq, 0, sizeof(ifreq));
    strncpy(ifreq.ifr_name, ifname.c_str(), sizeof(ifreq.ifr_name) - 1);

    // Set the address
    addr.copy_out(ifreq.ifr_addr);
    if (ioctl(_s4, SIOCSIFADDR, &ifreq) < 0) {
	errmsg = c_format("%s", strerror(errno));
	return (XORP_ERROR);
    }

    // Set the netmask
    IPvX::make_prefix(addr.af(), prefix_len).copy_out(ifreq.ifr_addr);
    if (ioctl(_s4, SIOCSIFNETMASK, &ifreq) < 0) {
	errmsg = c_format("%s", strerror(errno));
	return (XORP_ERROR);
    }

    // Set the p2p or broadcast address
    if (is_p2p) {
	dst_or_bcast.copy_out(ifreq.ifr_dstaddr);
	if (ioctl(_s4, SIOCSIFDSTADDR, &ifreq) < 0) {
	    errmsg = c_format("%s", strerror(errno));
	    return (XORP_ERROR);
	}
    } else {
	dst_or_bcast.copy_out(ifreq.ifr_broadaddr);
	if (ioctl(_s4, SIOCSIFBRDADDR, &ifreq) < 0) {
	    errmsg = c_format("%s", strerror(errno));
	    return (XORP_ERROR);
	}
    }
    return (XORP_OK);
}

/**
 * Add an IPv6 address to an interface.
 */
int
IfConfigSetIoctl::add_vif_address6(const string& ifname,
				   const string& vifname,
				   uint16_t if_index,
				   bool is_p2p,
				   const IPvX& addr,
				   const IPvX& dst,
				   uint32_t prefix_len,
				   string& errmsg)
{
    debug_msg("add_vif_address6 "
	      "(ifname = %s vifname = %s if_index = %u is_p2p = %s "
	      "addr = %s dst = %s prefix_len = %u)\n",
	      ifname.c_str(), vifname.c_str(), if_index,
	      (is_p2p)? "true" : "false", addr.str().c_str(),
	      dst.str().c_str(), prefix_len);

#ifndef HAVE_IPV6
    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(if_index);
    UNUSED(is_p2p);
    UNUSED(addr);
    UNUSED(dst);
    UNUSED(prefix_len);
    
    errmsg = "IPv6 is not supported";

    return (XORP_ERROR);

#else // HAVE_IPV6

    if (! ifc().have_ipv6()) {
	errmsg = "IPv6 is not supported";
	return (XORP_ERROR);
    }

#ifdef SIOCAIFADDR_IN6
    //
    // Add an alias address
    //
    struct in6_aliasreq ifra;

    UNUSED(vifname);
    UNUSED(if_index);

    memset(&ifra, 0, sizeof(ifra));
    strncpy(ifra.ifra_name, ifname.c_str(), sizeof(ifra.ifra_name) - 1);

    addr.copy_out(ifra.ifra_addr);
    if (is_p2p)
	dst.copy_out(ifra.ifra_dstaddr);
    IPvX::make_prefix(addr.af(), prefix_len).copy_out(ifra.ifra_prefixmask);
    ifra.ifra_lifetime.ia6t_vltime = ND6_INFINITE_LIFETIME;
    ifra.ifra_lifetime.ia6t_pltime = ND6_INFINITE_LIFETIME;

    if (ioctl(_s6, SIOCAIFADDR_IN6, &ifra) < 0) {
	errmsg = c_format("%s", strerror(errno));
	return (XORP_ERROR);
    }
    return (XORP_OK);

#else // !SIOCAIFADDR_IN6

    //
    // Set a new address
    //

    //
    // XXX: Linux uses a weird struct in6_ifreq to do this, and this
    // name clashes with the KAME in6_ifreq.
    // For now, we don't make this code specific only to Linux.
    //
    struct in6_ifreq in6_ifreq;

    UNUSED(ifname);
    UNUSED(vifname);

    memset(&in6_ifreq, 0, sizeof(in6_ifreq));
    in6_ifreq.ifr6_ifindex = if_index;

    // Set the address and the prefix length
    addr.copy_out(in6_ifreq.ifr6_addr);
    in6_ifreq.ifr6_prefixlen = prefix_len;
    if (ioctl(_s6, SIOCSIFADDR, &in6_ifreq) < 0) {
	errmsg = c_format("%s", strerror(errno));
	return (XORP_ERROR);
    }

    // Set the p2p address
    if (is_p2p) {
	dst.copy_out(in6_ifreq.ifr6_addr);
	if (ioctl(_s6, SIOCSIFDSTADDR, &in6_ifreq) < 0) {
	    errmsg = c_format("%s", strerror(errno));
	    return (XORP_ERROR);
	}
    }
    return (XORP_OK);
#endif // ! SIOCAIFADDR_IN6

#endif // HAVE_IPV6
}

int
IfConfigSetIoctl::delete_vif_address(const string& ifname,
				     const string& vifname,
				     uint16_t if_index,
				     const IPvX& addr,
				     uint32_t prefix_len,
				     string& errmsg)
{
    debug_msg("delete_vif_address "
	      "(ifname = %s vifname = %s if_index = %u addr = %s "
	      "prefix_len = %u)\n",
	      ifname.c_str(), vifname.c_str(), if_index, addr.str().c_str(),
	      prefix_len);

    // Check that the family is supported
    switch (addr.af()) {
    case AF_INET:
	if (! ifc().have_ipv4()) {
	    errmsg = "IPv4 is not supported";
	    return (XORP_ERROR);
	}
	break;

#ifdef HAVE_IPV6
    case AF_INET6:
	if (! ifc().have_ipv6()) {
	    errmsg = "IPv6 is not supported";
	    return (XORP_ERROR);
	}
	break;
#endif // HAVE_IPV6

    default:
	XLOG_UNREACHABLE();
	break;
    }

    switch (addr.af()) {
    case AF_INET:
    {
	struct ifreq ifreq;

	UNUSED(vifname);
	UNUSED(if_index);
	UNUSED(prefix_len);

	memset(&ifreq, 0, sizeof(ifreq));
	strncpy(ifreq.ifr_name, ifname.c_str(), sizeof(ifreq.ifr_name) - 1);

#ifndef HOST_OS_LINUX
	addr.copy_out(ifreq.ifr_addr);
	if (ioctl(_s4, SIOCDIFADDR, &ifreq) < 0) {
	    errmsg = c_format("%s", strerror(errno));
	    return (XORP_ERROR);
	}
#else // HOST_OS_LINUX
	// XXX: In case of Linux, SIOCDIFADDR doesn't delete IPv4 addresses.
	// Hence, we use SIOCSIFADDR to add 0.0.0.0 as an address. The
	// effect of this hack is that the IPv4 address on that interface
	// is deleted. Sigh...
	UNUSED(addr);
	IPv4::ZERO().copy_out(ifreq.ifr_addr);
	if (ioctl(_s4, SIOCSIFADDR, &ifreq) < 0) {
	    errmsg = c_format("%s", strerror(errno));
	    return (XORP_ERROR);
	}
#endif // HOST_OS_LINUX
	return (XORP_OK);
	break;
    }

#ifdef HAVE_IPV6
    case AF_INET6:
    {
#ifdef SIOCDIFADDR_IN6
	struct in6_ifreq in6_ifreq;

	UNUSED(if_index);
	UNUSED(prefix_len);

	memset(&in6_ifreq, 0, sizeof(in6_ifreq));
	strncpy(in6_ifreq.ifr_name, ifname.c_str(),
		sizeof(in6_ifreq.ifr_name) - 1);

	addr.copy_out(in6_ifreq.ifr_addr);

	if (ioctl(_s6, SIOCDIFADDR_IN6, &in6_ifreq) < 0) {
	    errmsg = c_format("%s", strerror(errno));
	    return (XORP_ERROR);
	}
	return (XORP_OK);

#else // !SIOCDIFADDR_IN6
	//
	// XXX: Linux uses a weird struct in6_ifreq to do this, and this
	// name clashes with the KAME in6_ifreq.
	// For now, we don't make this code specific only to Linux.
	//
	struct in6_ifreq in6_ifreq;

	UNUSED(ifname);

	memset(&in6_ifreq, 0, sizeof(in6_ifreq));
	in6_ifreq.ifr6_ifindex = if_index;
	addr.copy_out(in6_ifreq.ifr6_addr);
	in6_ifreq.ifr6_prefixlen = prefix_len;

	if (ioctl(_s6, SIOCDIFADDR, &in6_ifreq) < 0) {
	    errmsg = c_format("%s", strerror(errno));
	    return (XORP_ERROR);
	}
	return (XORP_OK);
#endif // ! SIOCDIFADDR_IN6

	break;
    }
#endif // HAVE_IPV6

    default:
	XLOG_UNREACHABLE();
	break;
    }

    return (XORP_OK);
}

#endif // HAVE_IOCTL_SIOCGIFCONF
