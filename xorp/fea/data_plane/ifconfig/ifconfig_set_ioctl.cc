// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
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

#include <xorp_config.h>
#if defined(HAVE_IOCTL_SIOCGIFCONF) && !defined(HAVE_NETLINK_SOCKETS)


#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ether_compat.h"
#include "libxorp/ipvx.hh"

#include "libcomm/comm_api.h"

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_NET_IF_ARP_H
#include <net/if_arp.h>
#endif
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

#include "fea/ifconfig.hh"

#include "ifconfig_set_ioctl.hh"


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

IfConfigSetIoctl::IfConfigSetIoctl(FeaDataPlaneManager& fea_data_plane_manager)
    : IfConfigSet(fea_data_plane_manager),
      _s4(-1),
      _s6(-1)
{
}

IfConfigSetIoctl::~IfConfigSetIoctl()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the ioctl(2) mechanism to set "
		   "information about network interfaces into the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
IfConfigSetIoctl::start(string& error_msg)
{
    if (_is_running)
	return (XORP_OK);

    if (fea_data_plane_manager().have_ipv4()) {
	if (_s4 < 0) {
	    _s4 = socket(AF_INET, SOCK_DGRAM, 0);
	    if (_s4 < 0) {
		error_msg = c_format("Could not initialize IPv4 ioctl() "
				     "socket: %s", strerror(errno));
		XLOG_FATAL("%s", error_msg.c_str());
	    }
	}
    }
    
#ifdef HAVE_IPV6
    if (fea_data_plane_manager().have_ipv6()) {
	if (_s6 < 0) {
	    _s6 = socket(AF_INET6, SOCK_DGRAM, 0);
	    if (_s6 < 0) {
		error_msg = c_format("Could not initialize IPv6 ioctl() "
				     "socket: %s", strerror(errno));
		XLOG_FATAL("%s", error_msg.c_str());
	    }
	}
    }
#endif // HAVE_IPV6

    _is_running = true;

    return (XORP_OK);
}

int
IfConfigSetIoctl::stop(string& error_msg)
{
    int ret_value4 = XORP_OK;
    int ret_value6 = XORP_OK;

    if (! _is_running)
	return (XORP_OK);

    if (_s4 >= 0) {
	ret_value4 = comm_close(_s4);
	_s4 = -1;
	if (ret_value4 != XORP_OK) {
	    error_msg = c_format("Could not close IPv4 ioctl() socket: %s",
				 comm_get_last_error_str());
	}
    }
    if (_s6 >= 0) {
	ret_value6 = comm_close(_s6);
	_s6 = -1;
	if ((ret_value6 != XORP_OK) && (ret_value4 == XORP_OK)) {
	    error_msg = c_format("Could not close IPv6 ioctl() socket: %s",
				 comm_get_last_error_str());
	}
    }

    if ((ret_value4 != XORP_OK) || (ret_value6 != XORP_OK))
	return (XORP_ERROR);

    _is_running = false;

    return (XORP_OK);
}

bool
IfConfigSetIoctl::is_discard_emulated(const IfTreeInterface& i) const
{
    UNUSED(i);

#if	defined(HOST_OS_BSDI)		\
	|| defined(HOST_OS_FREEBSD)	\
	|| defined(HOST_OS_MACOSX)	\
	|| defined(HOST_OS_NETBSD)	\
	|| defined(HOST_OS_OPENBSD)	\
	|| defined(HOST_OS_DRAGONFLYBSD)
    return (true);
#else
    return (false);
#endif
}

bool
IfConfigSetIoctl::is_unreachable_emulated(const IfTreeInterface& i) const
{
    UNUSED(i);

#if	defined(HOST_OS_BSDI)		\
	|| defined(HOST_OS_FREEBSD)	\
	|| defined(HOST_OS_MACOSX)	\
	|| defined(HOST_OS_NETBSD)	\
	|| defined(HOST_OS_OPENBSD)	\
	|| defined(HOST_OS_DRAGONFLYBSD)
    return (true);
#else
    return (false);
#endif
}

int
IfConfigSetIoctl::config_begin(string& error_msg)
{
    // XXX: nothing to do

    UNUSED(error_msg);

    return (XORP_OK);
}

int
IfConfigSetIoctl::config_end(string& error_msg)
{
    // XXX: nothing to do

    UNUSED(error_msg);

    return (XORP_OK);
}

