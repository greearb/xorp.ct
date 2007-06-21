// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

#ident "$XORP: xorp/fea/data_plane/ifconfig/ifconfig_parse_getifaddrs.cc,v 1.9 2007/06/04 23:17:35 pavlin Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ether_compat.h"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
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
#ifdef HAVE_IFADDRS_H
#include <ifaddrs.h>
#endif

#include "fea/ifconfig.hh"
#include "fea/ifconfig_get.hh"
#include "fea/data_plane/control_socket/system_utilities.hh"

#include "ifconfig_media.hh"


//
// Parse information about network interface configuration change from
// the underlying system.
//
// The information to parse is in "struct ifaddrs" format
// (e.g., obtained by getifaddrs(3) mechanism).
//

#ifndef HAVE_GETIFADDRS

bool
IfConfigGetGetifaddrs::parse_buffer_getifaddrs(IfConfig& ,
					       IfTree& ,
					       const struct ifaddrs* )
{
    return (false);
}

#else // HAVE_GETIFADDRS

bool
IfConfigGetGetifaddrs::parse_buffer_getifaddrs(IfConfig& ifconfig, IfTree& it,
					       const struct ifaddrs* ifap)
{
    u_short if_index = 0;
    string if_name, alias_if_name;
    const struct ifaddrs* ifa;
    
    UNUSED(if_index);
    UNUSED(it);
    
    for (ifa = ifap; ifa != NULL; ifa = ifa->ifa_next) {
	bool is_newlink = false;	// True if really a new link

	if (ifa->ifa_name == NULL) {
	    XLOG_ERROR("Ignoring interface with unknown name");
	    continue;
	}
	
	//
	// Get the interface name
	//
	char tmp_if_name[IFNAMSIZ+1];
	strncpy(tmp_if_name, ifa->ifa_name, sizeof(tmp_if_name) - 1);
	tmp_if_name[sizeof(tmp_if_name) - 1] = '\0';
	char* cptr;
	if ( (cptr = strchr(tmp_if_name, ':')) != NULL) {
	    // Replace colon with null. Needed because in Solaris and Linux
	    // the interface name changes for aliases.
	    *cptr = '\0';
	}
	if_name = string(ifa->ifa_name);
	alias_if_name = string(tmp_if_name);
	debug_msg("interface: %s\n", if_name.c_str());
	debug_msg("alias interface: %s\n", alias_if_name.c_str());
	
	//
	// Get the physical interface index
	//
	do {
	    if_index = 0;
#ifdef AF_LINK
	    if ((ifa->ifa_addr != NULL)
		&& (ifa->ifa_addr->sa_family == AF_LINK)) {
		// Link-level address
		const struct sockaddr_dl* sdl = reinterpret_cast<const struct sockaddr_dl*>(ifa->ifa_addr);
		if_index = sdl->sdl_index;
	    }
	    if (if_index > 0)
		break;
#endif // AF_LINK
	    
#ifdef HAVE_IF_NAMETOINDEX
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
			sizeof(ifridx.ifr_name) - 1);
	    
		s = socket(AF_INET, SOCK_DGRAM, 0);
		if (s < 0) {
		    XLOG_FATAL("Could not initialize IPv4 ioctl() socket");
		}
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
		close(s);
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
	debug_msg("interface index: %d\n", if_index);
	
	//
	// Add the interface (if a new one)
	//
	ifconfig.map_ifindex(if_index, alias_if_name);
	IfTreeInterface* ifp = it.find_interface(alias_if_name);
	if (ifp == NULL) {
	    it.add_interface(alias_if_name);
	    is_newlink = true;
	    ifp = it.find_interface(alias_if_name);
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
	    if ((ifa->ifa_addr != NULL)
		&& (ifa->ifa_addr->sa_family == AF_LINK)) {
		// Link-level address
		const struct sockaddr_dl* sdl = reinterpret_cast<const struct sockaddr_dl*>(ifa->ifa_addr);
		if (sdl->sdl_type == IFT_ETHER) {
		    if (sdl->sdl_alen == sizeof(struct ether_addr)) {
			struct ether_addr ea;
			memcpy(&ea, sdl->sdl_data + sdl->sdl_nlen,
			       sdl->sdl_alen);
			EtherMac ether_mac(ea);
			if (is_newlink || (ether_mac != EtherMac(ifp->mac())))
			    ifp->set_mac(ether_mac);
			break;
		    } else if (sdl->sdl_alen != 0) {
			XLOG_ERROR("Address size %d uncatered for interface %s",
				   sdl->sdl_alen, if_name.c_str());
		    }
		}
	    }
#endif // AF_LINK

#ifdef SIOCGIFHWADDR
	    {
		int s;
		struct ifreq ifridx;
		memset(&ifridx, 0, sizeof(ifridx));
		strncpy(ifridx.ifr_name, if_name.c_str(),
			sizeof(ifridx.ifr_name) - 1);
	    
		s = socket(AF_INET, SOCK_DGRAM, 0);
		if (s < 0) {
		    XLOG_FATAL("Could not initialize IPv4 ioctl() socket");
		}
		if (ioctl(s, SIOCGIFHWADDR, &ifridx) < 0) {
		    XLOG_ERROR("ioctl(SIOCGIFHWADDR) for interface %s failed: %s",
			      if_name.c_str(), strerror(errno));
		} else {
		    struct ether_addr ea;
		    memcpy(&ea, ifridx.ifr_hwaddr.sa_data, sizeof(ea));
		    EtherMac ether_mac(ea);
		    if (is_newlink || (ether_mac != EtherMac(ifp->mac())))
			ifp->set_mac(ether_mac);
		    close(s);
		    break;
		}
		close(s);
	    }
#endif // SIOCGIFHWADDR
	    
	    break;
	} while (false);
	debug_msg("MAC address: %s\n", ifp->mac().str().c_str());
	
	//
	// Get the MTU
	//
	do {
#ifdef AF_LINK
	    if ((ifa->ifa_addr != NULL)
		&& (ifa->ifa_addr->sa_family == AF_LINK)) {
		// Link-level address
		if (ifa->ifa_data != NULL) {
		    const struct if_data* if_data = reinterpret_cast<const struct if_data*>(ifa->ifa_data);
		    unsigned int mtu = if_data->ifi_mtu;
		    if (mtu == 0) {
			XLOG_ERROR("Couldn't get the MTU for interface %s",
				   if_name.c_str());
		    }
		    if (is_newlink || (mtu != ifp->mtu()))
			ifp->set_mtu(mtu);
		    break;
		}
	    }
#endif // AF_LINK
	    
#ifdef SIOCGIFMTU
	    {
		int s;
		struct ifreq ifridx;
		memset(&ifridx, 0, sizeof(ifridx));
		strncpy(ifridx.ifr_name, if_name.c_str(),
			sizeof(ifridx.ifr_name) - 1);

		s = socket(AF_INET, SOCK_DGRAM, 0);
		if (s < 0) {
		    XLOG_FATAL("Could not initialize IPv4 ioctl() socket");
		}
		if (ioctl(s, SIOCGIFMTU, &ifridx) < 0) {
		    XLOG_ERROR("ioctl(SIOCGIFMTU) for interface %s failed: %s",
			      if_name.c_str(), strerror(errno));
		} else {
		    unsigned int mtu = ifridx.ifr_mtu;
		    if (is_newlink || (mtu != ifp->mtu()))
			ifp->set_mtu(mtu);
		    close(s);
		    break;
		}
		close(s);
	    }
#endif // SIOCGIFMTU
	    
	    break;
	} while (false);
	debug_msg("MTU: %d\n", ifp->mtu());

	//
	// Get the link status
	//
	do {
	    bool no_carrier = false;
	    string error_msg;

#if defined(AF_LINK) && defined(LINK_STATE_UP) && defined(LINK_STATE_DOWN)
	    if ((ifa->ifa_addr != NULL)
		&& (ifa->ifa_addr->sa_family == AF_LINK)) {
		// Link-level address
		if (ifa->ifa_data != NULL) {
		    const struct if_data* if_data = reinterpret_cast<const struct if_data*>(ifa->ifa_data);
		    switch (if_data->ifi_link_state) {
		    case LINK_STATE_UNKNOWN:
			// XXX: link state unknown
			break;
		    case LINK_STATE_DOWN:
			no_carrier = true;
			break;
		    case LINK_STATE_UP:
			no_carrier = false;
			break;
		    default:
			break;
		    }
		    if (is_newlink || no_carrier != ifp->no_carrier())
			ifp->set_no_carrier(no_carrier);
		    break;
		}
	    }
#endif // AF_LINK && LINK_STATE_UP && LINK_STATE_DOWN

	    if (ifconfig_media_get_link_status(if_name, no_carrier, error_msg)
		!= XORP_OK) {
		XLOG_ERROR("%s", error_msg.c_str());
		break;
	    }
	    if (is_newlink || (no_carrier != ifp->no_carrier()))
		ifp->set_no_carrier(no_carrier);
	    break;
	} while (false);
	debug_msg("no_carrier: %s\n", bool_c_str(ifp->no_carrier()));
	
	//
	// Get the flags
	//
	unsigned int flags = ifa->ifa_flags;
	if (is_newlink || (flags != ifp->interface_flags())) {
	    ifp->set_interface_flags(flags);
	    ifp->set_enabled(flags & IFF_UP);
	}
	debug_msg("enabled: %s\n", bool_c_str(ifp->enabled()));
	
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
	if (is_newlink || (flags != ifp->interface_flags())) {
	    vifp->set_enabled(ifp->enabled() && (flags & IFF_UP));
	    vifp->set_broadcast(flags & IFF_BROADCAST);
	    vifp->set_loopback(flags & IFF_LOOPBACK);
	    vifp->set_point_to_point(flags & IFF_POINTOPOINT);
	    vifp->set_multicast(flags & IFF_MULTICAST);
	}
	debug_msg("vif enabled: %s\n", bool_c_str(vifp->enabled()));
	debug_msg("vif broadcast: %s\n", bool_c_str(vifp->broadcast()));
	debug_msg("vif loopback: %s\n", bool_c_str(vifp->loopback()));
	debug_msg("vif point_to_point: %s\n",
		  bool_c_str(vifp->point_to_point()));
	debug_msg("vif multicast: %s\n", bool_c_str(vifp->multicast()));
	
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
	    bool has_broadcast_addr = false;
	    bool has_peer_addr = false;
	    
	    // Get the IP address
	    if (ifa->ifa_addr != NULL) {
		lcl_addr.copy_in(*ifa->ifa_addr);
	    }
	    debug_msg("IP address: %s\n", lcl_addr.str().c_str());
	    
	    // Get the netmask
	    if (ifa->ifa_netmask != NULL) {
		const struct sockaddr_in* sin = sockaddr2sockaddr_in(ifa->ifa_netmask);
		subnet_mask.copy_in(sin->sin_addr);
	    }
	    debug_msg("IP netmask: %s\n", subnet_mask.str().c_str());
	    
	    // Get the broadcast address
	    if (vifp->broadcast() && (ifa->ifa_broadaddr != NULL)) {
		const struct sockaddr_in* sin = sockaddr2sockaddr_in(ifa->ifa_broadaddr);
		broadcast_addr.copy_in(sin->sin_addr);
		has_broadcast_addr = true;
		debug_msg("Broadcast address: %s\n",
			  broadcast_addr.str().c_str());
	    }
	    
	    // Get the p2p address
	    if (vifp->point_to_point() && (ifa->ifa_dstaddr != NULL)) {
		const struct sockaddr_in* sin = sockaddr2sockaddr_in(ifa->ifa_dstaddr);
		peer_addr.copy_in(sin->sin_addr);
		has_peer_addr = true;
		debug_msg("Peer address: %s\n", peer_addr.str().c_str());
	    }
	    
	    debug_msg("\n");	// put an empty line between interfaces
	    
	    // Add the address
	    vifp->add_addr(lcl_addr);
	    IfTreeAddr4* ap = vifp->find_addr(lcl_addr);
	    XLOG_ASSERT(ap != NULL);
	    ap->set_enabled(vifp->enabled() && (flags & IFF_UP));
	    ap->set_broadcast(vifp->broadcast()
			      && (flags & IFF_BROADCAST)
			      && has_broadcast_addr);
	    ap->set_loopback(vifp->loopback() && (flags & IFF_LOOPBACK));
	    ap->set_point_to_point(vifp->point_to_point()
				   && (flags & IFF_POINTOPOINT)
				   && has_peer_addr);
	    ap->set_multicast(vifp->multicast() && (flags & IFF_MULTICAST));
	    
	    ap->set_prefix_len(subnet_mask.mask_len());
	    if (ap->broadcast())
		ap->set_bcast(broadcast_addr);
	    if (ap->point_to_point())
		ap->set_endpoint(peer_addr);
	    
	    break;
	}
#ifdef HAVE_IPV6
	case AF_INET6:
	{
	    // The default values
	    IPv6 lcl_addr;
	    IPv6 subnet_mask;
	    IPv6 peer_addr;
	    bool has_peer_addr = false;
	    
	    // Get the IP address
	    if (ifa->ifa_addr != NULL) {
		lcl_addr.copy_in(*ifa->ifa_addr);
	    }
	    lcl_addr = system_adjust_ipv6_recv(lcl_addr);
	    debug_msg("IP address: %s\n", lcl_addr.str().c_str());
	    
	    // Get the netmask
	    if (ifa->ifa_netmask != NULL) {
		const struct sockaddr_in6* sin6 = sockaddr2sockaddr_in6(ifa->ifa_netmask);
		subnet_mask.copy_in(sin6->sin6_addr);
	    }
	    debug_msg("IP netmask: %s\n", subnet_mask.str().c_str());
	    
	    // Get the p2p address
	    // XXX: note that unlike IPv4, here we check whether the
	    // address family of the destination address is valid.
	    if (vifp->point_to_point()
		&& (ifa->ifa_dstaddr != NULL)
		&& (ifa->ifa_dstaddr->sa_family == AF_INET6)) {
		const struct sockaddr_in6* sin6 = sockaddr2sockaddr_in6(ifa->ifa_dstaddr);
		peer_addr.copy_in(sin6->sin6_addr);
		has_peer_addr = true;
		debug_msg("Peer address: %s\n", peer_addr.str().c_str());
	    }
	    
	    debug_msg("\n");	// put an empty line between interfaces
	    
	    // Add the address
	    vifp->add_addr(lcl_addr);
	    IfTreeAddr6* ap = vifp->find_addr(lcl_addr);
	    XLOG_ASSERT(ap != NULL);
	    ap->set_enabled(vifp->enabled() && (flags & IFF_UP));
	    ap->set_loopback(vifp->loopback() && (flags & IFF_LOOPBACK));
	    ap->set_point_to_point(vifp->point_to_point()
				   && (flags & IFF_POINTOPOINT)
				   && has_peer_addr);
	    ap->set_multicast(vifp->multicast() && (flags & IFF_MULTICAST));
	    
	    ap->set_prefix_len(subnet_mask.mask_len());
	    if (ap->point_to_point())
		ap->set_endpoint(peer_addr);
	    
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
