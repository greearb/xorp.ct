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

#ident "$XORP: xorp/fea/ifconfig_parse_rtm.cc,v 1.14 2003/10/02 16:55:58 pavlin Exp $"


#include "fea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "libxorp/ether_compat.h"

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#include <net/if.h>
#ifdef HAVE_NET_IF_VAR_H
#include <net/if_var.h>
#endif
#ifdef HAVE_NET_IF_DL_H
#include <net/if_dl.h>
#endif
#include <net/route.h>
#ifdef HAVE_NET_IF_TYPES_H
#include <net/if_types.h>
#endif
#ifdef HAVE_NETINET6_IN6_VAR_H
#include <netinet6/in6_var.h>
#endif

#include "ifconfig.hh"
#include "ifconfig_get.hh"
#include "kernel_utils.hh"
#include "routing_socket_utils.hh"


//
// Parse information about network interface configuration change from
// the underlying system.
//
// The information to parse is in RTM format
// (e.g., obtained by routing sockets or by sysctl(3) mechanism).
//

#ifndef HAVE_ROUTING_SOCKETS
bool
IfConfigGet::parse_buffer_rtm(IfTree& , const uint8_t* , size_t )
{
    return false;
}

#else // HAVE_ROUTING_SOCKETS

static void rtm_ifinfo_to_fea_cfg(IfConfig& ifc, const struct if_msghdr* ifm,
				  IfTree& it, u_short& if_index_hint);
static void rtm_addr_to_fea_cfg(IfConfig& ifc, const struct if_msghdr* ifm,
				IfTree& it, u_short if_index_hint);
#ifdef RTM_IFANNOUNCE
static void rtm_announce_to_fea_cfg(IfConfig& ifc, const struct if_msghdr* ifm,
				    IfTree& it);
#endif

// Reading route(4) manual page is a good start for understanding this
bool
IfConfigGet::parse_buffer_rtm(IfTree& it, const uint8_t* buf, size_t buf_bytes)
{
    bool recognized = false;
    u_short if_index_hint = 0;
    
    const struct if_msghdr* ifm = reinterpret_cast<const struct if_msghdr*>(buf);
    const uint8_t* last = buf + buf_bytes;
    
    for (const uint8_t* ptr = buf; ptr < last; ptr += ifm->ifm_msglen) {
    	ifm = reinterpret_cast<const struct if_msghdr*>(ptr);
	if (ifm->ifm_version != RTM_VERSION) {
	    XLOG_ERROR("RTM version mismatch: expected %d got %d",
		       RTM_VERSION,
		       ifm->ifm_version);
	    continue;
	}
	
	switch (ifm->ifm_type) {
	case RTM_IFINFO:
	    if_index_hint = 0;
	    rtm_ifinfo_to_fea_cfg(ifc(), ifm, it, if_index_hint);
	    recognized = true;
	    break;
	case RTM_NEWADDR:
	case RTM_DELADDR:
	    rtm_addr_to_fea_cfg(ifc(), ifm, it, if_index_hint);
	    recognized = true;
	    break;
#ifdef RTM_IFANNOUNCE
	case RTM_IFANNOUNCE:
	    if_index_hint = 0;
	    rtm_announce_to_fea_cfg(ifc(), ifm, it);
	    recognized = true;
	    break;
#endif // RTM_IFANNOUNCE
	case RTM_ADD:
	case RTM_MISS:
	case RTM_CHANGE:
	case RTM_GET:
	case RTM_LOSING:
	case RTM_DELETE:
	    if_index_hint = 0;
	    break;
	default:
	    debug_msg("Unhandled type %s(%d) (%d bytes)\n",
		      RtmUtils::rtm_msg_type(ifm->ifm_type).c_str(),
		      ifm->ifm_type, ifm->ifm_msglen);
	    if_index_hint = 0;
	    break;
	}
    }
    
    if (! recognized)
	return false;
    
    return true;
}

