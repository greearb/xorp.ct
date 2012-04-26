// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others
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
#ifdef HAVE_ROUTING_SOCKETS


#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ether_compat.h"

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
#ifdef HAVE_NET_ROUTE_H
#include <net/route.h>
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
#include "fea/data_plane/control_socket/system_utilities.hh"
#include "fea/data_plane/control_socket/routing_socket_utilities.hh"

#include "ifconfig_get_sysctl.hh"
#include "ifconfig_media.hh"


//
// Parse information about network interface configuration change from
// the underlying system.
//
// The information to parse is in RTM format
// (e.g., obtained by routing sockets or by sysctl(3) mechanism).
//
// Reading route(4) manual page is a good start for understanding this
//


static void rtm_ifinfo_to_fea_cfg(const struct if_msghdr* ifm, IfTree& iftree,
				  uint32_t& if_index_hint);
static void rtm_addr_to_fea_cfg(const struct if_msghdr* ifm, IfTree& iftree,
				uint32_t if_index_hint);
#ifdef RTM_IFANNOUNCE
static void rtm_announce_to_fea_cfg(const struct if_msghdr* ifm,
				    IfTree& iftree);
#endif

//
// Get the link status
//
static int
ifm_get_link_status(const struct if_msghdr* ifm, const string& if_name,
		    bool& no_carrier, uint64_t& baudrate, string& error_msg)
{
    if (ifconfig_media_get_link_status(if_name, no_carrier, baudrate,
				       error_msg)
	== XORP_OK) {
	return (XORP_OK);
    }

#if defined(LINK_STATE_UP) && defined(LINK_STATE_DOWN)
    switch (ifm->ifm_data.ifi_link_state) {
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

    UNUSED(if_name);
    UNUSED(error_msg);
    return (XORP_OK);
#else

    UNUSED(ifm);
    return (XORP_ERROR);
#endif // ! LINK_STATE_UP && LINK_STATE_DOWN
}

/**
 * Generate ifconfig(8) like flags string
 * @param flags interface flags value from routing socket message.
 * @return ifconfig(8) like flags string
 */
string
IfConfigGetSysctl::iff_flags(uint32_t flags)
{
    struct {
	uint32_t 	value;
	const char*	name;
    } iff_fl[] = {
#define IFF_FLAG_ENTRY(X) { IFF_##X, #X }
#ifdef IFF_UP
	IFF_FLAG_ENTRY(UP),
#endif
#ifdef IFF_BROADCAST
	IFF_FLAG_ENTRY(BROADCAST),
#endif
#ifdef IFF_DEBUG
	IFF_FLAG_ENTRY(DEBUG),
#endif
#ifdef IFF_LOOPBACK
	IFF_FLAG_ENTRY(LOOPBACK),
#endif
#ifdef IFF_POINTOPOINT
	IFF_FLAG_ENTRY(POINTOPOINT),
#endif
#ifdef IFF_SMART
	IFF_FLAG_ENTRY(SMART),
#endif
#ifdef IFF_RUNNING
	IFF_FLAG_ENTRY(RUNNING),
#endif
#ifdef IFF_NOARP
	IFF_FLAG_ENTRY(NOARP),
#endif
#ifdef IFF_PROMISC
	IFF_FLAG_ENTRY(PROMISC),
#endif
#ifdef IFF_ALLMULTI
	IFF_FLAG_ENTRY(ALLMULTI),
#endif
#ifdef IFF_OACTIVE
	IFF_FLAG_ENTRY(OACTIVE),
#endif
#ifdef IFF_SIMPLEX
	IFF_FLAG_ENTRY(SIMPLEX),
#endif
#ifdef IFF_LINK0
	IFF_FLAG_ENTRY(LINK0),
#endif
#ifdef IFF_LINK1
	IFF_FLAG_ENTRY(LINK1),
#endif
#ifdef IFF_LINK2
	IFF_FLAG_ENTRY(LINK2),
#endif
#ifdef IFF_ALTPHYS
	IFF_FLAG_ENTRY(ALTPHYS),
#endif
#ifdef IFF_MULTICAST
	IFF_FLAG_ENTRY(MULTICAST),
#endif
	{ 0, "" }  // for nitty compilers that don't like trailing ","
    };
    const size_t n_iff_fl = sizeof(iff_fl) / sizeof(iff_fl[0]);

    string ret("<");
    for (size_t i = 0; i < n_iff_fl; i++) {
	if (0 == (flags & iff_fl[i].value))
	    continue;
	flags &= ~iff_fl[i].value;
	ret += iff_fl[i].name;
	if (0 == flags)
	    break;
	ret += ",";
    }
    ret += ">";
    return ret;
}

int
IfConfigGetSysctl::parse_buffer_routing_socket(IfConfig& ifconfig,
					       IfTree& iftree,
					       const vector<uint8_t>& buffer)
{
    bool recognized = false;
    uint32_t if_index_hint = 0;
    const struct if_msghdr* ifm;
    size_t offset;

    UNUSED(ifconfig);

    ifm = (const struct if_msghdr*)(&(buffer[0]));
    for (offset = 0; offset < buffer.size(); offset += ifm->ifm_msglen) {
	ifm = (const struct if_msghdr*)(&(buffer[offset]));
	if (ifm->ifm_version != RTM_VERSION) {
#if defined(RTM_OVERSION) && defined(HOST_OS_OPENBSD)
	    //
	    // XXX: Silently ignore old messages.
	    // The OpenBSD kernel sends each message twice, once as
	    // RTM_VERSION and once as RTM_OVERSION, hence we need to ignore
	    // the RTM_OVERSION duplicates.
	    //
	    if (ifm->ifm_version == RTM_OVERSION)
		continue;
#endif // RTM_OVERSION && HOST_OS_OPENBSD
	    XLOG_ERROR("RTM version mismatch: expected %d got %d",
		       RTM_VERSION,
		       ifm->ifm_version);
	    continue;
	}
	
	switch (ifm->ifm_type) {
	case RTM_IFINFO:
	    if_index_hint = 0;
	    rtm_ifinfo_to_fea_cfg(ifm, iftree, if_index_hint);
	    recognized = true;
	    break;
	case RTM_NEWADDR:
	case RTM_DELADDR:
	    rtm_addr_to_fea_cfg(ifm, iftree, if_index_hint);
	    recognized = true;
	    break;
#ifdef RTM_IFANNOUNCE
	case RTM_IFANNOUNCE:
	    if_index_hint = 0;
	    rtm_announce_to_fea_cfg(ifm, iftree);
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
	return (XORP_ERROR);

    return (XORP_OK);
}

static void
rtm_ifinfo_to_fea_cfg(const struct if_msghdr* ifm, IfTree& iftree,
		      uint32_t& if_index_hint)
{
    XLOG_ASSERT(ifm->ifm_type == RTM_IFINFO);
    
    const struct sockaddr *sa, *rti_info[RTAX_MAX];
    uint32_t if_index = ifm->ifm_index;
    bool is_newlink = false;	// True if really a new link
    string error_msg;
    
    debug_msg("%p index %u RTM_IFINFO\n", ifm, if_index);
    
    // Get the pointers to the corresponding data structures    
    sa = reinterpret_cast<const sockaddr*>(ifm + 1);
    RtmUtils::get_rta_sockaddr(ifm->ifm_addrs, sa, rti_info);
    
    if_index_hint = if_index;
    
    if (rti_info[RTAX_IFP] == NULL) {
	//
	// Probably an interface being disabled or coming up
	// following RTM_IFANNOUNCE.
	//

	if (if_index == 0) {
	    debug_msg("Ignoring interface with unknown index\n");
	    return;
	}
	debug_msg("interface index: %u\n", if_index);
	
	IfTreeInterface* ifp = iftree.find_interface(if_index);
	if (ifp == NULL) {
	    XLOG_WARNING("Could not find interface with index %u", if_index);
	    return;
	}
	if (ifp->is_marked(IfTreeItem::CREATED))
	    is_newlink = true;

	//
	// Set the physical interface index for the interface
	//
	if (is_newlink || (if_index != ifp->pif_index()))
	    ifp->set_pif_index(if_index);

	//
	// Set the flags
	//
	unsigned int flags = ifm->ifm_flags;
	if (is_newlink || (flags != ifp->interface_flags())) {
	    ifp->set_interface_flags(flags);
	    ifp->set_enabled(flags & IFF_UP);
	}
	debug_msg("enabled: %s\n", bool_c_str(ifp->enabled()));
	
	IfTreeVif* vifp = iftree.find_vif(if_index);
	if (vifp == NULL) {
	    XLOG_FATAL("Could not find vif with index %u", if_index);
	}
	
	//
	// Set the physical interface index for the vif
	//
	if (is_newlink || (if_index != vifp->pif_index()))
	    vifp->set_pif_index(if_index);

	//
	// Get the link status and the baudrate
	//
	bool no_carrier = false;
	uint64_t baudrate = 0;

	if (ifm_get_link_status(ifm, ifp->ifname(), no_carrier, baudrate,
				error_msg)
	    != XORP_OK) {
	    XLOG_ERROR("%s", error_msg.c_str());
	} else {
	    if (is_newlink || (no_carrier != ifp->no_carrier()))
		ifp->set_no_carrier(no_carrier);
	    if (is_newlink || (baudrate != ifp->baudrate()))
		ifp->set_baudrate(baudrate);
	}
	debug_msg("no_carrier: %s\n", bool_c_str(ifp->no_carrier()));
	debug_msg("baudrate: %u\n", XORP_UINT_CAST(ifp->baudrate()));
	
	//
	// Set the vif flags
	//
	if (is_newlink || (flags != vifp->vif_flags())) {
	    vifp->set_vif_flags(flags);
	    vifp->set_enabled(ifp->enabled() && (flags & IFF_UP));
	    vifp->set_broadcast(flags & IFF_BROADCAST);
	    vifp->set_loopback(flags & IFF_LOOPBACK);
	    vifp->set_point_to_point(flags & IFF_POINTOPOINT);
	    vifp->set_multicast(flags & IFF_MULTICAST);
	    // Propagate the flags to the existing addresses
	    vifp->propagate_flags_to_addresses();
	}
	debug_msg("vif enabled: %s\n", bool_c_str(vifp->enabled()));
	debug_msg("vif broadcast: %s\n", bool_c_str(vifp->broadcast()));
	debug_msg("vif loopback: %s\n", bool_c_str(vifp->loopback()));
	debug_msg("vif point_to_point: %s\n", bool_c_str(vifp->point_to_point()));
	debug_msg("vif multicast: %s\n", bool_c_str(vifp->multicast()));
	return;
    }

    //
    // Get the interface name
    //
    string if_name;
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
	if (if_index > 0)
	    break;
#endif

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
#ifdef HAVE_STRUCT_IFREQ_IFR_IFINDEX
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
    debug_msg("interface index: %u\n", if_index);
    
    //
    // Add the interface (if a new one)
    //
    IfTreeInterface* ifp = iftree.find_interface(if_name);
    if (ifp == NULL) {
	iftree.add_interface(if_name);
	is_newlink = true;
	ifp = iftree.find_interface(if_name);
	XLOG_ASSERT(ifp != NULL);
    }

    //
    // Set the physical interface index for the interface
    //
    if (is_newlink || (if_index != ifp->pif_index()))
	ifp->set_pif_index(if_index);

    //
    // Set the MAC address
    //
    do {
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
		Mac mac(ea);
		if (is_newlink || (mac != ifp->mac()))
		    ifp->set_mac(mac);
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
    // Set the MTU
    //
    unsigned int mtu = ifm->ifm_data.ifi_mtu;
    if (is_newlink || (mtu != ifp->mtu()))
	ifp->set_mtu(mtu);
    debug_msg("MTU: %d\n", ifp->mtu());
    
    //
    // Set the flags
    //
    unsigned int flags = ifm->ifm_flags;
    if (is_newlink || (flags != ifp->interface_flags())) {
	ifp->set_interface_flags(flags);
	ifp->set_enabled(flags & IFF_UP);
    }
    debug_msg("enabled: %s\n", bool_c_str(ifp->enabled()));

    //
    // Get the link status and the baudrate
    //
    bool no_carrier = false;
    uint64_t baudrate = 0;

    if (ifm_get_link_status(ifm, if_name, no_carrier, baudrate, error_msg)
	!= XORP_OK) {
	XLOG_ERROR("%s", error_msg.c_str());
    } else {
	if (is_newlink || (no_carrier != ifp->no_carrier()))
	    ifp->set_no_carrier(no_carrier);
	if (is_newlink || (baudrate != ifp->baudrate()))
	    ifp->set_baudrate(baudrate);
    }
    debug_msg("no_carrier: %s\n", bool_c_str(ifp->no_carrier()));
    debug_msg("baudrate: %u\n", XORP_UINT_CAST(ifp->baudrate()));
    
    // XXX: vifname == ifname on this platform
    if (is_newlink)
	ifp->add_vif(if_name);
    IfTreeVif* vifp = ifp->find_vif(if_name);
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
	vifp->set_point_to_point(flags & IFF_POINTOPOINT);
	vifp->set_multicast(flags & IFF_MULTICAST);
	// Propagate the flags to the existing addresses
	vifp->propagate_flags_to_addresses();
    }
    debug_msg("vif enabled: %s\n", bool_c_str(vifp->enabled()));
    debug_msg("vif broadcast: %s\n", bool_c_str(vifp->broadcast()));
    debug_msg("vif loopback: %s\n", bool_c_str(vifp->loopback()));
    debug_msg("vif point_to_point: %s\n", bool_c_str(vifp->point_to_point()));
    debug_msg("vif multicast: %s\n", bool_c_str(vifp->multicast()));
}

static void
rtm_addr_to_fea_cfg(const struct if_msghdr* ifm, IfTree& iftree,
		    uint32_t if_index_hint)
{
    XLOG_ASSERT(ifm->ifm_type == RTM_NEWADDR || ifm->ifm_type == RTM_DELADDR);
    
    const ifa_msghdr* ifa = reinterpret_cast<const ifa_msghdr*>(ifm);
    const struct sockaddr *sa, *rti_info[RTAX_MAX];
    uint32_t if_index = ifa->ifam_index;
    
    debug_msg_indent(4);
    if (ifm->ifm_type == RTM_NEWADDR)
	debug_msg("%p index %u RTM_NEWADDR\n", ifm, if_index);
    if (ifm->ifm_type == RTM_DELADDR)
	debug_msg("%p index %u RTM_DELADDR\n", ifm, if_index);
    
    // Get the pointers to the corresponding data structures
    sa = reinterpret_cast<const sockaddr*>(ifa + 1);
    RtmUtils::get_rta_sockaddr(ifa->ifam_addrs, sa, rti_info);

    if (if_index == 0)
	if_index = if_index_hint;	// XXX: in case if_index is unknown
    
    if (if_index == 0) {
	XLOG_FATAL("Could not add or delete address for interface "
		   "with unknown index");
    }
    
    //
    // Locate the vif to pin data on
    //
    IfTreeVif* vifp = iftree.find_vif(if_index);
    if (vifp == NULL) {
	XLOG_WARNING("Could not find vif with index %u in IfTree", if_index);
	// Maybe it's just already deleted, so don't do anything fatal here.
	return;
    }
    debug_msg("Address event on interface %s vif %s with interface index %u\n",
	      vifp->ifname().c_str(), vifp->vifname().c_str(),
	      vifp->pif_index());

    if (rti_info[RTAX_IFA] == NULL) {
	debug_msg("Ignoring addr info with null RTAX_IFA entry");
	return;
    }
    
    if (rti_info[RTAX_IFA]->sa_family == AF_INET) {
	IPv4 lcl_addr(*rti_info[RTAX_IFA]);
	vifp->add_addr(lcl_addr);
	debug_msg("IP address: %s\n", lcl_addr.str().c_str());
	
	IfTreeAddr4* ap = vifp->find_addr(lcl_addr);
	XLOG_ASSERT(ap != NULL);
	ap->set_enabled(vifp->enabled());
	ap->set_broadcast(vifp->broadcast());
	ap->set_loopback(vifp->loopback());
	ap->set_point_to_point(vifp->point_to_point());
	ap->set_multicast(vifp->multicast());

	// Get the netmask
	if (rti_info[RTAX_NETMASK] != NULL) {
	    int mask_len = RtmUtils::get_sock_mask_len(AF_INET,
						       rti_info[RTAX_NETMASK]);
	    ap->set_prefix_len(mask_len);
	    IPv4 subnet_mask(IPv4::make_prefix(mask_len));
	    UNUSED(subnet_mask);
	    debug_msg("IP netmask: %s\n", subnet_mask.str().c_str());
	}
	
	// Get the broadcast or point-to-point address
	bool has_broadcast_addr = false;
	bool has_peer_addr = false;
	if ((rti_info[RTAX_BRD] != NULL)
	    && (rti_info[RTAX_BRD]->sa_family == AF_INET)) {
	    IPv4 o(*rti_info[RTAX_BRD]);
	    if (ap->broadcast()) {
		ap->set_bcast(o);
		has_broadcast_addr = true;
		debug_msg("Broadcast address: %s\n", o.str().c_str());
	    }
	    if (ap->point_to_point()) {
		ap->set_endpoint(o);
		has_peer_addr = true;
		debug_msg("Peer address: %s\n", o.str().c_str());
	    }
	}
	if (! has_broadcast_addr)
	    ap->set_broadcast(false);
	if (! has_peer_addr)
	    ap->set_point_to_point(false);
	
	// Mark as deleted if necessary
	if (ifa->ifam_type == RTM_DELADDR)
	    ap->mark(IfTreeItem::DELETED);
	
	return;
    }
    
#ifdef HAVE_IPV6
    if (rti_info[RTAX_IFA]->sa_family == AF_INET6) {
	IPv6 lcl_addr(*rti_info[RTAX_IFA]);
	lcl_addr = system_adjust_ipv6_recv(lcl_addr);
	vifp->add_addr(lcl_addr);
	debug_msg("IP address: %s\n", lcl_addr.str().c_str());
	
	IfTreeAddr6* ap = vifp->find_addr(lcl_addr);
	ap->set_enabled(vifp->enabled());
	ap->set_loopback(vifp->loopback());
	ap->set_point_to_point(vifp->point_to_point());
	ap->set_multicast(vifp->multicast());
	
	// Get the netmask
	if (rti_info[RTAX_NETMASK] != NULL) {
	    int mask_len = RtmUtils::get_sock_mask_len(AF_INET6,
						       rti_info[RTAX_NETMASK]);
	    ap->set_prefix_len(mask_len);
	    IPv6 subnet_mask(IPv6::make_prefix(mask_len));
	    UNUSED(subnet_mask);
	    debug_msg("IP netmask: %s\n", subnet_mask.str().c_str());
	}
	
	// Get the point-to-point address
	bool has_peer_addr = false;
        if ((rti_info[RTAX_BRD] != NULL)
	    && (rti_info[RTAX_BRD]->sa_family == AF_INET6)) {
	    if (ap->point_to_point()) {
		IPv6 o(*rti_info[RTAX_BRD]);
		ap->set_endpoint(o);
		has_peer_addr = true;
		debug_msg("Peer address: %s\n", o.str().c_str());
	    }
        }
	if (! has_peer_addr)
	    ap->set_point_to_point(false);
	
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
	    strncpy(ifrcopy6.ifr_name, vifp->vifname().c_str(),
		    sizeof(ifrcopy6.ifr_name) - 1);
	    a.copy_out(ifrcopy6.ifr_addr);
	    if (ioctl(s, SIOCGIFAFLAG_IN6, &ifrcopy6) < 0) {
		XLOG_ERROR("ioctl(SIOCGIFAFLAG_IN6) for interface %s failed: %s",
			   vifp->vif_name().c_str(), strerror(errno));
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
	    ap->mark(IfTreeItem::DELETED);
	
	return;
    }
#endif // HAVE_IPV6
}

#ifdef RTM_IFANNOUNCE
static void
rtm_announce_to_fea_cfg(const struct if_msghdr* ifm, IfTree& iftree)
{
    XLOG_ASSERT(ifm->ifm_type == RTM_IFANNOUNCE);
    
    const if_announcemsghdr* ifan = reinterpret_cast<const if_announcemsghdr*>(ifm);
    uint32_t if_index = ifan->ifan_index;
    string if_name = string(ifan->ifan_name);
    
    debug_msg("RTM_IFANNOUNCE %s on interface %s with interface index %u\n",
	      (ifan->ifan_what == IFAN_DEPARTURE) ? "DEPARTURE" : "ARRIVAL",
	      if_name.c_str(), if_index);
    
    switch (ifan->ifan_what) {
    case IFAN_ARRIVAL:
    {
	//
	// Add interface
	//
	debug_msg("Adding interface/vif %s\n", if_name.c_str());

	iftree.add_interface(if_name);
	IfTreeInterface* ifp = iftree.find_interface(if_name);
	XLOG_ASSERT(ifp != NULL);
	ifp->set_pif_index(if_index);

	// XXX: vifname == ifname on this platform
	ifp->add_vif(if_name);
	IfTreeVif* vifp = ifp->find_vif(if_name);
	XLOG_ASSERT(vifp != NULL);
	vifp->set_pif_index(if_index);

	break;
    }
	
    case IFAN_DEPARTURE:
    {
	//
	// Delete interface
	//
	debug_msg("Deleting interface/vif %s\n", if_name.c_str());

	// Delete the interface
	IfTreeInterface* ifp = NULL;
	if (if_index != 0) {
	    ifp = iftree.find_interface(if_index);
	} else {
	    ifp = iftree.find_interface(if_name);
	}
	if (ifp != NULL)
	    ifp->mark(IfTree::DELETED);

	// Delete the vif
	IfTreeVif* vifp = NULL;
	if (if_index != 0) {
	    vifp = iftree.find_vif(if_index);
	} else {
	    // XXX: vifname == ifname on this platform
	    vifp = iftree.find_vif(if_name, if_name);
	}
	if (vifp != NULL)
	    vifp->mark(IfTree::DELETED);

	break;
    }
    
    default:
	debug_msg("Unknown RTM_IFANNOUNCE message type %d\n", ifan->ifan_what);
	break;
    }
}
#endif // RTM_IFANNOUNCE

#endif // HAVE_ROUTING_SOCKETS
