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

#ident "$XORP: xorp/fea/ifconfig_parse_nlm.cc,v 1.9 2004/03/17 07:16:14 pavlin Exp $"


#include "fea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "libxorp/ether_compat.h"

// TODO: XXX: PAVPAVPAV: move this include somewhere else!!
#ifdef HOST_OS_LINUX
#include <linux/types.h>
#endif
#ifdef HAVE_LINUX_RTNETLINK_H
#include <linux/rtnetlink.h>
#endif

#include <net/if.h>
#include <net/if_arp.h>

#include "ifconfig.hh"
#include "ifconfig_get.hh"
#include "kernel_utils.hh"
#include "netlink_socket_utils.hh"


//
// Parse information about network interface configuration change from
// the underlying system.
//
// The information to parse is in NETLINK format
// (e.g., obtained by netlink(7) sockets mechanism).
//
// Reading netlink(3) manual page is a good start for understanding this
//

#ifndef HAVE_NETLINK_SOCKETS
bool
IfConfigGet::parse_buffer_nlm(IfTree& , const uint8_t* , size_t )
{
    return false;
}

#else // HAVE_NETLINK_SOCKETS

static void nlm_newlink_to_fea_cfg(IfConfig& ifc, IfTree& it,
				   const struct ifinfomsg* ifinfomsg,
				   int rta_len);
static void nlm_dellink_to_fea_cfg(IfConfig& ifc, IfTree& it,
				   const struct ifinfomsg* ifinfomsg,
				   int rta_len);
static void nlm_newdeladdr_to_fea_cfg(IfConfig& ifc, IfTree& it,
				      const struct ifaddrmsg* ifaddrmsg,
				      int rta_len, bool is_deleted);

bool
IfConfigGet::parse_buffer_nlm(IfTree& it, const uint8_t* buf, size_t buf_bytes)
{
    const struct nlmsghdr* nlh;
    bool recognized = false;
    
    for (nlh = reinterpret_cast<const struct nlmsghdr*>(buf);
	 NLMSG_OK(nlh, buf_bytes);
	 nlh = NLMSG_NEXT(const_cast<struct nlmsghdr*>(nlh), buf_bytes)) {
	caddr_t nlmsg_data = reinterpret_cast<caddr_t>(NLMSG_DATA(const_cast<struct nlmsghdr*>(nlh)));
	
	switch (nlh->nlmsg_type) {
	case NLMSG_ERROR:
	{
	    const struct nlmsgerr* err;
	    
	    err = reinterpret_cast<const struct nlmsgerr*>(nlmsg_data);
	    if (nlh->nlmsg_len < NLMSG_LENGTH(sizeof(*err))) {
		XLOG_ERROR("AF_NETLINK nlmsgerr length error");
		break;
	    }
	    errno = -err->error;
	    XLOG_ERROR("AF_NETLINK NLMSG_ERROR message: %s", strerror(errno));
	}
	break;
	
	case NLMSG_DONE:
	{
	    if (! recognized)
		return false;
	    return true;
	}
	break;
	
	case NLMSG_NOOP:
	    break;
	    
	case RTM_NEWLINK:
	case RTM_DELLINK:
	{
	    const struct ifinfomsg* ifinfomsg;
	    int rta_len = IFLA_PAYLOAD(nlh);
	    
	    if (rta_len < 0) {
		XLOG_ERROR("AF_NETLINK ifinfomsg length error");
		break;
	    }
	    ifinfomsg = reinterpret_cast<const struct ifinfomsg*>(nlmsg_data);
	    if (nlh->nlmsg_type == RTM_NEWLINK)
		nlm_newlink_to_fea_cfg(ifc(), it, ifinfomsg, rta_len);
	    else
		nlm_dellink_to_fea_cfg(ifc(), it, ifinfomsg, rta_len);
	    recognized = true;
	}
	break;
	
	case RTM_NEWADDR:
	case RTM_DELADDR:
	{
	    const struct ifaddrmsg* ifaddrmsg;
	    int rta_len = IFA_PAYLOAD(nlh);
	    
	    if (rta_len < 0) {
		XLOG_ERROR("AF_NETLINK ifaddrmsg length error");
		break;
	    }
	    ifaddrmsg = reinterpret_cast<const struct ifaddrmsg*>(nlmsg_data);
	    if (nlh->nlmsg_type == RTM_NEWADDR)
		nlm_newdeladdr_to_fea_cfg(ifc(), it, ifaddrmsg, rta_len, false);
	    else
		nlm_newdeladdr_to_fea_cfg(ifc(), it, ifaddrmsg, rta_len, true);
	    recognized = true;
	}
	break;
	
	default:
	    debug_msg("Unhandled type %s(%d) (%d bytes)\n",
		      NlmUtils::nlm_msg_type(nlh->nlmsg_type).c_str(),
		      nlh->nlmsg_type, nlh->nlmsg_len);
	    break;
	}
    }
    
    if (! recognized)
	return false;
    
    return true;
}