static void
rtm_ifinfo_to_fea_cfg(IfConfig& ifc, const struct if_msghdr* ifm, IfTree& it,
		      u_short& if_index_hint)
{
    XLOG_ASSERT(ifm->ifm_type == RTM_IFINFO);
    
    const struct sockaddr *sa, *rti_info[RTAX_MAX];
    u_short if_index = ifm->ifm_index;
    string if_name;
    
    debug_msg("%p index %d RTM_IFINFO\n", ifm, if_index);
    
    // Get the pointers to the corresponding data structures    
    sa = reinterpret_cast<const sockaddr*>(ifm + 1);
    RtmUtils::get_rta_sockaddr(ifm->ifm_addrs, sa, rti_info);
    
    if_index_hint = if_index;
    
    if (rti_info[RTAX_IFP] == NULL) {
	// Probably an interface being disabled or coming up
	// following RTM_IFANNOUNCE
	
	if (if_index == 0) {
	    debug_msg("Ignoring interface with unknown index\n");
	    return;
	}
	
	const char* name = ifc.get_ifname(if_index);
	if (name == NULL) {
	    char name_buf[IF_NAMESIZE];
#ifdef HAVE_IF_INDEXTONAME
	    name = if_indextoname(if_index, name_buf);
#endif
	    if (name != NULL)
		ifc.map_ifindex(if_index, name);
	}
	if (name == NULL) {
	    XLOG_FATAL("Could not find interface corresponding to index %d",
		       if_index);
	}
	if_name = string(name);
	debug_msg("interface: %s\n", if_name.c_str());
	debug_msg("interface index: %d\n", if_index);
	
	IfTreeInterface* fi = ifc.get_if(it, if_name);
	if (fi == NULL) {
	    XLOG_FATAL("Could not find IfTreeInterface named %s",
		       if_name.c_str());
	}

	//
	// Get the flags
	//
	int flags = ifm->ifm_flags;
	fi->set_if_flags(flags);
	fi->set_enabled(flags & IFF_UP);
	debug_msg("enabled: %s\n", fi->enabled() ? "true" : "false");
	
	// XXX: vifname == ifname on this platform
	IfTreeVif* fv = ifc.get_vif(it, if_name, if_name);
	if (fv == NULL) {
	    XLOG_FATAL("Could not find IfTreeVif on %s named %s",
		       if_name.c_str(), if_name.c_str());
	}
	
	//
	// Set the physical interface index for the vif
	//
	fv->set_pif_index(if_index);
	
	//
	// Set the vif flags
	//
	fv->set_enabled(fi->enabled() && (flags & IFF_UP));
	fv->set_broadcast(flags & IFF_BROADCAST);
	fv->set_loopback(flags & IFF_LOOPBACK);
	fv->set_point_to_point(flags & IFF_POINTOPOINT);
	fv->set_multicast(flags & IFF_MULTICAST);
	debug_msg("vif enabled: %s\n", fv->enabled() ? "true" : "false");
	debug_msg("vif broadcast: %s\n", fv->broadcast() ? "true" : "false");
	debug_msg("vif loopback: %s\n", fv->loopback() ? "true" : "false");
	debug_msg("vif point_to_point: %s\n", fv->point_to_point() ? "true"
		  : "false");
	debug_msg("vif multicast: %s\n", fv->multicast() ? "true" : "false");
	return;
    }
    
    sa = rti_info[RTAX_IFP];
    if (sa->sa_family != AF_LINK) {
	// TODO: verify whether this is really an error.
	XLOG_ERROR("Ignoring RTM_INFO with sa_family = %d", sa->sa_family);
	return;
    }
    const sockaddr_dl* sdl = reinterpret_cast<const sockaddr_dl*>(sa);
    
    if (sdl->sdl_nlen > 0) {
	if_name = string(sdl->sdl_data, sdl->sdl_nlen);
    } else {
	if (if_index == 0) {
	    XLOG_FATAL("Interface with no name and index");
	}
	const char* name = NULL;
#ifdef HAVE_IF_INDEXTONAME
	char name_buf[IF_NAMESIZE];
	name = if_indextoname(if_index, name_buf);
#endif
	if (name == NULL) {
	    XLOG_FATAL("Could not find interface corresponding to index %d",
		       if_index);
	}
	if_name = string(name);
    }
    debug_msg("interface: %s\n", if_name.c_str());
    
    //
    // Get the physical interface index (if unknown)
    //
    do {
	if (if_index > 0)
	    break;
#ifdef HAVE_IF_NAMETOINDEX
	if_index = if_nametoindex(if_name.c_str());
#endif
	if (if_index > 0)
	    break;
#ifdef SIOCGIFINDEX
	{
	    int s;
	    struct ifreq ifridx;
	    
	    s = socket(AF_INET, SOCK_DGRAM, 0);
	    if (s < 0) {
		XLOG_FATAL("Could not initialize IPv4 ioctl() socket");
	    }
	    memset(&ifridx, 0, sizeof(ifridx));
	    strncpy(ifridx.ifr_name, if_name.c_str(),
		    sizeof(ifridx.ifr_name) - 1);
	    if (ioctl(s, SIOCGIFINDEX, &ifridx) < 0) {
		XLOG_ERROR("ioctl(SIOCGIFINDEX) for interface %s failed: %s",
			   ifridx.ifr_name, strerror(errno));
	    } else {
#ifdef HAVE_IFR_IFINDEX
		    if_index = ifridx.ifr_ifindex;
#else
		    if_index = ifridx.ifr_index;
#endif
	    }
	    close(s);
	}
#endif // SIOCGIFINDEX
	if (if_index > 0)
	    break;
    } while (false);
    if (if_index == 0) {
	XLOG_FATAL("Could not find index for interface %s", if_name.c_str());
    }
    if_index_hint = if_index;
    debug_msg("interface index: %d\n", if_index);
    
    //
    // Add the interface (if a new one)
    //
    ifc.map_ifindex(if_index, if_name);
    it.add_if(if_name);
    IfTreeInterface& fi = it.get_if(if_name)->second;
    
    //
    // Get the MAC address
    //
    do {
	if (sdl->sdl_type == IFT_ETHER) {
	    if (sdl->sdl_alen == sizeof(struct ether_addr)) {
		struct ether_addr ea;
		memcpy(&ea, sdl->sdl_data + sdl->sdl_nlen,
		       sdl->sdl_alen);
		fi.set_mac(EtherMac(ea));
		break;
	    } else if (sdl->sdl_alen != 0) {
		XLOG_ERROR("Address size %d uncatered for interface %s",
			   sdl->sdl_alen, if_name.c_str());
	    }
	}
	
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
		fi.set_mac(EtherMac(ea));
		close(s);
		break;
	    }
	    close(s);
	}
