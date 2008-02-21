// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 International Computer Science Institute
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

#ident "$XORP: xorp/fea/data_plane/ifconfig/ifconfig_parse_netlink_socket.cc,v 1.15 2008/01/04 03:16:08 pavlin Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ether_compat.h"

#ifdef HAVE_LINUX_TYPES_H
#include <linux/types.h>
#endif
#ifdef HAVE_LINUX_RTNETLINK_H
#include <linux/rtnetlink.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_NET_IF_ARP_H
#include <net/if_arp.h>
#endif

#include "fea/ifconfig.hh"
#include "fea/ifconfig_get.hh"
#include "fea/data_plane/control_socket/system_utilities.hh"
#include "fea/data_plane/control_socket/netlink_socket_utilities.hh"

#include "ifconfig_get_netlink_socket.hh"


//
// Parse information about network interface configuration change from
// the underlying system.
//
// The information to parse is in NETLINK format
// (e.g., obtained by netlink(7) sockets mechanism).
//
// Reading netlink(3) manual page is a good start for understanding this
//

#ifdef HAVE_NETLINK_SOCKETS

static void nlm_newlink_to_fea_cfg(IfTree& iftree,
				   const struct ifinfomsg* ifinfomsg,
				   int rta_len);
static void nlm_dellink_to_fea_cfg(IfTree& iftree,
				   const struct ifinfomsg* ifinfomsg,
				   int rta_len);
static void nlm_newdeladdr_to_fea_cfg(IfTree& iftree,
				      const struct ifaddrmsg* ifaddrmsg,
				      int rta_len, bool is_deleted);