static void
nlm_newlink_to_fea_cfg(IfConfig& ifc, IfTree& it,
		       const struct ifinfomsg* ifinfomsg, int rta_len)
{
    const struct rtattr *rtattr;
    const struct rtattr *rta_array[IFLA_MAX + 1];
    u_short if_index = 0;
    string if_name;
    
    // The attributes
    memset(rta_array, 0, sizeof(rta_array));
    rtattr = IFLA_RTA(const_cast<struct ifinfomsg*>(ifinfomsg));
    NlmUtils::get_rtattr(rtattr, rta_len, rta_array,
			 sizeof(rta_array) / sizeof(rta_array[0]));
    
    //
    // Get the interface name
    //
    if (rta_array[IFLA_IFNAME] == NULL) {
	XLOG_FATAL("Could not find interface name for interface index %d",
		   ifinfomsg->ifi_index);
    }
    caddr_t rta_data = reinterpret_cast<caddr_t>(RTA_DATA(const_cast<struct rtattr*>(rta_array[IFLA_IFNAME])));
    if_name = string(reinterpret_cast<char*>(rta_data));
    debug_msg("interface: %s\n", if_name.c_str());
    
    //
    // Get the interface index
    //
    if_index = ifinfomsg->ifi_index;
    if (if_index == 0) {
	XLOG_FATAL("Could not find physical interface index "
		   "for interface %s",
		   if_name.c_str());
    }
    debug_msg("interface index: %d\n", if_index);
    
    //
    // Add the interface (if a new one)
    //
    ifc.map_ifindex(if_index, if_name);
    it.add_if(if_name);
    IfTreeInterface& fi = it.get_if(if_name)->second;

    //
    // Set the physical interface index for the interface
    //
    fi.set_pif_index(if_index);

    //
    // Get the MAC address
    //
    if (rta_array[IFLA_ADDRESS] != NULL) {
	if ((ifinfomsg->ifi_type == ARPHRD_ETHER)
	    && (RTA_PAYLOAD(rta_array[IFLA_ADDRESS])
		== sizeof(struct ether_addr))) {
	    struct ether_addr ea;
	    memcpy(&ea, RTA_DATA(const_cast<struct rtattr*>(rta_array[IFLA_ADDRESS])), sizeof(ea));
	    fi.set_mac(EtherMac(ea));
	}
    }
    debug_msg("MAC address: %s\n", fi.mac().str().c_str());
    
    //
    // Get the MTU
    //
    if (rta_array[IFLA_MTU] != NULL) {
	unsigned int mtu;
	
	XLOG_ASSERT(RTA_PAYLOAD(rta_array[IFLA_MTU]) == sizeof(mtu));
	mtu = *reinterpret_cast<unsigned int*>(RTA_DATA(const_cast<struct rtattr*>(rta_array[IFLA_MTU])));
	fi.set_mtu(mtu);
    }
    debug_msg("MTU: %d\n", fi.mtu());
    
    //
    // Get the flags
    //
    unsigned int flags = ifinfomsg->ifi_flags;
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
nlm_dellink_to_fea_cfg(IfConfig& ifc, IfTree& it,
		       const struct ifinfomsg* ifinfomsg, int rta_len)
{
    const struct rtattr *rtattr;
    const struct rtattr *rta_array[IFLA_MAX + 1];
    u_short if_index = 0;
    string if_name;
    
    // The attributes
    memset(rta_array, 0, sizeof(rta_array));
    rtattr = IFLA_RTA(const_cast<struct ifinfomsg*>(ifinfomsg));
    NlmUtils::get_rtattr(rtattr, rta_len, rta_array,
			 sizeof(rta_array) / sizeof(rta_array[0]));
    
    //
    // Get the interface name
    //
    if (rta_array[IFLA_IFNAME] == NULL) {
	XLOG_FATAL("Could not find interface name for interface index %d",
		   ifinfomsg->ifi_index);
    }
    caddr_t rta_data = reinterpret_cast<caddr_t>(RTA_DATA(const_cast<struct rtattr*>(rta_array[IFLA_IFNAME])));
    if_name = string(reinterpret_cast<char*>(rta_data));
    debug_msg("interface: %s\n", if_name.c_str());
    
    //
    // Get the interface index
    //
    if_index = ifinfomsg->ifi_index;
    if (if_index == 0) {
	XLOG_FATAL("Could not find physical interface index "
		   "for interface %s",
		   if_name.c_str());
    }
    debug_msg("interface index: %d\n", if_index);
    
    //
    // Delete the interface
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
}

static void
nlm_newdeladdr_to_fea_cfg(IfConfig& ifc, IfTree& it,
    const struct ifaddrmsg* ifaddrmsg, int rta_len, bool is_deleted)
{
    const struct rtattr *rtattr;
    const struct rtattr *rta_array[IFA_MAX + 1];
    u_short if_index = 0;
    string if_name;
    int family = ifaddrmsg->ifa_family;
    
    //
    // Test the family
    //
    switch (family) {
    case AF_INET:
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	break;
#endif // HAVE_IPV6
    default:
	debug_msg("Ignoring address of family %d\n", family);
	return;
    }
    
    // The attributes
    memset(rta_array, 0, sizeof(rta_array));
    rtattr = IFA_RTA(const_cast<struct ifaddrmsg*>(ifaddrmsg));
    NlmUtils::get_rtattr(rtattr, rta_len, rta_array,
			 sizeof(rta_array) / sizeof(rta_array[0]));

    //
    // Get the interface index
    //
    if_index = ifaddrmsg->ifa_index;
    if (if_index == 0) {
	XLOG_FATAL("Could not add or delete address for interface "
		   "with unknown index");
    }
    
    //
    // Get the interface name
    //
    const char* name = ifc.get_ifname(if_index);
    if (name == NULL) {
	if (is_deleted) {
	    //
	    // XXX: in case of Linux kernel we may receive first
	    // a message to delete the interface and then a message to
	    // delete an address on that interface.
	    // However, the first message would remove all state about
	    // that interface (including its addresses).
	    // Hence, we silently ignore messages for deleting addresses
	    // if the interface is not found.
	    //
	    return;
	}

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
    
    // Get the IP address
    if (rta_array[IFA_LOCAL] != NULL) {
	const uint8_t* data = reinterpret_cast<const uint8_t*>(RTA_DATA(const_cast<struct rtattr*>(rta_array[IFA_LOCAL])));
	if (RTA_PAYLOAD(rta_array[IFA_LOCAL]) != IPvX::addr_size(family)) {
	    XLOG_FATAL("Invalid IFA_LOCAL address size payload: "
		       "received %d expected %d",
		       RTA_PAYLOAD(rta_array[IFA_LOCAL]),
		       IPvX::addr_size(family));
	}
	lcl_addr.copy_in(family, data);
    }
    lcl_addr = kernel_ipvx_adjust(lcl_addr);
    debug_msg("IP address: %s\n", lcl_addr.str().c_str());
    
    // Get the netmask
    subnet_mask = IPvX::make_prefix(family, ifaddrmsg->ifa_prefixlen);
    debug_msg("IP netmask: %s\n", subnet_mask.str().c_str());
    
    // Get the broadcast address
    if (fv->broadcast()) {
	switch (family) {
	case AF_INET:
	    if (rta_array[IFA_BROADCAST] != NULL) {
		const uint8_t* data = reinterpret_cast<const uint8_t*>(RTA_DATA(const_cast<struct rtattr*>(rta_array[IFA_BROADCAST])));
		if (RTA_PAYLOAD(rta_array[IFA_BROADCAST])
		    != IPvX::addr_size(family)) {
		    XLOG_FATAL("Invalid IFA_BROADCAST address size payload: "
			       "received %d expected %d",
			       RTA_PAYLOAD(rta_array[IFA_BROADCAST]),
			       IPvX::addr_size(family));
		}
		broadcast_addr.copy_in(family, data);
		has_broadcast_addr = true;
	    }
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
    if (fv->point_to_point()) {
	if (rta_array[IFA_ADDRESS] != NULL) {
	    const uint8_t* data = reinterpret_cast<const uint8_t*>(RTA_DATA(const_cast<struct rtattr*>(rta_array[IFA_ADDRESS])));
	    if (RTA_PAYLOAD(rta_array[IFA_ADDRESS])
		!= IPvX::addr_size(family)) {
		XLOG_FATAL("Invalid IFA_ADDRESS address size payload: "
			   "received %d expected %d",
			   RTA_PAYLOAD(rta_array[IFA_ADDRESS]),
			   IPvX::addr_size(family));
	    }
	    peer_addr.copy_in(family, data);
	    has_peer_addr = true;
	}
	debug_msg("Peer address: %s\n", peer_addr.str().c_str());
    }
    
    debug_msg("\n");		// put an empty line between interfaces
    
    // Add or delete the address
    switch (family) {
    case AF_INET:
    {
	fv->add_addr(lcl_addr.get_ipv4());
	IfTreeAddr4& fa = fv->get_addr(lcl_addr.get_ipv4())->second;
	fa.set_enabled(fv->enabled());
	fa.set_broadcast(fv->broadcast()
			 && has_broadcast_addr);
	fa.set_loopback(fv->loopback());
	fa.set_point_to_point(fv->point_to_point()
			      && has_peer_addr);
	fa.set_multicast(fv->multicast());
	
	fa.set_prefix_len(subnet_mask.mask_len());
	if (fa.broadcast())
	    fa.set_bcast(broadcast_addr.get_ipv4());
	if (fa.point_to_point())
	    fa.set_endpoint(peer_addr.get_ipv4());
	
	// Mark as deleted if necessary
	if (is_deleted)
	    fa.mark(IfTreeItem::DELETED);
	
	break;
    }
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	fv->add_addr(lcl_addr.get_ipv6());
	IfTreeAddr6& fa = fv->get_addr(lcl_addr.get_ipv6())->second;
	fa.set_enabled(fv->enabled());
	fa.set_loopback(fv->loopback());
	fa.set_point_to_point(fv->point_to_point());
	fa.set_multicast(fv->multicast());
	
	fa.set_prefix_len(subnet_mask.mask_len());
	if (fa.point_to_point())
	    fa.set_endpoint(peer_addr.get_ipv6());
	
	// Mark as deleted if necessary
	if (is_deleted)
	    fa.mark(IfTreeItem::DELETED);
	
	break;
    }
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
	break;
    }
}

#endif // HAVE_NETLINK_SOCKETS
