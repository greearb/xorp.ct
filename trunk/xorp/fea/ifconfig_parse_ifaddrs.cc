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

#ident "$XORP: xorp/fea/ifconfig_parse_ifaddrs.cc,v 1.2 2003/05/14 01:13:42 pavlin Exp $"


#include "fea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "libxorp/ether_compat.h"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"

#include <net/if.h>
#ifdef HAVE_IFADDRS_H
#include <ifaddrs.h>
#endif
#ifdef HAVE_NET_IF_DL_H
#include <net/if_dl.h>
#endif
#ifdef HAVE_NET_IF_TYPES_H
#include <net/if_types.h>
#endif

#include "ifconfig.hh"
#include "ifconfig_get.hh"
#include "kernel_utils.hh"


//
// Parse information about network interface configuration change from
// the underlying system.
//
// The information to parse is in "struct ifaddrs" format
// (e.g., obtained by getifaddrs(3) mechanism).
//

#ifdef HAVE_GETIFADDRS

bool
IfConfigGet::parse_buffer_ifaddrs(IfTree& it, const ifaddrs **ifap)
{
    u_short if_index = 0;
    string if_name;
    const ifaddrs *ifa;
    
    UNUSED(if_index);
    UNUSED(it);
    
    for (ifa = *ifap; ifa != NULL; ifa = ifa->ifa_next) {
	if (ifa->ifa_name == NULL) {
	    XLOG_ERROR("Ignoring interface with unknown name");
	    continue;
	}
	
	// Get the interface name
	char tmp_if_name[IFNAMSIZ+1];
	strncpy(tmp_if_name, ifa->ifa_name, sizeof(tmp_if_name));
#ifdef HOST_OS_SOLARIS
	char *cptr;
	if ( (cptr = strchr(tmp_if_name, ':')) != NULL) {
	    // Replace colon with null. Needed because in Solaris the
	    // interface name changes for aliases
	    *cptr = '\0';
	}
#endif // HOST_OS_SOLARIS
	if_name = string(tmp_if_name);
	
	//
	// Try to get the physical interface index
	//
	do {
	    if_index = 0;
	    if ((ifa->ifa_addr != NULL)
		&& (ifa->ifa_addr->sa_family == AF_LINK)) {
		// Link-level address
		struct sockaddr_dl *sdl = (struct sockaddr_dl *)ifa->ifa_addr;
		if_index = sdl->sdl_index;
	    }
	    if (if_index > 0)
		break;
#ifdef HAVE_IF_NAMETOINDEX
	    // TODO: check whether for Solaris we have to use the
	    // original interface name instead (i.e., the one that has
	    // unique vif name per alias (see the ':' substitution
	    // above).
	    if_index = if_nametoindex(if_name.c_str());
	    if (if_index > 0)
		break;
#endif // HAVE_IF_NAMETOINDEX
#ifdef SIOCGIFINDEX
	    {
		int s;
		struct ifreq ifridx;
		memset(&ifridx, 0, sizeof(ifridx));
		strncpy(ifridx.ifr_name, if_name.c_str(),
			sizeof(ifridx.ifr_name));
	    
		s = socket(AF_INET, SOCK_DGRAM, 0);
		if (s < 0) {
		    XLOG_FATAL("Could not initialize IPv4 ioctl() socket");
		}
		if (ioctl(s, SIOCGIFINDEX, &ifridx) < 0) {
		    XLOG_ERROR("ioctl(SIOCGIFINDEX) for interface %s failed: %s",
			       ifridx.ifr_name, strerror(errno));
		} else {
		    if_index = ifridx.ifr_ifindex;
		}
		close(s);
	    }
	    if (if_index > 0)
		break;
#endif // SIOCGIFINDEX
	} while (false);
	if (if_index == 0) {
	    // TODO: what to do? Shall I assign my own pseudo-indexes?
	    XLOG_FATAL("Could not find index for interface %s",
		       if_name.c_str());
	}
	
	//
	// Add the interface (if a new one)
	//
	ifc().map_ifindex(if_index, if_name);
	it.add_if(if_name);
	IfTreeInterface& fi = it.get_if(if_name)->second;

	//
	// Get the MAC address
	//
	if ((ifa->ifa_addr != NULL)
	    && (ifa->ifa_addr->sa_family == AF_LINK)) {
	    // Link-level address
	    const sockaddr_dl* sdl = reinterpret_cast<const sockaddr_dl*>(ifa->ifa_addr);
	    if (sdl->sdl_type == IFT_ETHER) {
		if (sdl->sdl_alen == sizeof(ether_addr)) {
		    ether_addr ea;
		    memcpy(&ea, sdl->sdl_data + sdl->sdl_nlen, sdl->sdl_alen);
		    fi.set_mac(EtherMac(ea));
		} else if (sdl->sdl_alen != 0) {
		    XLOG_ERROR("Address size %d uncatered for interface %s",
			       sdl->sdl_alen, if_name.c_str());
		}
	    }
	}
	
	//
	// Get the MTU
	//
	if ((ifa->ifa_addr != NULL)
	    && (ifa->ifa_addr->sa_family == AF_LINK)) {
	    int mtu = 0;
	    // Link-level address
	    if (ifa->ifa_data != NULL) {
		struct if_data *if_data = (struct if_data *)ifa->ifa_data;
		mtu = if_data->ifi_mtu;
		if (mtu == 0) {
		    XLOG_ERROR("Coudn't get the MTU for interface %s",
			       if_name.c_str());
		}
		fi.set_mtu(mtu);		
	    }
	}
	
	//
	// Get the flags
	//
	int flags = ifa->ifa_flags;
	fi.set_enabled(flags & IFF_UP);
	
	debug_msg("%s flags %s\n",
		  if_name.c_str(), IfConfigGet::iff_flags(flags).c_str());
	// XXX: vifname == ifname on this platform
	fi.add_vif(if_name);
	IfTreeVif& fv = fi.get_vif(if_name)->second;
	fv.set_enabled(fi.enabled() && (flags & IFF_UP));
	fv.set_broadcast(flags & IFF_BROADCAST);
	fv.set_loopback(flags & IFF_LOOPBACK);
	fv.set_point_to_point(flags & IFF_POINTOPOINT);
	fv.set_multicast(flags & IFF_MULTICAST);
	
	//
	// Get the IP address, netmask, broadcast address, P2P destination
	//
	if (ifa->ifa_addr == NULL)
	    continue;
	
	switch (ifa->ifa_addr->sa_family) {
	case AF_INET:
	{
	    // The default values
	    IPv4 lcl_addr;
	    IPv4 subnet_mask;
	    IPv4 broadcast_addr;
	    IPv4 peer_addr;
	    
	    // Get the IP address
	    if (ifa->ifa_addr != NULL)
		lcl_addr.copy_in(*ifa->ifa_addr);
	    
	    // Get the netmask
	    if (ifa->ifa_netmask != NULL) {
		const sockaddr_in *sin = (sockaddr_in *)ifa->ifa_netmask;
		subnet_mask.copy_in(sin->sin_addr);
	    }
	    
	    // Get the broadcast address
	    if (fv.broadcast() && (ifa->ifa_broadaddr != NULL)) {
		const sockaddr_in *sin = (sockaddr_in *)ifa->ifa_broadaddr;
		broadcast_addr.copy_in(sin->sin_addr);
	    }
	    
	    // Get the p2p address
	    if (fv.point_to_point() && (ifa->ifa_dstaddr != NULL)) {
		const sockaddr_in *sin = (sockaddr_in *)ifa->ifa_dstaddr;
		peer_addr.copy_in(sin->sin_addr);
	    }
	    
	    // Add the address
	    fv.add_addr(lcl_addr);
	    IfTreeAddr4& fa = fv.get_addr(lcl_addr)->second;
	    fa.set_enabled(fv.enabled() && (flags & IFF_UP));
	    fa.set_broadcast(fv.broadcast() && (flags & IFF_BROADCAST));
	    fa.set_loopback(fv.loopback() && (flags & IFF_LOOPBACK));
	    fa.set_point_to_point(fv.point_to_point() && (flags & IFF_POINTOPOINT));
	    fa.set_multicast(fv.multicast() && (flags & IFF_MULTICAST));
	    
	    fa.set_prefix(subnet_mask.prefix_length());
	    if (fa.broadcast())
		fa.set_bcast(broadcast_addr);
	    if (fa.point_to_point())
		fa.set_endpoint(peer_addr);
	    
	    break;
	}
#ifdef HAVE_IPV6
	case AF_INET6:
	{
	    // The default values
	    IPv6 lcl_addr;
	    IPv6 subnet_mask;
	    IPv6 peer_addr;
	    
	    // Get the IP address
	    if (ifa->ifa_addr != NULL)
		lcl_addr.copy_in(*ifa->ifa_addr);
	    lcl_addr = kernel_ipv6_adjust(lcl_addr);
	    
	    // Get the netmask
	    if (ifa->ifa_netmask != NULL) {
		const sockaddr_in6 *sin6 = (sockaddr_in6 *)ifa->ifa_netmask;
		subnet_mask.copy_in(sin6->sin6_addr);
	    }
	    
	    // Get the p2p address
	    if (fv.point_to_point() && (ifa->ifa_dstaddr != NULL)) {
		const sockaddr_in6 *sin6 = (sockaddr_in6 *)ifa->ifa_dstaddr;
		peer_addr.copy_in(sin6->sin6_addr);
	    }
	    
	    // Add the address
	    fv.add_addr(lcl_addr);
	    IfTreeAddr6& fa = fv.get_addr(lcl_addr)->second;
	    fa.set_enabled(fv.enabled() && (flags & IFF_UP));
	    fa.set_loopback(fv.loopback() && (flags & IFF_LOOPBACK));
	    fa.set_point_to_point(fv.point_to_point() && (flags & IFF_POINTOPOINT));
	    fa.set_multicast(fv.multicast() && (flags & IFF_MULTICAST));
	    
	    fa.set_prefix(subnet_mask.prefix_length());
	    if (fa.point_to_point())
		fa.set_endpoint(peer_addr);
	    
	    //
	    // TODO: do we need to check the IPv6-specific flags, and ignore
	    // anycast addresses?
	    // XXX: what about TENTATIVE, DUPLICATED, DETACHED, DEPRECATED?
	    break;
	}
#endif // HAVE_IPV6
	default:
	    break;
	}
    }
    
    return true;
}

#endif // HAVE_GETIFADDRS