int
IfConfigGetNetlinkSocket::parse_buffer_netlink_socket(IfConfig& ifconfig,
						      IfTree& iftree,
						      const vector<uint8_t>& buffer)
{
    size_t buffer_bytes = buffer.size();
    AlignData<struct nlmsghdr> align_data(buffer);
    const struct nlmsghdr* nlh;
    bool recognized = false;

    UNUSED(ifconfig);

    for (nlh = align_data.payload();
	 NLMSG_OK(nlh, buffer_bytes);
	 nlh = NLMSG_NEXT(const_cast<struct nlmsghdr*>(nlh), buffer_bytes)) {
	void* nlmsg_data = NLMSG_DATA(const_cast<struct nlmsghdr*>(nlh));
	
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
		return (XORP_ERROR);
	    return (XORP_OK);
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
		nlm_newlink_to_fea_cfg(iftree, ifinfomsg, rta_len);
	    else
		nlm_dellink_to_fea_cfg(iftree, ifinfomsg, rta_len);
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
	    if (nlh->nlmsg_type == RTM_NEWADDR) {
		nlm_newdeladdr_to_fea_cfg(iftree, ifaddrmsg, rta_len, false);
	    } else {
		nlm_newdeladdr_to_fea_cfg(iftree, ifaddrmsg, rta_len, true);
	    }
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
	return (XORP_ERROR);
    
    return (XORP_OK);
}

static int
nlm_decode_ipvx_address(int family, const struct rtattr* rtattr,
			IPvX& ipvx_addr, bool& is_set, string& error_msg)
{
    is_set = false;

    if (rtattr == NULL) {
	error_msg = c_format("Missing address attribute to decode");
	return (XORP_ERROR);
    }

    //
    // Get the attribute information
    //
    size_t addr_size = RTA_PAYLOAD(rtattr);
    const uint8_t* addr_data = reinterpret_cast<const uint8_t*>(RTA_DATA(const_cast<struct rtattr*>(rtattr)));

    //
    // Test the address length
    //
    if (addr_size != IPvX::addr_bytelen(family)) {
	error_msg = c_format("Invalid address size payload: %u instead of %u",
			     XORP_UINT_CAST(addr_size),
			     XORP_UINT_CAST(IPvX::addr_bytelen(family)));
	return (XORP_ERROR);
    }

    //
    // Decode the address
    //
    ipvx_addr.copy_in(family, addr_data);
    is_set = true;

    return (XORP_OK);
}

static int
nlm_decode_ipvx_interface_address(const struct ifinfomsg* ifinfomsg,
				  const struct rtattr* rtattr,
				  IPvX& ipvx_addr, bool& is_set,
				  string& error_msg)
{
    int family = AF_UNSPEC;

    is_set = false;

    XLOG_ASSERT(ifinfomsg != NULL);

    if (rtattr == NULL) {
	error_msg = c_format("Missing address attribute to decode");
	return (XORP_ERROR);
    }

    // Decode the attribute type
    switch (ifinfomsg->ifi_type) {
    case ARPHRD_ETHER:
    {
	// MAC address: processed earlier when decoding the interface
	return (XORP_OK);
    }
    case ARPHRD_TUNNEL:
	// FALLTHROUGH
    case ARPHRD_SIT:
	// FALLTHROUGH
    case ARPHRD_IPGRE:
    {
	// IPv4 address
	family = IPv4::af();
	break;
    }
#ifdef HAVE_IPV6
    case ARPHRD_TUNNEL6:
    {
	// IPv6 address
	family = IPv6::af();
	break;
    }
#endif // HAVE_IPV6
    default:
	// Unknown address type: ignore
	return (XORP_OK);
    }

    XLOG_ASSERT(family != AF_UNSPEC);
    return (nlm_decode_ipvx_address(family, rtattr, ipvx_addr, is_set,
				    error_msg));
}

static void
nlm_newlink_to_fea_cfg(IfTree& iftree, const struct ifinfomsg* ifinfomsg,
		       int rta_len)
{
    const struct rtattr *rtattr;
    const struct rtattr *rta_array[IFLA_MAX + 1];
    u_short if_index = 0;
    string if_name;
    bool is_newlink = false;	// True if really a new link
    
    // The attributes
    memset(rta_array, 0, sizeof(rta_array));
    rtattr = IFLA_RTA(const_cast<struct ifinfomsg*>(ifinfomsg));
    NlmUtils::get_rtattr(rtattr, rta_len, rta_array,
			 sizeof(rta_array) / sizeof(rta_array[0]));
    
    //
    // Get the interface name
    //
    if (rta_array[IFLA_IFNAME] == NULL) {
	//
	// XXX: The kernel did not provide the interface name.
	// It could be because this is a wireless event carried to user
	// space. Such events are encapsulated in the IFLA_WIRELESS field of
	// a RTM_NEWLINK message.
	// Unfortunately, there is no easy way to verify whether this is
	// indeed a wireless event or a problem with the message integrity,
	// hence we silently ignore it.
	//
	debug_msg("Could not find interface name for interface index %d",
		  ifinfomsg->ifi_index);
	return;
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
    // Get the MAC address
    //
    if (rta_array[IFLA_ADDRESS] != NULL) {
	size_t addr_size = RTA_PAYLOAD(rta_array[IFLA_ADDRESS]);
	const uint8_t* addr_data = reinterpret_cast<const uint8_t*>(RTA_DATA(const_cast<struct rtattr*>(rta_array[IFLA_ADDRESS])));
	switch (ifinfomsg->ifi_type) {
	case ARPHRD_ETHER:
	{
	    // MAC address
	    struct ether_addr ea;
	    if (addr_size != sizeof(ea))
		break;
	    memcpy(&ea, addr_data, sizeof(ea));
	    EtherMac ether_mac(ea);
	    if (is_newlink || (ether_mac != EtherMac(ifp->mac())))
		ifp->set_mac(ether_mac);
	    break;
	}
	default:
	    // Either unknown type or type that will be processed later
	    break;
	}
    }
    debug_msg("MAC address: %s\n", ifp->mac().str().c_str());
    
    //
    // Get the MTU
    //
    if (rta_array[IFLA_MTU] != NULL) {
	unsigned int mtu;
	
	XLOG_ASSERT(RTA_PAYLOAD(rta_array[IFLA_MTU]) == sizeof(mtu));
	mtu = *reinterpret_cast<unsigned int*>(RTA_DATA(const_cast<struct rtattr*>(rta_array[IFLA_MTU])));
	if (is_newlink || (mtu != ifp->mtu()))
	    ifp->set_mtu(mtu);
    }
    debug_msg("MTU: %u\n", ifp->mtu());
    
    //
    // Get the flags
    //
    unsigned int flags = ifinfomsg->ifi_flags;
    if (is_newlink || (flags != ifp->interface_flags())) {
	ifp->set_interface_flags(flags);
	ifp->set_enabled(flags & IFF_UP);
    }
    debug_msg("enabled: %s\n", bool_c_str(ifp->enabled()));

    //
    // Get the link status
    //
    bool no_carrier = false;
    if ((flags & IFF_UP) && !(flags & IFF_RUNNING))
	no_carrier = true;
    else
	no_carrier = false;
    if (is_newlink || (no_carrier != ifp->no_carrier()))
	ifp->set_no_carrier(no_carrier);
    debug_msg("no_carrier: %s\n", bool_c_str(ifp->no_carrier()));
    
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
    }
    debug_msg("vif enabled: %s\n", bool_c_str(vifp->enabled()));
    debug_msg("vif broadcast: %s\n", bool_c_str(vifp->broadcast()));
    debug_msg("vif loopback: %s\n", bool_c_str(vifp->loopback()));
    debug_msg("vif point_to_point: %s\n", bool_c_str(vifp->point_to_point()));
    debug_msg("vif multicast: %s\n", bool_c_str(vifp->multicast()));

    //
    // Add any interface-specific addresses
    //
    IPvX lcl_addr(AF_INET);
    IPvX peer_addr(AF_INET);
    bool has_lcl_addr = false;
    bool has_peer_addr = false;
    string error_msg;

    // Get the local address
    if (rta_array[IFLA_ADDRESS] != NULL) {
	if (nlm_decode_ipvx_interface_address(ifinfomsg,
					      rta_array[IFLA_ADDRESS],
					      lcl_addr, has_lcl_addr,
					      error_msg)
	    != XORP_OK) {
	    XLOG_WARNING("Error decoding address for interface %s vif %s: %s",
			 vifp->ifname().c_str(), vifp->vifname().c_str(),
			 error_msg.c_str());
	}
    }
    if (! has_lcl_addr)
	return;			// XXX: nothing more to do
    lcl_addr = system_adjust_ipvx_recv(lcl_addr);
    debug_msg("IP address: %s\n", lcl_addr.str().c_str());

    // XXX: No info about the masklen: assume it is the address bitlen
    size_t mask_len = IPvX::addr_bitlen(lcl_addr.af());

    // Get the broadcast/peer address
    if (rta_array[IFLA_BROADCAST] != NULL) {
	if (nlm_decode_ipvx_interface_address(ifinfomsg,
					      rta_array[IFLA_BROADCAST],
					      peer_addr, has_peer_addr,
					      error_msg)
	    != XORP_OK) {
	    XLOG_WARNING("Error decoding broadcast/peer address for "
			 "interface %s vif %s: %s",
			 vifp->ifname().c_str(), vifp->vifname().c_str(),
			 error_msg.c_str());
	}
    }
    if (has_peer_addr)
	XLOG_ASSERT(lcl_addr.af() == peer_addr.af());

    // Add the address
    switch (lcl_addr.af()) {
    case AF_INET:
    {
	vifp->add_addr(lcl_addr.get_ipv4());
	IfTreeAddr4* ap = vifp->find_addr(lcl_addr.get_ipv4());
	XLOG_ASSERT(ap != NULL);
	ap->set_enabled(vifp->enabled());
	ap->set_broadcast(vifp->broadcast() && has_peer_addr);
	ap->set_loopback(vifp->loopback());
	ap->set_point_to_point(vifp->point_to_point() && has_peer_addr);
	ap->set_multicast(vifp->multicast());

	if (has_peer_addr) {
	    ap->set_prefix_len(mask_len);
	    if (ap->broadcast())
		ap->set_bcast(peer_addr.get_ipv4());
	    if (ap->point_to_point())
		ap->set_endpoint(peer_addr.get_ipv4());
	}

	break;
    }
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	vifp->add_addr(lcl_addr.get_ipv6());
	IfTreeAddr6* ap = vifp->find_addr(lcl_addr.get_ipv6());
	XLOG_ASSERT(ap != NULL);
	ap->set_enabled(vifp->enabled());
	ap->set_loopback(vifp->loopback());
	ap->set_point_to_point(vifp->point_to_point());
	ap->set_multicast(vifp->multicast());

	if (has_peer_addr) {
	    ap->set_prefix_len(mask_len);
	    if (ap->point_to_point())
		ap->set_endpoint(peer_addr.get_ipv6());
	}

	break;
    }
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
	break;
    }
}