int
IfConfigSetIoctl::config_interface_begin(const IfTreeInterface* pulled_ifp,
					 IfTreeInterface& config_iface,
					 string& error_msg)
{
    int ret_value = XORP_OK;
    bool was_disabled = false;
    bool should_disable = false;

    if (pulled_ifp == NULL) {
	// Nothing to do: the interface has been deleted from the system
	return (XORP_OK);
    }

#ifdef HOST_OS_LINUX
    //
    // XXX: Set the interface DOWN otherwise we may not be able to
    // set the MAC address or the MTU (limitation imposed by the Linux kernel).
    //
    if (pulled_ifp->enabled())
	should_disable = true;
#endif

    //
    // Set the MTU
    //
    if (config_iface.mtu() != pulled_ifp->mtu()) {
	if (should_disable && (! was_disabled)) {
	    if (set_interface_status(config_iface.ifname(),
				     config_iface.pif_index(),
				     config_iface.interface_flags(),
				     false,
				     error_msg)
		!= XORP_OK) {
		ret_value = XORP_ERROR;
		goto done;
	    }
	    was_disabled = true;
	}
	if (set_interface_mtu(config_iface.ifname(), config_iface.mtu(),
			      error_msg)
	    != XORP_OK) {
	    ret_value = XORP_ERROR;
	    goto done;
	}
    }

    //
    // Set the MAC address
    //
    if (config_iface.mac() != pulled_ifp->mac()) {
	if (should_disable && (! was_disabled)) {
	    if (set_interface_status(config_iface.ifname(),
				     config_iface.pif_index(),
				     config_iface.interface_flags(),
				     false,
				     error_msg)
		!= XORP_OK) {
		ret_value = XORP_ERROR;
		goto done;
	    }
	    was_disabled = true;
	}
	if (set_interface_mac_address(config_iface.ifname(),
				      config_iface.mac(),
				      error_msg)
	    != XORP_OK) {
	    ret_value = XORP_ERROR;
	    goto done;
	}
    }

 done:
    if (was_disabled) {
	if (set_interface_status(config_iface.ifname(),
				 config_iface.pif_index(),
				 config_iface.interface_flags(),
				 true,
				 error_msg)
	    != XORP_OK) {
	    return (XORP_ERROR);
	}
    }

    return (ret_value);
}

int
IfConfigSetIoctl::config_interface_end(const IfTreeInterface* pulled_ifp,
				       const IfTreeInterface& config_iface,
				       string& error_msg)
{
    if (pulled_ifp == NULL) {
	// Nothing to do: the interface has been deleted from the system
	return (XORP_OK);
    }

    //
    // Set the interface status
    //
    if (config_iface.enabled() != pulled_ifp->enabled()) {
	if (set_interface_status(config_iface.ifname(),
				 config_iface.pif_index(),
				 config_iface.interface_flags(),
				 config_iface.enabled(),
				 error_msg)
	    != XORP_OK) {
	    return (XORP_ERROR);
	}
    }

    return (XORP_OK);
}

int
IfConfigSetIoctl::config_vif_begin(const IfTreeInterface* pulled_ifp,
				   const IfTreeVif* pulled_vifp,
				   const IfTreeInterface& config_iface,
				   const IfTreeVif& config_vif,
				   string& error_msg)
{
    UNUSED(pulled_ifp);
    UNUSED(config_iface);
    UNUSED(config_vif);
    UNUSED(error_msg);

    if (pulled_vifp == NULL) {
	// Nothing to do: the vif has been deleted from the system
	return (XORP_OK);
    }

    // XXX: nothing to do

    return (XORP_OK);
}

int
IfConfigSetIoctl::config_vif_end(const IfTreeInterface* pulled_ifp,
				 const IfTreeVif* pulled_vifp,
				 const IfTreeInterface& config_iface,
				 const IfTreeVif& config_vif,
				 string& error_msg)
{
    UNUSED(pulled_ifp);

    if (pulled_vifp == NULL) {
	// Nothing to do: the vif has been deleted from the system
	return (XORP_OK);
    }

    //
    // XXX: If the interface name and vif name are different, then
    // they might have different status: the interface can be UP, while
    // the vif can be DOWN.
    //
    if (config_iface.ifname() != config_vif.vifname()) {
	//
	// Set the vif status
	//
	if (config_vif.enabled() != pulled_vifp->enabled()) {
	    //
	    // XXX: The interface and vif status setting mechanism is
	    // equivalent for this platform.
	    //
	    if (set_interface_status(config_vif.vifname(),
				     config_vif.pif_index(),
				     config_vif.vif_flags(),
				     config_vif.enabled(),
				     error_msg)
		!= XORP_OK) {
		return (XORP_ERROR);
	    }
	}
    }

    return (XORP_OK);
}