#endif // SIOCGIFHWADDR
	
	break;
    } while (false);
    debug_msg("MAC address: %s\n", fi.mac().str().c_str());
    
    //
    // Get the MTU
    //
    fi.set_mtu(ifm->ifm_data.ifi_mtu);
    debug_msg("MTU: %d\n", fi.mtu());
    
    //
    // Get the flags
    //
    int flags = ifm->ifm_flags;
    fi.set_if_flags(flags);
    fi.set_enabled(flags & IFF_UP);
    debug_msg("enabled: %s\n", fi.enabled() ? "true" : "false");
    
    // XXX: vifname == ifname on this platform
    fi.add_vif(if_name);
    IfTreeVif& fv = fi.get_vif(if_name)->second;
    
    //
    // Set the physical interface index for the vif
    //
    fv.set_pif_index(if_index);
    
    //
    // Set the vif flags
    //
    fv.set_enabled(fi.enabled() && (flags & IFF_UP));
    fv.set_broadcast(flags & IFF_BROADCAST);
    fv.set_loopback(flags & IFF_LOOPBACK);
    fv.set_point_to_point(flags & IFF_POINTOPOINT);
    fv.set_multicast(flags & IFF_MULTICAST);
    debug_msg("vif enabled: %s\n", fv.enabled() ? "true" : "false");
    debug_msg("vif broadcast: %s\n", fv.broadcast() ? "true" : "false");
    debug_msg("vif loopback: %s\n", fv.loopback() ? "true" : "false");
    debug_msg("vif point_to_point: %s\n", fv.point_to_point() ? "true"
	      : "false");
    debug_msg("vif multicast: %s\n", fv.multicast() ? "true" : "false");
}