static void
nlm_dellink_to_fea_cfg(IfTree& iftree, const struct ifinfomsg* ifinfomsg,
		       int rta_len)
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
    debug_msg("Deleting interface name: %s\n", if_name.c_str());
    
    //
    // Get the interface index
    //
    if_index = ifinfomsg->ifi_index;
    if (if_index == 0) {
	XLOG_FATAL("Could not find physical interface index "
		   "for interface %s",
		   if_name.c_str());
    }
    debug_msg("Deleting interface index: %d\n", if_index);
    
    //
    // Delete the interface and vif
    //
    IfTreeInterface* ifp = iftree.find_interface(if_index);
    if (ifp != NULL) {
	ifp->mark(IfTree::DELETED);
    }
    IfTreeVif* vifp = iftree.find_vif(if_index);
    if (vifp != NULL) {
	vifp->mark(IfTree::DELETED);
    }
}

static void
nlm_newdeladdr_to_fea_cfg(IfTree& iftree, const struct ifaddrmsg* ifaddrmsg,
			  int rta_len, bool is_deleted)
{
    const struct rtattr *rtattr;
    const struct rtattr *rta_array[IFA_MAX + 1];
    u_short if_index = 0;
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
    // Locate the vif to pin data on
    //
    IfTreeVif* vifp = iftree.find_vif(if_index);
    if (vifp == NULL) {
	if (is_deleted) {
	    //
	    // XXX: In case of Linux kernel we may receive first
	    // a message to delete the interface and then a message to
	    // delete an address on that interface.
	    // However, the first message would remove all state about
	    // that interface (including its addresses).
	    // Hence, we silently ignore messages for deleting addresses
	    // if the interface is not found.
	    //
	    return;
	}
	XLOG_FATAL("Could not find vif with index %u in IfTree", if_index);
    }
    debug_msg("Address event on interface %s vif %s with interface index %u\n",
	      vifp->ifname().c_str(), vifp->vifname().c_str(),
	      vifp->pif_index());

    //
    // Get the IP address, netmask, broadcast address, P2P destination
    //
    // The default values
    IPvX lcl_addr = IPvX::ZERO(family);
    IPvX subnet_mask = IPvX::ZERO(family);
    IPvX broadcast_addr = IPvX::ZERO(family);
    IPvX peer_addr = IPvX::ZERO(family);
    bool has_lcl_addr = false;
    bool has_broadcast_addr = false;
    bool has_peer_addr = false;
    bool is_ifa_address_reassigned = false;
    string error_msg;

    //
    // XXX: re-assign IFA_ADDRESS to IFA_LOCAL (and vice-versa).
    // This tweak is needed according to the iproute2 source code.
    //
    if (rta_array[IFA_LOCAL] == NULL) {
	rta_array[IFA_LOCAL] = rta_array[IFA_ADDRESS];
    }
    if (rta_array[IFA_ADDRESS] == NULL) {
	rta_array[IFA_ADDRESS] = rta_array[IFA_LOCAL];
	is_ifa_address_reassigned = true;
    }

    // Get the IP address
    if (rta_array[IFA_LOCAL] != NULL) {
	if (nlm_decode_ipvx_address(family, rta_array[IFA_LOCAL],
				    lcl_addr, has_lcl_addr, error_msg)
	    != XORP_OK) {
	    XLOG_FATAL("Error decoding address for interface %s vif %s: %s",
		       vifp->ifname().c_str(), vifp->vifname().c_str(),
		       error_msg.c_str());
	}
    }
    if (! has_lcl_addr) {
	XLOG_FATAL("Missing local address for interface %s vif %s",
		   vifp->ifname().c_str(), vifp->vifname().c_str());
    }
    lcl_addr = system_adjust_ipvx_recv(lcl_addr);
    debug_msg("IP address: %s\n", lcl_addr.str().c_str());
    
    // Get the netmask
    subnet_mask = IPvX::make_prefix(family, ifaddrmsg->ifa_prefixlen);
    debug_msg("IP netmask: %s\n", subnet_mask.str().c_str());
    
    // Get the broadcast address
    if (vifp->broadcast()) {
	switch (family) {
	case AF_INET:
	    if (rta_array[IFA_BROADCAST] != NULL) {
		if (nlm_decode_ipvx_address(family, rta_array[IFA_BROADCAST],
					    broadcast_addr, has_broadcast_addr,
					    error_msg)
		    != XORP_OK) {
		    XLOG_FATAL("Error decoding broadcast address for "
			       "interface %s vif %s: %s",
			       vifp->ifname().c_str(), vifp->vifname().c_str(),
			       error_msg.c_str());
		}
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
    if (vifp->point_to_point()) {
	if ((rta_array[IFA_ADDRESS] != NULL) && !is_ifa_address_reassigned) {
	    if (rta_array[IFA_ADDRESS] != NULL) {
		if (nlm_decode_ipvx_address(family, rta_array[IFA_ADDRESS],
					    peer_addr, has_peer_addr,
					    error_msg)
		    != XORP_OK) {
		    XLOG_FATAL("Error decoding peer address for "
			       "interface %s vif %s: %s",
			       vifp->ifname().c_str(), vifp->vifname().c_str(),
			       error_msg.c_str());
		}
	    }
	}
	debug_msg("Peer address: %s\n", peer_addr.str().c_str());
    }
    
    debug_msg("\n");		// put an empty line between interfaces
    
    // Add or delete the address
    switch (family) {
    case AF_INET:
    {
	vifp->add_addr(lcl_addr.get_ipv4());
	IfTreeAddr4* ap = vifp->find_addr(lcl_addr.get_ipv4());
	XLOG_ASSERT(ap != NULL);
	ap->set_enabled(vifp->enabled());
	ap->set_broadcast(vifp->broadcast() && has_broadcast_addr);
	ap->set_loopback(vifp->loopback());
	ap->set_point_to_point(vifp->point_to_point() && has_peer_addr);
	ap->set_multicast(vifp->multicast());
	
	ap->set_prefix_len(subnet_mask.mask_len());
	if (ap->broadcast())
	    ap->set_bcast(broadcast_addr.get_ipv4());
	if (ap->point_to_point())
	    ap->set_endpoint(peer_addr.get_ipv4());
	
	// Mark as deleted if necessary
	if (is_deleted)
	    ap->mark(IfTreeItem::DELETED);
	
	break;
    }
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	vifp->add_addr(lcl_addr.get_ipv6());
	IfTreeAddr6* ap = vifp->find_addr(lcl_addr.get_ipv6());
	XLOG_ASSERT(ap != NULL);
	ap->set_enabled(vifp->enabled());
	ap->set_loopback(vifp->loopback());
	ap->set_point_to_point(vifp->point_to_point());
	ap->set_multicast(vifp->multicast());
	
	ap->set_prefix_len(subnet_mask.mask_len());
	if (ap->point_to_point())
	    ap->set_endpoint(peer_addr.get_ipv6());
	
	// Mark as deleted if necessary
	if (is_deleted)
	    ap->mark(IfTreeItem::DELETED);
	
	break;
    }
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
	break;
    }
}

#endif // HAVE_NETLINK_SOCKETS