int
IfConfigSetIoctl::config_add_address(const IfTreeInterface* pulled_ifp,
				     const IfTreeVif* pulled_vifp,
				     const IfTreeAddr4* pulled_addrp,
				     const IfTreeInterface& config_iface,
				     const IfTreeVif& config_vif,
				     const IfTreeAddr4& config_addr,
				     string& error_msg)
{
    UNUSED(pulled_ifp);
    UNUSED(pulled_vifp);

    //
    // Test whether a new address
    //
    do {
	if (pulled_addrp == NULL)
	    break;
	if (pulled_addrp->addr() != config_addr.addr())
	    break;
	if (pulled_addrp->broadcast() != config_addr.broadcast())
	    break;
	if (pulled_addrp->broadcast()
	    && (pulled_addrp->bcast() != config_addr.bcast())) {
	    break;
	}
	if (pulled_addrp->point_to_point() != config_addr.point_to_point())
	    break;
	if (pulled_addrp->point_to_point()
	    && (pulled_addrp->endpoint() != config_addr.endpoint())) {
	    break;
	}
	if (pulled_addrp->prefix_len() != config_addr.prefix_len())
	    break;

	// XXX: Same address, therefore ignore it
	return (XORP_OK);
    } while (false);

    //
    // Delete the old address if necessary
    //
    if (pulled_addrp != NULL) {
	if (delete_addr(config_iface.ifname(), config_vif.vifname(),
			config_vif.pif_index(), config_addr.addr(),
			config_addr.prefix_len(), error_msg)
	    != XORP_OK) {
	    return (XORP_ERROR);
	}
    }

    //
    // Add the address
    //
    if (add_addr(config_iface.ifname(), config_vif.vifname(),
		 config_vif.pif_index(), config_addr.addr(),
		 config_addr.prefix_len(),
		 config_addr.broadcast(), config_addr.bcast(),
		 config_addr.point_to_point(), config_addr.endpoint(),
		 error_msg)
	!= XORP_OK) {
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
IfConfigSetIoctl::config_delete_address(const IfTreeInterface* pulled_ifp,
					const IfTreeVif* pulled_vifp,
					const IfTreeAddr4* pulled_addrp,
					const IfTreeInterface& config_iface,
					const IfTreeVif& config_vif,
					const IfTreeAddr4& config_addr,
					string& error_msg)
{
    UNUSED(pulled_ifp);
    UNUSED(pulled_vifp);
    UNUSED(pulled_addrp);

    //
    // Delete the address
    //
    if (delete_addr(config_iface.ifname(), config_vif.vifname(),
		    config_vif.pif_index(), config_addr.addr(),
		    config_addr.prefix_len(), error_msg)
	!= XORP_OK) {
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
IfConfigSetIoctl::config_add_address(const IfTreeInterface* pulled_ifp,
				     const IfTreeVif* pulled_vifp,
				     const IfTreeAddr6* pulled_addrp,
				     const IfTreeInterface& config_iface,
				     const IfTreeVif& config_vif,
				     const IfTreeAddr6& config_addr,
				     string& error_msg)
{
    UNUSED(pulled_ifp);
    UNUSED(pulled_vifp);

    //
    // Test whether a new address
    //
    do {
	if (pulled_addrp == NULL)
	    break;
	if (pulled_addrp->addr() != config_addr.addr())
	    break;
	if (pulled_addrp->point_to_point() != config_addr.point_to_point())
	    break;
	if (pulled_addrp->point_to_point()
	    && (pulled_addrp->endpoint() != config_addr.endpoint())) {
	    break;
	}
	if (pulled_addrp->prefix_len() != config_addr.prefix_len())
	    break;

	// XXX: Same address, therefore ignore it
	return (XORP_OK);
    } while (false);

    //
    // Delete the old address if necessary
    //
    if (pulled_addrp != NULL) {
	if (delete_addr(config_iface.ifname(), config_vif.vifname(),
			config_vif.pif_index(), config_addr.addr(),
			config_addr.prefix_len(), error_msg)
	    != XORP_OK) {
	    return (XORP_ERROR);
	}
    }

    //
    // Add the address
    //
    if (add_addr(config_iface.ifname(), config_vif.vifname(),
		 config_vif.pif_index(), config_addr.addr(),
		 config_addr.prefix_len(),
		 config_addr.point_to_point(), config_addr.endpoint(),
		 error_msg)
	!= XORP_OK) {
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
IfConfigSetIoctl::config_delete_address(const IfTreeInterface* pulled_ifp,
					const IfTreeVif* pulled_vifp,
					const IfTreeAddr6* pulled_addrp,
					const IfTreeInterface& config_iface,
					const IfTreeVif& config_vif,
					const IfTreeAddr6& config_addr,
					string& error_msg)
{
    UNUSED(pulled_ifp);
    UNUSED(pulled_vifp);
    UNUSED(pulled_addrp);

    //
    // Delete the address
    //
    if (delete_addr(config_iface.ifname(), config_vif.vifname(),
		    config_vif.pif_index(), config_addr.addr(),
		    config_addr.prefix_len(), error_msg)
	!= XORP_OK) {
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
IfConfigSetIoctl::set_interface_status(const string& ifname,
				       uint32_t if_index,
				       uint32_t interface_flags,
				       bool is_enabled,
				       string& error_msg)
{
    struct ifreq ifreq;

    UNUSED(if_index);

    //
    // Update the interface flags
    //
    if (is_enabled)
	interface_flags |= IFF_UP;
    else
	interface_flags &= ~IFF_UP;

    memset(&ifreq, 0, sizeof(ifreq));
    strncpy(ifreq.ifr_name, ifname.c_str(), sizeof(ifreq.ifr_name) - 1);
    ifreq.ifr_flags = interface_flags;

    if (ioctl(_s4, SIOCSIFFLAGS, &ifreq) < 0) {
	error_msg = c_format("Cannot set the interface flags to 0x%x on "
			     "interface %s: %s",
			     interface_flags, ifname.c_str(), strerror(errno));
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
IfConfigSetIoctl::set_interface_mac_address(const string& ifname,
					    const Mac& mac,
					    string& error_msg)
{
    struct ether_addr ether_addr;
    struct ifreq ifreq;

    mac.copy_out(ether_addr);
    memset(&ifreq, 0, sizeof(ifreq));
    strncpy(ifreq.ifr_name, ifname.c_str(), sizeof(ifreq.ifr_name) - 1);

#if defined(SIOCSIFLLADDR)
    //
    // FreeBSD
    //
    ifreq.ifr_addr.sa_family = AF_LINK;
    memcpy(ifreq.ifr_addr.sa_data, &ether_addr, sizeof(ether_addr));
#ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
    ifreq.ifr_addr.sa_len = sizeof(ether_addr);
#endif
    if (ioctl(_s4, SIOCSIFLLADDR, &ifreq) < 0) {
	error_msg = c_format("Cannot set the MAC address to %s "
			     "on interface %s: %s",
			     mac.str().c_str(),
			     ifname.c_str(),
			     strerror(errno));
	return (XORP_ERROR);
    }

    return (XORP_OK);

#elif defined(SIOCSIFHWADDR)
    //
    // Linux
    //
    ifreq.ifr_hwaddr.sa_family = ARPHRD_ETHER;
    memcpy(ifreq.ifr_hwaddr.sa_data, &ether_addr, ETH_ALEN);
#ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
    ifreq.ifr_hwaddr.sa_len = ETH_ALEN;
#endif
    if (ioctl(_s4, SIOCSIFHWADDR, &ifreq) < 0) {
	error_msg = c_format("Cannot set the MAC address %s "
			     "on interface %s: %s",
			     mac.str().c_str(),
			     ifname.c_str(),
			     strerror(errno));
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
    error_msg = c_format("No mechanism to set the MAC address "
			 "on an interface");
    return (XORP_ERROR);
#endif
}

int
IfConfigSetIoctl::set_interface_mtu(const string& ifname,
				    uint32_t mtu,
				    string& error_msg)
{
    struct ifreq ifreq;

    memset(&ifreq, 0, sizeof(ifreq));
    strncpy(ifreq.ifr_name, ifname.c_str(), sizeof(ifreq.ifr_name) - 1);
    ifreq.ifr_mtu = mtu;

    if (ioctl(_s4, SIOCSIFMTU, &ifreq) < 0) {
	error_msg = c_format("Cannot set the MTU to %u on "
			     "interface %s: %s",
			     mtu, ifname.c_str(), strerror(errno));
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
IfConfigSetIoctl::add_addr(const string& ifname, const string& vifname,
			   uint32_t if_index, const IPv4& addr,
			   uint32_t prefix_len,
			   bool is_broadcast, const IPv4& broadcast_addr,
			   bool is_point_to_point, const IPv4& endpoint_addr,
			   string& error_msg)
{
    //
    // XXX: If the system has ioctl(SIOCAIFADDR) (e.g., FreeBSD), then
    // an interface address is added as an alias, otherwise it overwrites the
    // previous address (if such exists).
    //

#if defined(SIOCAIFADDR)
    //
    // Add an alias address
    //
    struct in_aliasreq ifra;

    UNUSED(if_index);

    memset(&ifra, 0, sizeof(ifra));
    strncpy(ifra.ifra_name, vifname.c_str(), sizeof(ifra.ifra_name) - 1);
    addr.copy_out(ifra.ifra_addr);
    if (is_broadcast)
	broadcast_addr.copy_out(ifra.ifra_broadaddr);
    if (is_point_to_point)
	endpoint_addr.copy_out(ifra.ifra_dstaddr);
    IPv4 prefix_addr = IPv4::make_prefix(prefix_len);
    prefix_addr.copy_out(ifra.ifra_mask);

    if (ioctl(_s4, SIOCAIFADDR, &ifra) < 0) {
	error_msg = c_format("Cannot add address '%s' "
			     "on interface '%s' vif '%s': %s",
			     addr.str().c_str(),
			     ifname.c_str(), vifname.c_str(), strerror(errno));
	return (XORP_ERROR);
    }

    return (XORP_OK);

#elif defined(SIOCSIFADDR)
    //
    // Set a new address
    //
    struct ifreq ifreq;

    UNUSED(if_index);

    memset(&ifreq, 0, sizeof(ifreq));
    strncpy(ifreq.ifr_name, vifname.c_str(), sizeof(ifreq.ifr_name) - 1);

    // Set the address
    addr.copy_out(ifreq.ifr_addr);
    if (ioctl(_s4, SIOCSIFADDR, &ifreq) < 0) {
	error_msg = c_format("Cannot add address '%s' "
			     "on interface '%s' vif '%s': %s",
			     addr.str().c_str(),
			     ifname.c_str(), vifname.c_str(), strerror(errno));
	return (XORP_ERROR);
    }

    // Set the netmask
    IPv4 prefix_addr = IPv4::make_prefix(prefix_len);
    prefix_addr.copy_out(ifreq.ifr_addr);
    if (ioctl(_s4, SIOCSIFNETMASK, &ifreq) < 0) {
	error_msg = c_format("Cannot add network mask '%s' to address '%s' "
			     "on interface '%s' vif '%s': %s",
			     prefix_addr.str().c_str(),
			     addr.str().c_str(),
			     ifname.c_str(), vifname.c_str(), strerror(errno));
	return (XORP_ERROR);
    }

    // Set the broadcast or point-to-point address
    if (is_broadcast) {
	broadcast_addr.copy_out(ifreq.ifr_broadaddr);
	if (ioctl(_s4, SIOCSIFBRDADDR, &ifreq) < 0) {
	    error_msg = c_format("Cannot add broadcast address '%s' "
				 "to address '%s' "
				 "on interface '%s' vif '%s': %s",
				 broadcast_addr.str().c_str(),
				 addr.str().c_str(),
				 ifname.c_str(), vifname.c_str(),
				 strerror(errno));
	    return (XORP_ERROR);
	}
    }
    if (is_point_to_point) {
	endpoint_addr.copy_out(ifreq.ifr_dstaddr);
	if (ioctl(_s4, SIOCSIFDSTADDR, &ifreq) < 0) {
	    error_msg = c_format("Cannot add endpoint address '%s' "
				 "to address '%s' "
				 "on interface '%s' vif '%s': %s",
				 endpoint_addr.str().c_str(),
				 addr.str().c_str(),
				 ifname.c_str(), vifname.c_str(),
				 strerror(errno));
	    return (XORP_ERROR);
	}
    }

    return (XORP_OK);

#else
    //
    // No mechanism to add the address
    //
    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(if_index);
    UNUSED(addr);
    UNUSED(prefix_len);
    UNUSED(is_broadcast);
    UNUSED(broadcast_addr);
    UNUSED(is_point_to_point);
    UNUSED(endpoint_addr);

    error_msg = c_format("No mechanism to add an IPv4 address "
			 "on an interface");
    return (XORP_ERROR);
#endif
}

int
IfConfigSetIoctl::delete_addr(const string& ifname, const string& vifname,
			      uint32_t if_index, const IPv4& addr,
			      uint32_t prefix_len, string& error_msg)
{
    struct ifreq ifreq;

    memset(&ifreq, 0, sizeof(ifreq));
    strncpy(ifreq.ifr_name, vifname.c_str(), sizeof(ifreq.ifr_name) - 1);

#if defined(HOST_OS_LINUX)
    //
    // XXX: In case of Linux, SIOCDIFADDR doesn't delete IPv4 addresses.
    // Hence, we use SIOCSIFADDR to add 0.0.0.0 as an address. The
    // effect of this hack is that the IPv4 address on that interface
    // is deleted. Sigh...
    //
    UNUSED(if_index);
    UNUSED(prefix_len);

    IPv4::ZERO().copy_out(ifreq.ifr_addr);
    if (ioctl(_s4, SIOCSIFADDR, &ifreq) < 0) {
	error_msg = c_format("Cannot delete address '%s' "
			     "on interface '%s' vif '%s': %s",
			     addr.str().c_str(),
			     ifname.c_str(), vifname.c_str(),
			     strerror(errno));
	return (XORP_ERROR);
    }

    return (XORP_OK);

#elif defined(SIOCDIFADDR)
    UNUSED(if_index);
    UNUSED(prefix_len);

    addr.copy_out(ifreq.ifr_addr);
    if (ioctl(_s4, SIOCDIFADDR, &ifreq) < 0) {
	error_msg = c_format("Cannot delete address '%s' "
			     "on interface '%s' vif '%s': %s",
			     addr.str().c_str(),
			     ifname.c_str(), vifname.c_str(),
			     strerror(errno));
	return (XORP_ERROR);
    }

    return (XORP_OK);

#else
    //
    // No mechanism to delete the address
    //
    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(if_index);
    UNUSED(addr);
    UNUSED(prefix_len);

    error_msg = c_format("No mechanism to delete an IPv4 address "
			 "on an interface");
    return (XORP_ERROR);
#endif
}

int
IfConfigSetIoctl::add_addr(const string& ifname, const string& vifname,
			   uint32_t if_index, const IPv6& addr,
			   uint32_t prefix_len, bool is_point_to_point,
			   const IPv6& endpoint_addr, string& error_msg)

{
#ifndef HAVE_IPV6
    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(if_index);
    UNUSED(addr);
    UNUSED(prefix_len);
    UNUSED(is_point_to_point);
    UNUSED(endpoint_addr);

    error_msg = "IPv6 is not supported";

    return (XORP_ERROR);

#else // HAVE_IPV6

#if defined(SIOCAIFADDR_IN6)
    //
    // Add an alias address
    //
    struct in6_aliasreq ifra;

    UNUSED(if_index);

    memset(&ifra, 0, sizeof(ifra));
    strncpy(ifra.ifra_name, vifname.c_str(), sizeof(ifra.ifra_name) - 1);
    addr.copy_out(ifra.ifra_addr);
    if (is_point_to_point)
	endpoint_addr.copy_out(ifra.ifra_dstaddr);
    IPv6 prefix_addr = IPv6::make_prefix(prefix_len);
    prefix_addr.copy_out(ifra.ifra_prefixmask);
    ifra.ifra_lifetime.ia6t_vltime = ND6_INFINITE_LIFETIME;
    ifra.ifra_lifetime.ia6t_pltime = ND6_INFINITE_LIFETIME;
    if (ioctl(_s6, SIOCAIFADDR_IN6, &ifra) < 0) {
	error_msg = c_format("Cannot add address '%s' "
			     "on interface '%s' vif '%s': %s",
			     addr.str().c_str(),
			     ifname.c_str(), vifname.c_str(), strerror(errno));
	return (XORP_ERROR);
    }

    return (XORP_OK);

#elif defined(SIOCSIFADDR)
    //
    // Set a new address
    //

    //
    // XXX: Linux uses a weird struct in6_ifreq to do this, and this
    // name clashes with the KAME in6_ifreq.
    // For now, we don't make this code specific only to Linux.
    //
    struct in6_ifreq in6_ifreq;

    memset(&in6_ifreq, 0, sizeof(in6_ifreq));
    in6_ifreq.ifr6_ifindex = if_index;

    // Set the address and the prefix length
    addr.copy_out(in6_ifreq.ifr6_addr);
    in6_ifreq.ifr6_prefixlen = prefix_len;
    if (ioctl(_s6, SIOCSIFADDR, &in6_ifreq) < 0) {
	error_msg = c_format("Cannot add address '%s' "
			     "on interface '%s' vif '%s': %s",
			     addr.str().c_str(),
			     ifname.c_str(), vifname.c_str(), strerror(errno));
	return (XORP_ERROR);
    }

    // Set the p2p address
    if (is_point_to_point) {
	endpoint_addr.copy_out(in6_ifreq.ifr6_addr);
	if (ioctl(_s6, SIOCSIFDSTADDR, &in6_ifreq) < 0) {
	    error_msg = c_format("Cannot add endpoint address '%s' "
				 "to address '%s' "
				 "on interface '%s' vif '%s': %s",
				 endpoint_addr.str().c_str(),
				 addr.str().c_str(),
				 ifname.c_str(), vifname.c_str(),
				 strerror(errno));
	    return (XORP_ERROR);
	}
    }

    return (XORP_OK);

#else
    //
    // No mechanism to add the address
    //
    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(if_index);
    UNUSED(addr);
    UNUSED(prefix_len);
    UNUSED(is_point_to_point);
    UNUSED(endpoint_addr);

    error_msg = c_format("No mechanism to add an IPv6 address "
			 "on an interface");
    return (XORP_ERROR);
#endif
#endif // HAVE_IPV6
}

int
IfConfigSetIoctl::delete_addr(const string& ifname, const string& vifname,
			      uint32_t if_index, const IPv6& addr,
			      uint32_t prefix_len, string& error_msg)
{
#ifndef HAVE_IPV6
    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(if_index);
    UNUSED(addr);
    UNUSED(prefix_len);

    error_msg = "IPv6 is not supported";

    return (XORP_ERROR);

#else // HAVE_IPV6

#if defined(HOST_OS_LINUX)
    //
    // XXX: Linux uses a weird struct in6_ifreq to do this, and this
    // name clashes with the KAME in6_ifreq.
    //
    struct in6_ifreq in6_ifreq;

    memset(&in6_ifreq, 0, sizeof(in6_ifreq));
    in6_ifreq.ifr6_ifindex = if_index;
    addr.copy_out(in6_ifreq.ifr6_addr);
    in6_ifreq.ifr6_prefixlen = prefix_len;

    if (ioctl(_s6, SIOCDIFADDR, &in6_ifreq) < 0) {
	error_msg = c_format("Cannot delete address '%s' "
			     "on interface '%s' vif '%s': %s",
			     addr.str().c_str(),
			     ifname.c_str(), vifname.c_str(), strerror(errno));
	return (XORP_ERROR);
    }

    return (XORP_OK);

#elif defined(SIOCDIFADDR_IN6)
    struct in6_ifreq in6_ifreq;

    UNUSED(if_index);
    UNUSED(prefix_len);

    memset(&in6_ifreq, 0, sizeof(in6_ifreq));
    strncpy(in6_ifreq.ifr_name, vifname.c_str(),
	    sizeof(in6_ifreq.ifr_name) - 1);
    addr.copy_out(in6_ifreq.ifr_addr);

    if (ioctl(_s6, SIOCDIFADDR_IN6, &in6_ifreq) < 0) {
	error_msg = c_format("Cannot delete address '%s' "
			     "on interface '%s' vif '%s': %s",
			     addr.str().c_str(),
			     ifname.c_str(), vifname.c_str(), strerror(errno));
	return (XORP_ERROR);
    }

    return (XORP_OK);

#else
    //
    // No mechanism to delete the address
    //
    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(if_index);
    UNUSED(addr);
    UNUSED(prefix_len);

    error_msg = c_format("No mechanism to delete an IPv6 address "
			 "on an interface");
    return (XORP_ERROR);
#endif
#endif // HAVE_IPV6
}

#endif // HAVE_IOCTL_SIOCGIFCONF
