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



#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ether_compat.h"

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
#ifdef HAVE_NET_IF_VAR_H
#include <net/if_var.h>
#endif
#ifdef HAVE_NET_IF_DL_H
#include <net/if_dl.h>
#endif
#ifdef HAVE_NET_IF_TYPES_H
#include <net/if_types.h>
#endif
#ifdef HAVE_NETINET6_IN6_VAR_H
#include <netinet6/in6_var.h>
#endif

#include "fea/ifconfig.hh"
#include "fea/data_plane/control_socket/system_utilities.hh"

#include "ifconfig_get_ioctl.hh"
#include "ifconfig_media.hh"


//
// Parse information about network interface configuration change from
// the underlying system.
//
// The information to parse is in "struct ifreq" format
// (e.g., obtained by ioctl(SIOCGIFCONF) mechanism).
//

#ifdef HAVE_IOCTL_SIOCGIFCONF

int
IfConfigGetIoctl::parse_buffer_ioctl(IfConfig& ifconfig, IfTree& iftree,
				     int family, const vector<uint8_t>& buffer)
{
#ifndef HOST_OS_WINDOWS
    int s;
    uint32_t if_index = 0;
    string if_name, alias_if_name;
    size_t offset;

    UNUSED(ifconfig);

    s = socket(family, SOCK_DGRAM, 0);
    if (s < 0) {
	XLOG_FATAL("Could not initialize ioctl() socket");
    }

    for (offset = 0; offset < buffer.size(); ) {
	bool is_newlink = false;	// True if really a new link
	size_t len = 0;
	struct ifreq ifreq, ifrcopy;

	memcpy(&ifreq, &buffer[offset], sizeof(ifreq));
	
	// Get the length of the ifreq entry
#ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
	len = max(sizeof(struct sockaddr),
		  static_cast<size_t>(ifreq.ifr_addr.sa_len));
#else
	switch (ifreq.ifr_addr.sa_family) {
#ifdef HAVE_IPV6
	case AF_INET6:
	    len = sizeof(struct sockaddr_in6);
	    break;
#endif // HAVE_IPV6
	case AF_INET:
	default:
	    len = sizeof(struct sockaddr);
	    break;
	}
#endif // HAVE_STRUCT_SOCKADDR_SA_LEN
	len += sizeof(ifreq.ifr_name);
	len = max(len, sizeof(struct ifreq));
	offset += len;				// Point to the next entry
	
	//
	// Get the interface name
	//
	char tmp_if_name[IFNAMSIZ+1];
	strncpy(tmp_if_name, ifreq.ifr_name, sizeof(tmp_if_name) - 1);
	tmp_if_name[sizeof(tmp_if_name) - 1] = '\0';
	char* cptr;
	if ( (cptr = strchr(tmp_if_name, ':')) != NULL) {
	    // Replace colon with null. Needed because in Solaris and Linux
	    // the interface name changes for aliases.
	    *cptr = '\0';
	}
	if_name = string(ifreq.ifr_name);
	alias_if_name = string(tmp_if_name);
	debug_msg("interface: %s\n", if_name.c_str());
	debug_msg("alias interface: %s\n", alias_if_name.c_str());
	
	//
	// Get the physical interface index
	//
	do {
	    if_index = 0;
	    
#ifdef HAVE_IF_NAMETOINDEX
	    if_index = if_nametoindex(if_name.c_str());
	    if (if_index > 0)
		break;
#endif // HAVE_IF_NAMETOINDEX
	    
#ifdef SIOCGIFINDEX
	    {
		struct ifreq ifridx;
		
		memset(&ifridx, 0, sizeof(ifridx));
		strncpy(ifridx.ifr_name, if_name.c_str(),
			sizeof(ifridx.ifr_name) - 1);
		if (ioctl(s, SIOCGIFINDEX, &ifridx) < 0) {
		    XLOG_ERROR("ioctl(SIOCGIFINDEX) for interface %s failed: %s",
			       if_name.c_str(), strerror(errno));
		} else {
#ifdef HAVE_STRUCT_IFREQ_IFR_IFINDEX
		    if_index = ifridx.ifr_ifindex;
#else
		    if_index = ifridx.ifr_index;
#endif
		}
	    }
	    if (if_index > 0)
		break;
#endif // SIOCGIFINDEX
	    
	    break;
	} while (false);
	if (if_index == 0) {
	    XLOG_FATAL("Could not find physical interface index "
		       "for interface %s",
		       if_name.c_str());
	}
	debug_msg("interface index: %u\n", if_index);
	
	//
	// Add the interface (if a new one)
	//
	IfTreeInterface* ifp = iftree.find_interface(alias_if_name);
	if (ifp == NULL) {
	    iftree.add_interface(alias_if_name);
	    is_newlink = true;
	    ifp = iftree.find_interface(alias_if_name);
	    XLOG_ASSERT(ifp != NULL);
	}

	//
	// Set the physical interface index for the interface
	//
	if (is_newlink || (if_index != ifp->pif_index()))
	    ifp->set_pif_index(if_index);
	
	//
	// Get the MAC address
	//
	do {
#ifdef AF_LINK
	    const struct sockaddr* sa = &ifreq.ifr_addr;
	    if (sa->sa_family == AF_LINK) {
		const struct sockaddr_dl* sdl = reinterpret_cast<const struct sockaddr_dl*>(sa);
		if (sdl->sdl_type == IFT_ETHER) {
		    if (sdl->sdl_alen == sizeof(struct ether_addr)) {
			struct ether_addr ea;
			memcpy(&ea, sdl->sdl_data + sdl->sdl_nlen,
			       sdl->sdl_alen);
			Mac mac(ea);
			if (is_newlink || (mac != ifp->mac()))
			    ifp->set_mac(mac);
			break;
		    } else if (sdl->sdl_alen != 0) {
			XLOG_ERROR("Address size %d uncatered for interface %s",
				   sdl->sdl_alen, if_name.c_str());
		    }
		}
	    }
#endif // AF_LINK
	    
#ifdef SIOCGIFHWADDR
	    memcpy(&ifrcopy, &ifreq, sizeof(ifrcopy));
	    if (ioctl(s, SIOCGIFHWADDR, &ifrcopy) < 0) {
		XLOG_ERROR("ioctl(SIOCGIFHWADDR) for interface %s failed: %s",
			   if_name.c_str(), strerror(errno));
	    } else {
		struct ether_addr ea;
		memcpy(&ea, ifrcopy.ifr_hwaddr.sa_data, sizeof(ea));
		Mac mac(ea);
		if (is_newlink || (mac != ifp->mac()))
		    ifp->set_mac(mac);
		break;
	    }
#endif // SIOCGIFHWADDR
	    
	    break;
	} while (false);
	debug_msg("MAC address: %s\n", ifp->mac().str().c_str());
	
	//
	// Get the MTU
	//
	unsigned int mtu = 0;
	memcpy(&ifrcopy, &ifreq, sizeof(ifrcopy));
	if (ioctl(s, SIOCGIFMTU, &ifrcopy) < 0) {
	    XLOG_ERROR("ioctl(SIOCGIFMTU) for interface %s failed: %s",
		       if_name.c_str(), strerror(errno));
	} else {
#ifndef HOST_OS_SOLARIS
	    mtu = ifrcopy.ifr_mtu;
#else
	    //
	    // XXX: Solaris supports ioctl(SIOCGIFMTU), but stores the MTU
	    // in the ifr_metric field instead of ifr_mtu.
	    //
	    mtu = ifrcopy.ifr_metric;
#endif // HOST_OS_SOLARIS
	}
	if (is_newlink || (mtu != ifp->mtu()))
	    ifp->set_mtu(mtu);
	debug_msg("MTU: %u\n", XORP_UINT_CAST(ifp->mtu()));
	
	//
	// Get the flags
	//
	unsigned int flags = 0;
	memcpy(&ifrcopy, &ifreq, sizeof(ifrcopy));
	if (ioctl(s, SIOCGIFFLAGS, &ifrcopy) < 0) {
	    XLOG_ERROR("ioctl(SIOCGIFFLAGS) for interface %s failed: %s",
		       if_name.c_str(), strerror(errno));
	} else {
	    flags = ifrcopy.ifr_flags;
	}
	if (is_newlink || (flags != ifp->interface_flags())) {
	    ifp->set_interface_flags(flags);
	    ifp->set_enabled(flags & IFF_UP);
	}
	debug_msg("enabled: %s\n", bool_c_str(ifp->enabled()));

	//
	// Get the link status and baudrate
	//
	do {
	    bool no_carrier = false;
	    uint64_t baudrate = 0;
	    string error_msg;

	    if (ifconfig_media_get_link_status(if_name, no_carrier, baudrate,
					       error_msg)
		!= XORP_OK) {
		// XXX: No other method to retrieve the information
	    }
	    if (is_newlink || (no_carrier != ifp->no_carrier()))
		ifp->set_no_carrier(no_carrier);
	    if (is_newlink || (baudrate != ifp->baudrate()))
		ifp->set_baudrate(baudrate);
	    break;
	} while (false);
	debug_msg("no_carrier: %s\n", bool_c_str(ifp->no_carrier()));
	debug_msg("baudrate: %u\n", XORP_UINT_CAST(ifp->baudrate()));

	// XXX: vifname == ifname on this platform
	if (is_newlink)
	    ifp->add_vif(alias_if_name);
	IfTreeVif* vifp = ifp->find_vif(alias_if_name);
	XLOG_ASSERT(vifp != NULL);
	
	//
	// Set the physical interface index for the vif
	//
	if (is_newlink || (if_index != vifp->pif_index()))
	    vifp->set_pif_index(if_index);
	
	//
	// Set the vif flags
	//
	if (is_newlink || (flags != vifp->vif_flags())) {
	    vifp->set_vif_flags(flags);
	    vifp->set_enabled(ifp->enabled() && (flags & IFF_UP));
	    vifp->set_broadcast(flags & IFF_BROADCAST);
	    vifp->set_loopback(flags & IFF_LOOPBACK);
#ifdef IFF_POINTOPOINT
	    vifp->set_point_to_point(flags & IFF_POINTOPOINT);
#endif
	    vifp->set_multicast(flags & IFF_MULTICAST);
	    // Propagate the flags to the existing addresses
	    vifp->propagate_flags_to_addresses();
	}
	debug_msg("vif enabled: %s\n", bool_c_str(vifp->enabled()));
	debug_msg("vif broadcast: %s\n", bool_c_str(vifp->broadcast()));
	debug_msg("vif loopback: %s\n", bool_c_str(vifp->loopback()));
	debug_msg("vif point_to_point: %s\n",
		  bool_c_str(vifp->point_to_point()));
	debug_msg("vif multicast: %s\n", bool_c_str(vifp->multicast()));
	
	//
	// Get the interface addresses for the same address family only.
	// XXX: if the address family is zero, then we query the address.
	//
	if ((ifreq.ifr_addr.sa_family != family)
	    && (ifreq.ifr_addr.sa_family != 0)) {
	    continue;
	}
	
	//
	// Get the IP address, netmask, broadcast address, P2P destination
	//
        // The default values
	IPvX lcl_addr = IPvX::ZERO(family);
	IPvX subnet_mask = IPvX::ZERO(family);
	IPvX broadcast_addr = IPvX::ZERO(family);
	IPvX peer_addr = IPvX::ZERO(family);
	bool has_broadcast_addr = false;
	bool has_peer_addr = false;
	
	struct ifreq ip_ifrcopy;
	memcpy(&ip_ifrcopy, &ifreq, sizeof(ip_ifrcopy));
	ip_ifrcopy.ifr_addr.sa_family = family;
#ifdef SIOCGIFADDR_IN6
	struct in6_ifreq ip_ifrcopy6;
	memcpy(&ip_ifrcopy6, &ifreq, sizeof(ip_ifrcopy6));
	ip_ifrcopy6.ifr_ifru.ifru_addr.sin6_family = family;
#endif // SIOCGIFADDR_IN6
	
	// Get the IP address
	if (ifreq.ifr_addr.sa_family == family) {
	    lcl_addr.copy_in(ifreq.ifr_addr);
	    memcpy(&ip_ifrcopy, &ifreq, sizeof(ip_ifrcopy));
#ifdef SIOCGIFADDR_IN6
	    memcpy(&ip_ifrcopy6, &ifreq, sizeof(ip_ifrcopy6));
#endif
	} else {
	    // XXX: we need to query the local IP address
	    XLOG_ASSERT(ifreq.ifr_addr.sa_family == 0);
	    
	    switch (family) {
	    case AF_INET:
#ifdef SIOCGIFADDR
		memset(&ifrcopy, 0, sizeof(ifrcopy));
		strncpy(ifrcopy.ifr_name, if_name.c_str(),
			sizeof(ifrcopy.ifr_name) - 1);
		ifrcopy.ifr_addr.sa_family = family;
		if (ioctl(s, SIOCGIFADDR, &ifrcopy) < 0) {
		    // XXX: the interface probably has no address. Ignore.
		    continue;
		} else {
		    lcl_addr.copy_in(ifrcopy.ifr_addr);
		    memcpy(&ip_ifrcopy, &ifrcopy, sizeof(ip_ifrcopy));
		}
#endif // SIOCGIFADDR
		break;
		
#ifdef HAVE_IPV6
	    case AF_INET6:
#ifdef SIOCGIFADDR_IN6
	    {
		struct in6_ifreq ifrcopy6;
		
		memset(&ifrcopy6, 0, sizeof(ifrcopy6));
		strncpy(ifrcopy6.ifr_name, if_name.c_str(),
			sizeof(ifrcopy6.ifr_name) - 1);
		ifrcopy6.ifr_ifru.ifru_addr.sin6_family = family;
		if (ioctl(s, SIOCGIFADDR_IN6, &ifrcopy6) < 0) {
		    XLOG_ERROR("ioctl(SIOCGIFADDR_IN6) failed: %s",
			       strerror(errno));
		} else {
		    lcl_addr.copy_in(ifrcopy6.ifr_ifru.ifru_addr);
		    memcpy(&ip_ifrcopy6, &ifrcopy6, sizeof(ip_ifrcopy6));
		}
	    }
#endif // SIOCGIFADDR_IN6
	    break;
#endif // HAVE_IPV6
	    
	    default:
		XLOG_UNREACHABLE();
		break;
	    }
	}
	lcl_addr = system_adjust_ipvx_recv(lcl_addr);
	debug_msg("IP address: %s\n", lcl_addr.str().c_str());
	
	// Get the netmask
	switch (family) {
	case AF_INET:
#ifdef SIOCGIFNETMASK
	    memcpy(&ifrcopy, &ip_ifrcopy, sizeof(ifrcopy));
	    if (ioctl(s, SIOCGIFNETMASK, &ifrcopy) < 0) {
		if (! vifp->point_to_point()) {
		    XLOG_ERROR("ioctl(SIOCGIFNETMASK) failed: %s",
			       strerror(errno));
		}
	    } else {
		// The interface is configured
		// XXX: SIOCGIFNETMASK doesn't return proper family
		ifrcopy.ifr_addr.sa_family = family;
		subnet_mask.copy_in(ifrcopy.ifr_addr);
	    }
#endif // SIOCGIFNETMASK
	    break;
	    
#ifdef HAVE_IPV6
	case AF_INET6:
#ifdef SIOCGIFNETMASK_IN6
	{
	    struct in6_ifreq ifrcopy6;
	    
	    memcpy(&ifrcopy6, &ip_ifrcopy6, sizeof(ifrcopy6));
	    if (ioctl(s, SIOCGIFNETMASK_IN6, &ifrcopy6) < 0) {
		if (! vifp->point_to_point()) {
		    XLOG_ERROR("ioctl(SIOCGIFNETMASK_IN6) failed: %s",
			       strerror(errno));
		}
	    } else {
		// The interface is configured
		// XXX: SIOCGIFNETMASK_IN6 doesn't return proper family
		ifrcopy6.ifr_ifru.ifru_addr.sin6_family = family;
		subnet_mask.copy_in(ifrcopy6.ifr_ifru.ifru_addr);
	    }
	}
#endif // SIOCGIFNETMASK_IN6
	break;
#endif // HAVE_IPV6
	
	default:
	    XLOG_UNREACHABLE();
	    break;
	}
	debug_msg("IP netmask: %s\n", subnet_mask.str().c_str());
	
	// Get the broadcast address	
	if (vifp->broadcast()) {
	    switch (family) {
	    case AF_INET:
#ifdef SIOCGIFBRDADDR
		memcpy(&ifrcopy, &ip_ifrcopy, sizeof(ifrcopy));
		if (ioctl(s, SIOCGIFBRDADDR, &ifrcopy) < 0) {
		    XLOG_ERROR("ioctl(SIOCGIFBRADDR) failed: %s",
			       strerror(errno));
		} else {
		    // XXX: in case it doesn't return proper family
		    ifrcopy.ifr_broadaddr.sa_family = family;
		    broadcast_addr.copy_in(ifrcopy.ifr_addr);
		    has_broadcast_addr = true;
		}
#endif // SIOCGIFBRDADDR
		break;
		
#ifdef HAVE_IPV6
	    case AF_INET6:
		break;	// IPv6 doesn't have the idea of broadcast
#endif // HAVE_IPV6
		
	    default:
		XLOG_UNREACHABLE();
		break;
	    }
	    debug_msg("Broadcast address: %s\n", broadcast_addr.str().c_str());
	}
	
	// Get the p2p address
	if (vifp->point_to_point()) {
	    switch (family) {
	    case AF_INET:
#ifdef SIOCGIFDSTADDR
		memcpy(&ifrcopy, &ip_ifrcopy, sizeof(ifrcopy));
		if (ioctl(s, SIOCGIFDSTADDR, &ifrcopy) < 0) {
		    // Probably the p2p address is not configured
		} else {
		    // The p2p address is configured
		    // XXX: in case it doesn't return proper family
		    ifrcopy.ifr_dstaddr.sa_family = family;
		    peer_addr.copy_in(ifrcopy.ifr_dstaddr);
		    has_peer_addr = true;
		}
#endif // SIOCGIFDSTADDR
		break;
		
#ifdef HAVE_IPV6
	    case AF_INET6:
#ifdef SIOCGIFDSTADDR_IN6
	    {
		struct in6_ifreq ifrcopy6;
		
		memcpy(&ifrcopy6, &ip_ifrcopy6, sizeof(ifrcopy6));
		if (ioctl(s, SIOCGIFDSTADDR_IN6, &ifrcopy6) < 0) {
		    // Probably the p2p address is not configured
		} else {
		    // The p2p address is configured
		    // Just in case it doesn't return proper family
		    ifrcopy6.ifr_ifru.ifru_dstaddr.sin6_family = family;
		    peer_addr.copy_in(ifrcopy6.ifr_ifru.ifru_dstaddr);
		    has_peer_addr = true;
		}
	    }
#endif // SIOCGIFDSTADDR_IN6
	    break;
#endif // HAVE_IPV6
	    
	    default:
		XLOG_UNREACHABLE();
		break;
	    }
	    debug_msg("Peer address: %s\n", peer_addr.str().c_str());
	}
	
	debug_msg("\n");	// put an empty line between interfaces
	
	// Add the address
	switch (family) {
	case AF_INET:
	{
	    vifp->add_addr(lcl_addr.get_ipv4());
	    IfTreeAddr4* ap = vifp->find_addr(lcl_addr.get_ipv4());
	    XLOG_ASSERT(ap != NULL);
	    ap->set_enabled(vifp->enabled() && (flags & IFF_UP));
	    ap->set_broadcast(vifp->broadcast()
			      && (flags & IFF_BROADCAST)
			      && has_broadcast_addr);
	    ap->set_loopback(vifp->loopback() && (flags & IFF_LOOPBACK));
#ifdef IFF_POINTOPOINT
	    ap->set_point_to_point(vifp->point_to_point()
				   && (flags & IFF_POINTOPOINT)
				   && has_peer_addr);
#else
	    UNUSED(has_peer_addr);
#endif
	    ap->set_multicast(vifp->multicast() && (flags & IFF_MULTICAST));
	    
	    ap->set_prefix_len(subnet_mask.mask_len());
	    if (ap->broadcast())
		ap->set_bcast(broadcast_addr.get_ipv4());
	    if (ap->point_to_point())
		ap->set_endpoint(peer_addr.get_ipv4());
	    break;
	}
#ifdef HAVE_IPV6
	case AF_INET6:
	{
	    vifp->add_addr(lcl_addr.get_ipv6());
	    IfTreeAddr6* ap = vifp->find_addr(lcl_addr.get_ipv6());
	    XLOG_ASSERT(ap != NULL);
	    ap->set_enabled(vifp->enabled() && (flags & IFF_UP));
	    ap->set_loopback(vifp->loopback() && (flags & IFF_LOOPBACK));
	    ap->set_point_to_point(vifp->point_to_point() && (flags & IFF_POINTOPOINT));
	    ap->set_multicast(vifp->multicast() && (flags & IFF_MULTICAST));
	    
	    ap->set_prefix_len(subnet_mask.mask_len());
	    if (ap->point_to_point())
		ap->set_endpoint(peer_addr.get_ipv6());
	    break;
	}
#endif // HAVE_IPV6
	default:
	    XLOG_UNREACHABLE();
	    break;
	}
    }

    comm_close(s);

    return (XORP_OK);
#else // HOST_OS_WINDOWS
    XLOG_FATAL("WinSock2 does not support struct ifreq.");

    UNUSED(it);
    UNUSED(family);
    UNUSED(buffer);
    return (XORP_ERROR);
#endif // HOST_OS_WINDOWS
}

#endif // HAVE_IOCTL_SIOCGIFCONF