static void
rtm_addr_to_fea_cfg(IfConfig& ifc, const struct if_msghdr* ifm, IfTree& it,
		    u_short if_index_hint)
{
    XLOG_ASSERT(ifm->ifm_type == RTM_NEWADDR || ifm->ifm_type == RTM_DELADDR);
    
    const ifa_msghdr* ifa = reinterpret_cast<const ifa_msghdr*>(ifm);
    const struct sockaddr *sa, *rti_info[RTAX_MAX];
    u_short if_index = ifa->ifam_index;
    string if_name;
    
    debug_msg_indent(4);
    if (ifm->ifm_type == RTM_NEWADDR)
	debug_msg("%p index %d RTM_NEWADDR\n", ifm, if_index);
    if (ifm->ifm_type == RTM_DELADDR)
	debug_msg("%p index %d RTM_DELADDR\n", ifm, if_index);
    
    // Get the pointers to the corresponding data structures
    sa = reinterpret_cast<const sockaddr*>(ifa + 1);
    RtmUtils::get_rta_sockaddr(ifa->ifam_addrs, sa, rti_info);

    if (if_index == 0)
	if_index = if_index_hint;	// XXX: in case if_index is unknown
    
    if (if_index == 0) {
	XLOG_FATAL("Could not add or delete address for interface "
		   "with unknown index");
    }
    
    const char* name = ifc.get_ifname(if_index);
    if (name == NULL) {
#ifdef HAVE_IF_INDEXTONAME
	char name_buf[IF_NAMESIZE];
	name = if_indextoname(if_index, name_buf);
#endif
	if (name != NULL)
	    ifc.map_ifindex(if_index, name);
    }
    if (name == NULL) {
	XLOG_FATAL("Could not find interface corresponding to index %d",
		   if_index);
    }
    if_name = string(name);
    
    debug_msg("Address on interface %s with interface index %d\n",
	      if_name.c_str(), if_index);
    
    //
    // Locate the vif to pin data on
    //
    // XXX: vifname == ifname on this platform
    IfTreeVif* fv = ifc.get_vif(it, if_name, if_name);
    if (fv == NULL) {
	XLOG_FATAL("Could not find vif named %s in IfTree.", if_name.c_str());
    }
    
    if (rti_info[RTAX_IFA] == NULL) {
	debug_msg("Ignoring addr info with null RTAX_IFA entry");
	return;
    }
    
    if (rti_info[RTAX_IFA]->sa_family == AF_INET) {
	IPv4 lcl_addr(*rti_info[RTAX_IFA]);
	fv->add_addr(lcl_addr);
	debug_msg("IP address: %s\n", lcl_addr.str().c_str());
	
	IfTreeAddr4& fa = fv->get_addr(lcl_addr)->second;
	fa.set_enabled(fv->enabled());
	fa.set_broadcast(fv->broadcast());
	fa.set_loopback(fv->loopback());
	fa.set_point_to_point(fv->point_to_point());
	fa.set_multicast(fv->multicast());

	// Get the netmask
	if (rti_info[RTAX_NETMASK] != NULL) {
	    int mask_len = RtmUtils::get_sock_mask_len(AF_INET,
						       rti_info[RTAX_NETMASK]);
	    fa.set_prefix_len(mask_len);
	    IPv4 subnet_mask(IPv4::make_prefix(mask_len));
	    debug_msg("IP netmask: %s\n", subnet_mask.str().c_str());
	}
	
	// Get the broadcast or point-to-point address
	bool has_broadcast_addr = false;
	bool has_peer_addr = false;
	if ((rti_info[RTAX_BRD] != NULL)
	    && (rti_info[RTAX_BRD]->sa_family == AF_INET)) {
	    IPv4 o(*rti_info[RTAX_BRD]);
	    if (fa.broadcast()) {
		fa.set_bcast(o);
		has_broadcast_addr = true;
		debug_msg("Broadcast address: %s\n", o.str().c_str());
	    }
	    if (fa.point_to_point()) {
		fa.set_endpoint(o);
		has_peer_addr = true;
		debug_msg("Peer address: %s\n", o.str().c_str());
	    }
	}
	if (! has_broadcast_addr)
	    fa.set_broadcast(false);
	if (! has_peer_addr)
	    fa.set_point_to_point(false);
	
	// Mark as deleted if necessary
	if (ifa->ifam_type == RTM_DELADDR)
	    fa.mark(IfTreeItem::DELETED);
	
	return;
    }
    
#ifdef HAVE_IPV6
    if (rti_info[RTAX_IFA]->sa_family == AF_INET6) {
	IPv6 lcl_addr(*rti_info[RTAX_IFA]);
	lcl_addr = kernel_ipv6_adjust(lcl_addr);
	fv->add_addr(lcl_addr);
	debug_msg("IP address: %s\n", lcl_addr.str().c_str());
	
	IfTreeAddr6& fa = fv->get_addr(lcl_addr)->second;
	fa.set_enabled(fv->enabled());
	fa.set_loopback(fv->loopback());
	fa.set_point_to_point(fv->point_to_point());
	fa.set_multicast(fv->multicast());
	
	// Get the netmask
	if (rti_info[RTAX_NETMASK] != NULL) {
	    int mask_len = RtmUtils::get_sock_mask_len(AF_INET6,
						       rti_info[RTAX_NETMASK]);
	    fa.set_prefix_len(mask_len);
	    IPv6 subnet_mask(IPv6::make_prefix(mask_len));
	    debug_msg("IP netmask: %s\n", subnet_mask.str().c_str());
	}
	
	// Get the point-to-point address
	bool has_peer_addr = false;
        if ((rti_info[RTAX_BRD] != NULL)
	    && (rti_info[RTAX_BRD]->sa_family == AF_INET6)) {
	    if (fa.point_to_point()) {
		IPv6 o(*rti_info[RTAX_BRD]);
		fa.set_endpoint(o);
		has_peer_addr = true;
		debug_msg("Peer address: %s\n", o.str().c_str());
	    }
        }
	if (! has_peer_addr)
	    fa.set_point_to_point(false);
	
#if 0	// TODO: don't get the flags?
	do {
	    //
	    // Get IPv6 specific flags
	    //
	    int s;
	    struct in6_ifreq ifrcopy6;
	    
	    s = socket(AF_INET6, SOCK_DGRAM, 0);
	    if (s < 0) {
		XLOG_FATAL("Could not initialize IPv6 ioctl() socket");
	    }
	    memset(&ifrcopy6, 0, sizeof(ifrcopy6));
	    strncpy(ifrcopy6.ifr_name, if_name.c_str(),
		    sizeof(ifrcopy6.ifr_name) - 1);
	    a.copy_out(ifrcopy6.ifr_addr);
	    if (ioctl(s, SIOCGIFAFLAG_IN6, &ifrcopy6) < 0) {
		XLOG_ERROR("ioctl(SIOCGIFAFLAG_IN6) for interface %s failed: %s",
			   if_name.c_str(), strerror(errno));
	    }
	    close (s);
	    //
	    // TODO: shall we ignore the anycast addresses?
	    // TODO: what about TENTATIVE, DUPLICATED, DETACHED, DEPRECATED?
	    //
	    if (ifrcopy6.ifr_ifru.ifru_flags6 & IN6_IFF_ANYCAST) {
		
	    }
	} while (false);
#endif // 0/1
	
	// Mark as deleted if necessary
	if (ifa->ifam_type == RTM_DELADDR)
	    fa.mark(IfTreeItem::DELETED);
	
	return;
    }
#endif // HAVE_IPV6
}

#ifdef RTM_IFANNOUNCE
static void
rtm_announce_to_fea_cfg(IfConfig& ifc, const struct if_msghdr* ifm, IfTree& it)
{
    XLOG_ASSERT(ifm->ifm_type == RTM_IFANNOUNCE);
    
    const if_announcemsghdr* ifan = reinterpret_cast<const if_announcemsghdr*>(ifm);
    u_short if_index = ifan->ifan_index;
    string if_name = string(ifan->ifan_name);
    
    debug_msg("RTM_IFANNOUNCE %s on interface %s with interface index %d\n",
	      (ifan->ifan_what == IFAN_DEPARTURE) ? "DEPARTURE" : "ARRIVAL",
	      if_name.c_str(), if_index);
    
    switch (ifan->ifan_what) {
    case IFAN_ARRIVAL:
    {
	//
	// Add interface
	//
	debug_msg("Mapping %d -> %s\n", if_index, if_name.c_str());
	ifc.map_ifindex(if_index, if_name);
	
	it.add_if(if_name);
	IfTreeInterface* fi = ifc.get_if(it, if_name);
	// XXX: vifname == ifname on this platform
	if (fi != NULL) {
	    fi->add_vif(if_name);
	} else {
	    debug_msg("Could not add interface/vif %s\n", if_name.c_str());
	}
	break;
    }
	
    case IFAN_DEPARTURE:
    {
	//
	// Delete interface
	//
	debug_msg("Deleting interface and vif named: %s\n", if_name.c_str());
	IfTreeInterface* fi = ifc.get_if(it, if_name);
	if (fi != NULL) {
	    fi->mark(IfTree::DELETED);
	} else {
	    debug_msg("Attempted to delete missing interface: %s\n",
		      if_name.c_str());
	}
	// XXX: vifname == ifname on this platform
	IfTreeVif* fv = ifc.get_vif(it, if_name, if_name);
	if (fv != NULL) {
	    fv->mark(IfTree::DELETED);
	} else {
	    debug_msg("Attempted to delete missing interface: %s\n",
		      if_name.c_str());
	}
	ifc.unmap_ifindex(if_index);
	break;
    }
    
    default:
	debug_msg("Unknown RTM_IFANNOUNCE message type %d\n", ifan->ifan_what);
	break;
    }
}
#endif // RTM_IFANNOUNCE

#endif // HAVE_ROUTING_SOCKETS
